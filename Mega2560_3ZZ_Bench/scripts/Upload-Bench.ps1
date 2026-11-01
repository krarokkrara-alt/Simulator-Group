param([Parameter(Mandatory = $true)][ValidatePattern('^COM[0-9]+$')][string]$Port)
$ErrorActionPreference = 'Stop'
$projectDir = Split-Path -Parent $PSScriptRoot
$cliPath = Join-Path $projectDir 'tools\arduino-cli\arduino-cli.exe'
if (!(Test-Path -LiteralPath $cliPath)) { throw 'Run Install-Arduino.ps1 first' }
$sketchPath = Join-Path $projectDir 'firmware\Mega1ZZBench'
& $cliPath compile --fqbn arduino:avr:mega $sketchPath
if ($LASTEXITCODE -ne 0) { throw 'Compilation failed; upload cancelled' }
& $cliPath upload --fqbn arduino:avr:mega --port $Port $sketchPath
if ($LASTEXITCODE -ne 0) { throw 'Upload failed' }
