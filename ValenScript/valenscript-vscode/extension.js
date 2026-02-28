const vscode = require('vscode');
const fs = require('fs');
const path = require('path');

const KEYWORDS = [
  'PRINT', 'WAIT', 'WHEN', 'WHILE', 'IF', 'ELSE', 'REPEAT', 'TIMES',
  'FOR', 'TO', 'FUNC', 'RETURN', 'BREAK', 'CONTINUE', 'IMPORT',
  'AND', 'OR', 'NOT', 'TRUE', 'FALSE', 'TYPE'
];

const BUILTINS = [
  'SPAWN', 'TASK_DONE', 'AWAIT', 'TASK_RESULT',
  'CHANNEL_CREATE', 'CHANNEL_SEND', 'CHANNEL_RECV',
  'SET_TIMEOUT', 'LEN', 'TYPE', 'NUM', 'BOOL',
  'STRINGIFIED', 'NUMIFIED', 'BOOLIFIED', 'STDIN', 'ASSERT'
];

function loadModuleMap(context) {
  const p = path.join(context.extensionPath, 'data', 'module-functions.json');
  const raw = fs.readFileSync(p, 'utf8');
  return JSON.parse(raw);
}

function addUnique(items, item, keySet) {
  const k = `${item.label}:${item.kind}`;
  if (!keySet.has(k)) {
    keySet.add(k);
    items.push(item);
  }
}

function keywordItem(name) {
  const item = new vscode.CompletionItem(name, vscode.CompletionItemKind.Keyword);
  item.insertText = name;
  return item;
}

function functionItem(name) {
  const item = new vscode.CompletionItem(name, vscode.CompletionItemKind.Function);
  item.insertText = name;
  return item;
}

function moduleItem(name) {
  const item = new vscode.CompletionItem(name, vscode.CompletionItemKind.Module);
  item.insertText = name;
  return item;
}

function eventItem(name) {
  const item = new vscode.CompletionItem(name, vscode.CompletionItemKind.Event);
  item.insertText = name;
  return item;
}

function importedModules(document, position) {
  const text = document.getText(new vscode.Range(new vscode.Position(0, 0), position));
  const re = /^\s*IMPORT\s+([A-Za-z_][A-Za-z0-9_]*)/gm;
  const out = new Set();
  for (const m of text.matchAll(re)) {
    out.add(m[1]);
  }
  return out;
}

function activate(context) {
  const moduleMap = loadModuleMap(context);
  const modules = Object.keys(moduleMap).sort();
  const moduleFuncPairs = modules.flatMap((m) => moduleMap[m].map((f) => `${m}.${f}`));

  const provider = vscode.languages.registerCompletionItemProvider(
    'valenscript',
    {
      provideCompletionItems(document, position) {
        const line = document.lineAt(position.line).text;
        const left = line.slice(0, position.character);
        const leftTrim = left.trimStart();
        const items = [];
        const keySet = new Set();

        const dotMatch = left.match(/\b([A-Za-z_][A-Za-z0-9_]*)\.([A-Za-z0-9_]*)?$/);
        if (dotMatch) {
          const mod = dotMatch[1];
          const funcs = moduleMap[mod] || [];
          for (const f of funcs) {
            const it = functionItem(f);
            it.detail = `${mod}.${f}`;
            addUnique(items, it, keySet);
          }
          return items;
        }

        if (/^\s*IMPORT\s+[A-Za-z_0-9]*$/i.test(left)) {
          for (const mod of modules) {
            addUnique(items, moduleItem(mod), keySet);
          }
          return items;
        }

        if (/^\s*WHEN\s+[A-Za-z0-9_\.]*$/i.test(left)) {
          for (const evt of moduleFuncPairs) {
            addUnique(items, eventItem(evt), keySet);
          }
          return items;
        }

        for (const kw of KEYWORDS) {
          addUnique(items, keywordItem(kw), keySet);
        }

        for (const fn of BUILTINS) {
          addUnique(items, functionItem(fn), keySet);
        }

        const imported = importedModules(document, position);
        for (const mod of modules) {
          if (imported.has(mod)) {
            addUnique(items, moduleItem(mod), keySet);
          }
        }

        return items;
      }
    },
    '.'
  );

  context.subscriptions.push(provider);
}

function deactivate() {}

module.exports = {
  activate,
  deactivate
};
