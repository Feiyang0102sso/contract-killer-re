"""Generate the Win32 copies of the Swerve headers that call the ARM engine through its tables.

The game is compiled for x86, but the Swerve objects live in the emulated ARM engine: their
tables hold ARM code addresses. This script rewrites the recovered headers so that every table
call goes through ck::arm (pc_port/swerve_compat/ArmCall.h):

- twb_queryinterface.h, tws.h: `virtual R f(args) = 0;` becomes
  `R f(args) { return ck::arm::Method<R>(this, slot, names); }`; slots count on from the base class.
- SwerveApi.h, SwvPort.h: `R (*Name)(args);` inside `struct *Functions` becomes
  `ck::arm::Function<R(args)> Name;`, and `const *Functions* m_functions;` a ck::arm::Table.

Run it again whenever one of the recovered headers changes.
"""

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PACKAGES = ROOT / "CK_source/packages"
OUTPUT = ROOT / "CK_source/pc_port/swerve_compat"

QUERY_INTERFACE = PACKAGES / "swerve_tw/code/inc/twb_queryinterface.h"
WRAPPERS = PACKAGES / "swerve_tw/code/inc/tws.h"
API = PACKAGES / "swerve/code/inc/SwerveApi.h"
PORT = PACKAGES / "swerve/code/inc/SwvPort.h"

# CK's direct slot call in tws.h (also used by game files) and its replacement: the entry may be ARM
# code, so the call goes through ck::arm::Function (the callers pass the object themselves).
ORIGINAL_SLOT_HELPER = """template <class Method>
Method CKMethod(IQueryInterface* object, unsigned int offset)
{
    return reinterpret_cast<Method>((*reinterpret_cast<void***>(object))[offset / 4]);
}"""
ARM_SLOT_HELPER = """template <class Method>
ck::arm::Function<std::remove_pointer_t<Method>> CKMethod(IQueryInterface* object, unsigned int offset)
{
    const uint32_t table = *reinterpret_cast<const uint32_t*>(object);
    const uint32_t* entries = static_cast<const uint32_t*>(ck::arm::ResolvePointer(table));
    return { entries[offset / 4] };
}"""

CLASS_START = re.compile(r"^class (\w+)(?: : public (\w+))? \{")
VIRTUAL_START = re.compile(r"^(\s*)virtual (.+?)\b(\w+)\((.*)$", re.S)
POINTER_START = re.compile(r"^(\s*)(.+?) \(\*(\w+)\)\((.*)$", re.S)
FUNCTIONS_START = re.compile(r"^struct (\w+Functions) \{")
TABLE_MEMBER = re.compile(r"^(\s*)const (\w+Functions)\* m_functions;")


def split_parameters(text):
    """Split a parameter list on its top-level commas."""
    parts = []
    depth = 0
    current = ""
    for character in text:
        if character in "(<[":
            depth += 1
        elif character in ")>]":
            depth -= 1
        if character == "," and depth == 0:
            parts.append(current)
            current = ""
        else:
            current += character
    parts.append(current)
    return parts


def argument_names(parameters):
    """Names of the parameters, in order (empty for `()` and `(void)`)."""
    names = []
    for parameter in split_parameters(parameters):
        parameter = parameter.split("=")[0].strip()
        if parameter == "" or parameter == "void":
            continue
        match = re.search(r"(\w+)\s*(\[[^\]]*\])?$", parameter)
        if match is None:
            raise ValueError(f"no parameter name in '{parameter}'")
        names.append(match.group(1))
    return names


def collect_declaration(lines, index, terminator):
    """Join lines from `index` until one contains `terminator`; return (text, last index)."""
    text = lines[index]
    while terminator not in text:
        index += 1
        text += "\n" + lines[index]
    return text, index


