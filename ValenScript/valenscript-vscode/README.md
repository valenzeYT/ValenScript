# ValenScript VS Code Extension

This adds:
- Syntax highlighting for `.vs`
- IntelliSense completions for keywords, built-ins, modules, and module functions
- Event name completion after `WHEN ` (for `WHEN module.func ...`)
- Snippets (`when`, `func`, `if`, `ifelse`, `for`, `while`, `import`, `print`)

## Run locally (fastest)

1. Open `valenscript-vscode` folder in VS Code.
2. Press `F5` (Run Extension).
3. In the Extension Development Host window, open a `.vs` file.

## Install as VSIX (optional)

1. Install packaging tool:
   - `npm i -g @vscode/vsce`
2. From `valenscript-vscode` folder:
   - `vsce package`
3. Install generated `.vsix`:
   - `code --install-extension valenscript-vscode-0.1.0.vsix`

## Notes

- Event hooks are now function-based, so write handlers like:

```vs
IMPORT os
WHEN os.write path content
	PRINT ["write", path, content]
```

- Dot completion works after module name:

```vs
os.
```
