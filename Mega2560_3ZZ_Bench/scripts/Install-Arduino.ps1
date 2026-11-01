$ErrorActionPreference = 'Stop'
$projectDir = Split-Path -Parent $PSScriptRoot
$toolDir = Join-Path $projectDir 'tools\arduino-cli'
New-Item -ItemType Directory -Force -Path $toolDir | Out-Null
$archivePath = Join-Path $projectDir 'tools\arduino-cli.zip'
Invoke-WebRequest 'https://downloads.arduino.cc/arduino-cli/arduino-cli_latest_Windows_64bit.zip' -OutFile $archivePath
Expand-Archive -LiteralPath $archivePath -DestinationPath $toolDir -Force
$cliPath = Join-Path $toolDir 'arduino-cli.exe'
& $cliPath version
if ($LASTEXITCODE -ne 0) { throw 'Arduino CLI failed' }
& $cliPath core update-index
if ($LASTEXITCODE -ne 0) { throw 'Index download failed' }
& $cliPath core install arduino:avr
if ($LASTEXITCODE -ne 0) { throw 'AVR core installation failed' }
& $cliPath compile --fqbn arduino:avr:mega (Join-Path $projectDir 'firmware\Mega1ZZBench')
if ($LASTEXITCODE -ne 0) { throw 'Compilation failed' }
& $cliPath board list
if ($LASTEXITCODE -ne 0) { throw 'Board enumeration failed' }
Write-Host 'Compiled bench test. ECU profile remains unverified. Upload separately.'
