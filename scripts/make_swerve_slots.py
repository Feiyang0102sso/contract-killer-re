"""Generate the slot numbers of CK's Swerve object tables for the native m3gcore backend.

Every Swerve object of CK 1.5.2 starts with a pointer to a table of functions. The original engine
core (CK_source/packages/swerve/code/cocoa/libs/swerve_core.s) fills these tables in its
`<class>_initialiseVtable` functions, which first call the parent's initialiser and then store
pc-relative function addresses at fixed offsets; the runtime root's table is filled by swvNew.
The functions keep their original names (object3d_find, group_pickNode, ...), so interpreting the
initialisers gives the exact slot -> function map of every class.

Output: CK_source/pc_port/swerve_native/SwerveSlots.h
- ck::native::slot::<prefix>::<method>: slot number of each original function, grouped by the
  function's name prefix (object3d_find -> slot::object3d::find).
- ck::native::slot::names::<class>[]: original function name of every slot of a class, for the
  "slot is not implemented" log of the native backend.

Run after swerve_core.s changes:  python scripts/make_swerve_slots.py
"""

import struct
from pathlib import Path

import make_swerve_image as image_script

ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "CK_source/pc_port/swerve_native/SwerveSlots.h"

# Slots 0-2 of every object are set by the object base (swervehandle_*), not by the initialisers.
BASE_SLOTS = ["AddRef", "Release", "QueryInterface"]
# swvNew keeps the root's table inside the root object, right after two words of state.
ROOT_TABLE_OFFSET = 8
# Instructions decoded by the interpreter (ARM, condition AL unless masked out)
INSTRUCTION_BRANCH_LINK = 0x0B000000
INSTRUCTION_LOAD_LITERAL = 0x059F0000
INSTRUCTION_ADD_PC = 0x008F0000
INSTRUCTION_MOVE = 0x01A00000
INSTRUCTION_RETURN = 0xE12FFF1E
MAX_INSTRUCTIONS = 400


class Image:
    """The laid-out engine core with its label table."""

    def __init__(self):
        statements = image_script.parse(image_script.ASSEMBLY)
        labels, addresses, _ends = image_script.layout(statements)
        imports = image_script.collect_imports(statements, labels)
        slots = {}
        for index, name in enumerate(imports):
            slots[name] = index
        self.bytes = image_script.build_image(statements, labels, addresses, slots)
        self.labels = labels
        self.names = {}
        for name, address in labels.items():
            self.names.setdefault(address, name)

    def word(self, address):
        return struct.unpack_from("<I", self.bytes, address - image_script.IMAGE_BASE)[0]

    def name_at(self, address):
        name = self.names.get(address)
        if name is None:
            raise ValueError(f"no label at {address:#x}")
        return name.removeprefix("_")


def branch_target(instruction, address):
    """Target of a bl at `address`."""
    displacement = instruction & 0xFFFFFF
    if displacement & 0x800000:
        displacement -= 0x1000000
    return address + 8 + displacement * 4


