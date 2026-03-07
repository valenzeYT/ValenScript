# ValenScript

ValenScript is a custom scripting language and runtime.

## Repo Structure
- `src/` C++ source files
- `include/` public headers
- `lib/` internal modules (C++ sources used by the runtime)
- `build/` build artifacts (`.o`)
- `bin/` compiled executables
- `assets/` non-code assets
- `tools/` code generators and helper scripts
- `.vscode/` editor settings and tasks

## VS Code IntelliSense (Imported Libs Only)
ValenScript uses workspace snippets for library function IntelliSense. Snippets are generated from installed lib sources and only include modules that are currently `IMPORT`ed in your `.vs` files.

- Build the generator: run the VS Code task `Build ValenScript Tools` (or run `powershell -NoProfile -ExecutionPolicy Bypass -File tools\\build_tools.ps1`).
- Keep snippets updated automatically (recommended): run `Watch ValenScript Snippets (Active File)`.
- Optional workspace-wide mode: `Watch ValenScript Snippets (Workspace)` (shows union of imports across the repo).
