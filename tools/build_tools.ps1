$ErrorActionPreference = 'Stop'

$toolsDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$src = Join-Path $toolsDir 'gen_valenscript_snippets.cpp'
$out = Join-Path $toolsDir 'gen_valenscript_snippets.exe'

if (!(Test-Path $src)) {
  throw "Missing source: $src"
}

$needsBuild = $true
if (Test-Path $out) {
  $needsBuild = ((Get-Item $src).LastWriteTime -gt (Get-Item $out).LastWriteTime)
}

if ($needsBuild) {
  & g++ -std=c++17 -O2 -o $out $src
  if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
  Write-Host "Built $out"
} else {
  Write-Host "Up to date: $out"
}

