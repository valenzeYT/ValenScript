from pathlib import Path
import re
import textwrap

interpreter_path = Path("interpreter.cpp")
lib_dir = Path("lib")

text = interpreter_path.read_text()
imp_token = "    if (auto imp = dynamic_cast<ImportNode*>(node)) {"
fc_token = "    if (auto fc = dynamic_cast<FunctionCallNode*>(node)) {"

imp_start = text.index(imp_token)
fc_start = text.index(fc_token)
import_block = text[imp_start:fc_start]

alias_token = "        if (!imp->alias.empty()) {"
alias_idx = import_block.index(alias_token)

module_block = import_block[import_block.index("if (importName == "):alias_idx]

pattern = re.compile(r'(?:if|else if)\s*\(importName == "([^"]+)"\)\s*\{')
modules = []
search_pos = 0

while True:
    match = pattern.search(module_block, search_pos)
    if not match:
        break
    module_name = match.group(1)
    body_start = match.end()
    depth = 1
    idx = body_start
    while depth > 0:
        if idx >= len(module_block):
            raise RuntimeError("Unbalanced braces while parsing module " + module_name)
        ch = module_block[idx]
        if ch == "{":
            depth += 1
        elif ch == "}":
            depth -= 1
        idx += 1
    body = module_block[body_start:idx - 1]
    modules.append((module_name, body))
    search_pos = idx


def convert_body(body: str) -> str:
    def convert_assignment(match: re.Match) -> str:
        indent = match.group("indent")
        module = match.group("module")
        key = match.group("key").strip()
        capture = match.group("capture").strip()
        args = match.group("args")

        if capture:
            capture = capture.replace("this", "&interp")
        else:
            capture = "&interp"

        if key.startswith('"') and key.endswith('"'):
            key_expr = key
        else:
            key_expr = key

        return (
            f"{indent}interp.registerModuleFunction(\"{module}\", {key_expr}, "
            f"[{capture}]({args}) -> Value {{"
        )

    assignment = re.compile(
        r'(?P<indent>[ \t]*)modules\["(?P<module>[^"]+)"\]\[(?P<key>[^\]]+)\]\s*=\s*\[(?P<capture>[^\]]*)\]\((?P<args>[^)]*)\)\s*->\s*Value\s*\{',
        re.MULTILINE,
    )

    out = assignment.sub(convert_assignment, body)
    out = out.replace("expectArity(", "interp.expectArity(")
    out = out.replace("expectString(", "interp.expectString(")
    out = out.replace("expectNumber(", "interp.expectNumber(")
    out = out.replace("fireEvent(", "interp.fireEvent(")
    return out


registry_entries = []
for module_name, body in modules:
    converted = convert_body(body)
    converted = textwrap.dedent(converted).strip("\n")
    if converted:
        indented = textwrap.indent(converted + "\n", "            ")
    else:
        indented = ""
    entry = (
        f'        module_registry::registerModule("{module_name}", [](Interpreter& interp) {{\n'
        f"{indented}"
        f"        }});\n"
    )
    registry_entries.append(entry)

registry_code = "".join(registry_entries)

builtins_path = lib_dir / "builtins_registry.cpp"
builtins_content = f"""#include "../include/module_registry.h"
#include "../include/interpreter.h"

namespace {{
struct BuiltinsRegistrar {{
    BuiltinsRegistrar() {{
{registry_code}    }}
}};

static BuiltinsRegistrar registrar;
}} // namespace
"""
builtins_path.write_text(builtins_content)

replacement = (
    "    if (auto imp = dynamic_cast<ImportNode*>(node)) {\n"
    "        const std::string importName = imp->moduleName;\n"
    "        auto initializer = module_registry::getModule(importName);\n"
    "        if (!initializer) {\n"
    "            throw std::runtime_error(\"Unknown module: \" + importName);\n"
    "        }\n"
    "        initializer(*this);\n"
    "        if (!imp->alias.empty()) {\n"
    "            modules[imp->alias] = modules[importName];\n"
    "        }\n"
    "        return Value::fromNumber(0.0);\n"
    "    }\n"
)

text = text[:imp_start] + replacement + text[fc_start:]
interpreter_path.write_text(text)