def convert_virtuals(source, base_slots):
    """Rewrite the pure virtual methods of every interface class; returns (text, slot counts)."""
    lines = source.split("\n")
    output = []
    slots = dict(base_slots)
    current_class = None
    next_slot = 0
    index = 0
    while index < len(lines):
        line = lines[index]
        class_match = CLASS_START.match(line)
        if class_match is not None:
            current_class = class_match.group(1)
            base = class_match.group(2)
            next_slot = 0
            if base is not None:
                next_slot = slots.get(base, 0)
            output.append(line)
            index += 1
            continue
        if current_class is not None and line.startswith("};"):
            slots[current_class] = next_slot
            current_class = None
            output.append(line)
            index += 1
            continue
        if current_class is None or "virtual " not in line:
            output.append(line)
            index += 1
            continue

        text, last = collect_declaration(lines, index, "= 0;")
        match = VIRTUAL_START.match(text)
        body, tail = text.split("= 0;", 1)
        signature_end = body.rstrip()
        if match is None or not signature_end.endswith(")"):
            raise ValueError(f"cannot parse virtual declaration in {current_class}: {text}")
        indent = match.group(1)
        result_type = match.group(2).strip()
        name = match.group(3)
        parameters = signature_end[signature_end.index(name + "(") + len(name) + 1:-1]
        arguments = [str(next_slot)] + argument_names(parameters)
        call = f"return ck::arm::Method<{result_type}>(this, {', '.join(arguments)});"
        declaration = signature_end.replace("virtual ", "", 1)
        output.append(f"{declaration} {{ {call} }}{tail}")
        next_slot += 1
        index = last + 1
    return "\n".join(output), slots


def convert_tables(source):
    """Rewrite the function pointers of every `struct I*Functions` table."""
    lines = source.split("\n")
    output = []
    inside_table = False
    index = 0
    while index < len(lines):
        line = lines[index]
        if FUNCTIONS_START.match(line) is not None:
            inside_table = True
            output.append(line)
            index += 1
            continue
        if inside_table and line.startswith("};"):
            inside_table = False
            output.append(line)
            index += 1
            continue
        table_match = TABLE_MEMBER.match(line)
        if table_match is not None:
            output.append(TABLE_MEMBER.sub(r"\1ck::arm::Table<\2> m_functions;", line))
            index += 1
            continue
        pointer_match = POINTER_START.match(line)
        if not inside_table or pointer_match is None:
            output.append(line)
            index += 1
            continue

        text, last = collect_declaration(lines, index, ");")
        indent, result_type, name, _ = pointer_match.groups()
        body, tail = text.split(");", 1)
        prefix = f"{indent}{result_type} (*{name})("
        parameters = body[len(prefix):]
        output.append(f"{indent}ck::arm::Function<{result_type}({parameters})> {name};{tail}")
        index = last + 1
    return "\n".join(output)


def convert_slot_helper(source):
    """CK's CKMethod<F>(object, offset) returns the raw table entry; return an ck::arm::Function instead."""
    if ORIGINAL_SLOT_HELPER not in source:
        raise ValueError("CKMethod helper not found in tws.h")
    return source.replace(ORIGINAL_SLOT_HELPER, ARM_SLOT_HELPER)


def write(path, header, text):
    """Write a generated header (LF line ends)."""
    path.write_bytes((header + text).encode("utf-8"))
    print(f"Generated {path.relative_to(ROOT)}")


def main():
    query_text, slots = convert_virtuals(QUERY_INTERFACE.read_text(encoding="utf-8"), {})
    write(OUTPUT / "twb_queryinterface.h",
          "// GENERATED by scripts/make_arm_headers.py: Win32 copy of the recovered query interface\n"
          "// using the ARM virtual table.\n#include \"ArmCall.h\"\n", query_text)

    wrapper_text, slots = convert_virtuals(WRAPPERS.read_text(encoding="utf-8"), slots)
    wrapper_text = convert_slot_helper(wrapper_text)
    write(OUTPUT / "tws.h",
          "// GENERATED by scripts/make_arm_headers.py: Win32 copy of the Swerve wrappers with explicit\n"
          "// calls to ARM virtual slots.\n#include \"ArmCall.h\"\n", wrapper_text)
    print(f"{len(slots)} interfaces: " + ", ".join(f"{name}={count}" for name, count in slots.items()))

    write(OUTPUT / "SwerveApi.h",
          "// GENERATED by scripts/make_arm_headers.py: Win32 copy of the recovered API with one ARM word\n"
          "// per function-table entry.\n#include \"ArmCall.h\"\n",
          convert_tables(API.read_text(encoding="utf-8")))

    write(OUTPUT / "SwvPort.h",
          "// GENERATED by scripts/make_arm_headers.py: Win32 copy of the porting layer types with one ARM\n"
          "// word per function-table entry.\n#include \"ArmCall.h\"\n",
          convert_tables(PORT.read_text(encoding="utf-8")))


if __name__ == "__main__":
    main()
