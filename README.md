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

## Build (example)
Use your compiler of choice. If you have a specific build command, document it here.

## GitHub Sync (Push local changes)
```bash
git status -sb
git add .
git commit -m "Describe your change"
git push