def interpret(image, function, table_register_value, table_offset, table):
    """
    Run the straight-line code of `function`: track literal loads, pc-relative adds and moves, and
    record every function address stored relative to the table pointer (r0 on entry).
    Calls of other initialisers are followed (the parent fills its slots first).
    """
    registers = {0: table_register_value}
    address = image.labels["_" + function]
    for _ in range(MAX_INSTRUCTIONS):
        instruction = image.word(address)
        if instruction == INSTRUCTION_RETURN:
            return
        if (instruction & 0xFFFF0000) == 0xE8BD0000 and (instruction & 0x8000):  # pop {..., pc} (always)
            return
        if (instruction & 0x0F000000) == INSTRUCTION_BRANCH_LINK:
            target = branch_target(instruction, address)
            callee = ""
            if target < image_script.TRAP_BASE:
                callee = image.name_at(target)
            if callee.endswith("_initialiseVtable"):
                interpret(image, callee, registers.get(4), table_offset, table)
            # r0-r3 are not preserved across a call; the table pointer survives in r4.
            for register in (0, 1, 2, 3, 12):
                registers.pop(register, None)
            # swvNew: the allocation it just made is the root object.
            if table_register_value == "ROOT":
                registers[0] = "ROOT"
        elif (instruction & 0x0FFF0000) == INSTRUCTION_LOAD_LITERAL:
            destination = (instruction >> 12) & 15
            registers[destination] = image.word(address + 8 + (instruction & 0xFFF))
        elif (instruction & 0x0FFF0FF0) == INSTRUCTION_ADD_PC:
            destination = (instruction >> 12) & 15
            offset = registers.get(instruction & 15)
            if isinstance(offset, int):
                registers[destination] = ("code", (address + 8 + offset) & 0xFFFFFFFF)
        elif (instruction & 0x0FFF0FF0) == INSTRUCTION_MOVE:
            destination = (instruction >> 12) & 15
            registers[destination] = registers.get(instruction & 15)
        elif (instruction & 0x0F700000) == 0x05000000 or (instruction & 0x0F700000) == 0x05800000:
            # str rd, [rn, #imm]
            source = registers.get((instruction >> 12) & 15)
            base = registers.get((instruction >> 16) & 15)
            if base == table_register_value and isinstance(source, tuple):
                offset = instruction & 0xFFF
                table[(offset - table_offset) // 4] = image.name_at(source[1])
        address += 4
    raise ValueError(f"{function}: no return found")


def class_tables(image):
    """Slot table of every class: {class name: {slot: function name}}."""
    tables = {}
    for label in sorted(image.labels):
        if not label.endswith("_initialiseVtable"):
            continue
        name = label.removeprefix("_").removesuffix("_initialiseVtable")
        table = {}
        for slot, base_name in enumerate(BASE_SLOTS):
            table[slot] = base_name
        interpret(image, label.removeprefix("_"), "TABLE", 0, table)
        tables[name] = table
    # The runtime root (swvNew): its table lives inside the object returned by swvSystemAllocate.
    root = {}
    interpret(image, "swvNew", "ROOT", ROOT_TABLE_OFFSET, root)
    tables["swerve"] = root
    return tables


def method_slots(tables):
    """{prefix: {method: slot}}, checking that a function has the same slot in every class."""
    methods = {}
    for class_name, table in tables.items():
        for slot, function in table.items():
            if function in BASE_SLOTS:
                continue
            prefix, method = function.split("_", 1)
            known = methods.setdefault(prefix, {}).get(method)
            if known is not None and known != slot:
                raise ValueError(f"{function}: slot {slot} in {class_name}, {known} elsewhere")
            methods[prefix][method] = slot
    return methods


def write_header(tables, methods):
    lines = [
        "// GENERATED by scripts/make_swerve_slots.py from swerve_core.s (*_initialiseVtable, swvNew). Do not edit.",
        "// Slot numbers of CK's Swerve object tables, named after the original engine functions.",
        "#pragma once",
        "",
        "namespace ck::native::slot {",
        "",
        "// Every object (set by the object base, swervehandle_*)",
    ]
    for slot, name in enumerate(BASE_SLOTS):
        lines.append(f"constexpr int {name} = {slot};")
    for prefix in sorted(methods):
        lines.append("")
        lines.append(f"namespace {prefix} {{")
        for method, slot in sorted(methods[prefix].items(), key=lambda item: item[1]):
            lines.append(f"constexpr int {method} = {slot}; // 0x{slot * 4:02x}")
        lines.append(f"}} // namespace {prefix}")
    lines.append("")
    lines.append("/** Original function of every slot of each class (index = slot). */")
    lines.append("namespace names {")
    for class_name in sorted(tables):
        table = tables[class_name]
        count = max(table) + 1
        entries = []
        for slot in range(count):
            entries.append('"' + table.get(slot, "") + '"')
        lines.append(f"constexpr const char* {class_name}[] = {{")
        for start in range(0, count, 4):
            lines.append("    " + ", ".join(entries[start:start + 4]) + ",")
        lines.append("};")
    lines.append("} // namespace names")
    lines.append("")
    lines.append("} // namespace ck::native::slot")
    lines.append("")
    OUTPUT.write_bytes("\n".join(lines).encode("utf-8"))
    print(f"Wrote {OUTPUT.relative_to(ROOT)}: {len(tables)} classes")


def main():
    image = Image()
    tables = class_tables(image)
    methods = method_slots(tables)
    write_header(tables, methods)


if __name__ == "__main__":
    main()
