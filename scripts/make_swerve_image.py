"""Build the ARM image of the Swerve engine core that the PC port runs in Unicorn.

The engine core of CK 1.5.2 is kept as assembly (CK_source/packages/swerve/code/cocoa/libs/
swerve_core.s): words, bytes, labels and `bl` calls of imported functions. This script lays the
sections out at their addresses in the original binary (so guest addresses match a disassembly of
the original), resolves every label and writes:

- swerve-arm.bin (--output): magic + the image from IMAGE_BASE to TRAP_BASE. Absolute pointers are
  moved by GUEST_OFFSET; every reference to an import points to its slot in the trap page.
- --headers: CK_source/pc_port/swerve_compat/ArmImportNames.inc (import names in slot order) and
  SwerveImage.h (image layout and the guest entry points the host calls).

Run with --headers after swerve_core.s changes; the build runs it with --output.
"""

import argparse
import re
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ASSEMBLY = ROOT / "CK_source/packages/swerve/code/cocoa/libs/swerve_core.s"
HEADER_DIRECTORY = ROOT / "CK_source/pc_port/swerve_compat"

MAGIC = b"CKARM02\0"
# Guest addresses sit high so they never collide with host memory, which is mapped 1:1.
GUEST_OFFSET = 0xE0000000
# Start of each section in the original binary (ck_repo/recovery/tools/ck_swerve_blob.py).
SECTION_BASES = {
    "__TEXT,__text": 0x22AF94,
    "__TEXT,__cstring": 0x2DE798,
    "__TEXT,__swv_empty": 0x2DEC70,
    "__TEXT,__const": 0x2F6738,
    "__DATA,__data": 0x32F87C,
    "__DATA,__const": 0x34556C,
    # The original GOT entries were scattered over __nl_symbol_ptr; here they get the first page
    # of the original __DATA segment, which holds nothing of the engine.
    "__DATA,__swv_got": 0x2F7000,
}
IMAGE_BASE = 0x22A000
TRAP_BASE = 0x348000          # one page of import slots right after the image (4 bytes each)
# Imports the host adds for its own use (not referenced by the assembly).
EXTRA_IMPORTS = ["swvPopulateVBO"]
# Guest functions the host calls.
ENTRY_POINTS = {"SwvNew": "_swvNew", "DecompressAcceptBytes": "_decompress_AcceptBytes"}

LABEL = re.compile(r"^([A-Za-z_.$][\w.$]*):$")
ARM_BL = 0xEB000000


def parse(path):
    """Return the statements of the assembly as (section, kind, argument) tuples."""
    statements = []
    section = None
    for raw in path.read_text(encoding="utf-8").split("\n"):
        line = raw.split("@", 1)[0].strip()
        if line == "":
            continue
        label = LABEL.match(line)
        if label is not None:
            statements.append((section, "label", label.group(1)))
            continue
        parts = line.split(None, 1)
        directive = parts[0]
        argument = ""
        if len(parts) > 1:
            argument = parts[1].strip()
        if directive == ".section":
            fields = argument.split(",")
            section = fields[0] + "," + fields[1]
        elif directive in (".long", ".byte", ".align", "bl"):
            statements.append((section, directive, argument))
        elif directive in (".globl", ".private_extern", ".code"):
            continue
        else:
            raise ValueError(f"unsupported line: {raw}")
    return statements


def size_of(kind, argument, address):
    """Bytes a statement occupies at `address`."""
    if kind == ".long" or kind == "bl":
        return 4
    if kind == ".byte":
        return 1
    if kind == ".align":
        alignment = 1 << int(argument)
        return (-address) % alignment
    return 0


def layout(statements):
    """Assign addresses; returns the label table and the address of every statement."""
    cursor = dict(SECTION_BASES)
    labels = {}
    addresses = []
    for section, kind, argument in statements:
        address = cursor[section]
        addresses.append(address)
        if kind == "label":
            labels[argument] = address
        cursor[section] = address + size_of(kind, argument, address)
    return labels, addresses, cursor


def collect_imports(statements, labels):
    """Imported symbols (referenced, never defined) in slot order; slot 0 stays unused."""
    names = set()
    for _section, kind, argument in statements:
        if kind != "bl" and kind != ".long":
            continue
        for token in argument.split(" - "):
            if token[0].isdigit() or token in labels:
                continue
            names.add(token)
    imports = [""]
    for name in sorted(names):
        imports.append(name.removeprefix("_"))
    imports.extend(EXTRA_IMPORTS)
    return imports


def symbol_value(token, labels, slots):
    """Guest address of a label, or of the trap slot of an import."""
    if token in labels:
        return labels[token]
    return TRAP_BASE + slots[token.removeprefix("_")] * 4


def evaluate(expression, labels, slots):
    """Value of a .long expression: a number, `a`, or `a - b`; absolute addresses are relocated."""
    if expression.startswith("0x") or expression.isdigit():
        return int(expression, 0)
    if " - " in expression:
        left, right = expression.split(" - ")
        return (symbol_value(left, labels, slots) - symbol_value(right, labels, slots)) & 0xFFFFFFFF
    return (symbol_value(expression, labels, slots) + GUEST_OFFSET) & 0xFFFFFFFF


def build_image(statements, labels, addresses, slots):
    """The bytes from IMAGE_BASE to TRAP_BASE."""
    image = bytearray(TRAP_BASE - IMAGE_BASE)
    for (section, kind, argument), address in zip(statements, addresses):
        offset = address - IMAGE_BASE
        if kind == ".long":
            struct.pack_into("<I", image, offset, evaluate(argument, labels, slots))
        elif kind == ".byte":
            image[offset] = int(argument, 0)
        elif kind == "bl":
            target = symbol_value(argument, labels, slots)
            displacement = (target - (address + 8)) >> 2
            struct.pack_into("<I", image, offset, ARM_BL | (displacement & 0xFFFFFF))
    return image


def write_headers(imports, labels, code_end):
    """ArmImportNames.inc and SwerveImage.h for pc_port/swerve_compat."""
    lines = ["// GENERATED by scripts/make_swerve_image.py: Swerve imports in trap slot order (slot 0 unused)."]
    for name in imports[1:]:
        lines.append(f'"{name}",')
    (HEADER_DIRECTORY / "ArmImportNames.inc").write_bytes(("\n".join(lines) + "\n").encode("utf-8"))

    header = [
        "// GENERATED by scripts/make_swerve_image.py: layout of swerve-arm.bin (addresses before GuestOffset).",
        "#pragma once",
        "#include <stdint.h>",
        "",
        "namespace ck::arm::layout {",
        "",
        f"constexpr char Magic[8] = \"{MAGIC[:-1].decode()}\";",
        f"constexpr uint32_t GuestOffset = 0x{GUEST_OFFSET:08x};",
        f"constexpr uint32_t ImageBase = 0x{IMAGE_BASE:06x};",
        f"constexpr uint32_t CodeStart = 0x{SECTION_BASES['__TEXT,__text']:06x};",
        f"constexpr uint32_t CodeEnd = 0x{code_end:06x};",
        f"constexpr uint32_t TrapBase = 0x{TRAP_BASE:06x};",
    ]
    for name, symbol in ENTRY_POINTS.items():
        header.append(f"constexpr uint32_t {name} = 0x{labels[symbol]:06x}; // {symbol.removeprefix('_')}")
    header += ["", "} // namespace ck::arm::layout", ""]
    (HEADER_DIRECTORY / "SwerveImage.h").write_bytes("\n".join(header).encode("utf-8"))
    print(f"Wrote ArmImportNames.inc ({len(imports) - 1} imports) and SwerveImage.h")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, help="write the image here")
    parser.add_argument("--headers", action="store_true", help="regenerate the pc_port headers")
    args = parser.parse_args()

    statements = parse(ASSEMBLY)
    labels, addresses, ends = layout(statements)
    for section, end in ends.items():
        if end > TRAP_BASE or SECTION_BASES[section] < IMAGE_BASE:
            raise ValueError(f"section {section} leaves the image")
    imports = collect_imports(statements, labels)
    if len(imports) * 4 > 0x800:
        raise ValueError("too many imports for the trap page")
    slots = {}
    for index, name in enumerate(imports):
        slots[name] = index

    if args.headers:
        write_headers(imports, labels, ends["__TEXT,__text"])
    if args.output is not None:
        image = MAGIC + build_image(statements, labels, addresses, slots)
        if not args.output.is_file() or args.output.read_bytes() != image:
            args.output.parent.mkdir(parents=True, exist_ok=True)
            temporary = args.output.with_name(args.output.name + ".tmp")
            temporary.write_bytes(image)
            temporary.replace(args.output)
        print(f"Swerve ARM image ready: {args.output}")


if __name__ == "__main__":
    main()
