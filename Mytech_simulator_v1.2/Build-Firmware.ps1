# Compile-only entry point. No board access, upload, erase, Wi-Fi or release approval.
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
try {
    $espWorkspace = $PSScriptRoot
    $espCli = Join-Path $espWorkspace 'tools\arduino-cli\arduino-cli.exe'
    $espConfig = Join-Path $espWorkspace 'tools\arduino-cli-local.yaml'
    $espSketch = Join-Path $espWorkspace 'development\ESP32_WiFi_StandAlone_Test'
    $espBuild = Join-Path $espWorkspace 'build\development-v0.3.1'
    $espLog = Join-Path $espWorkspace 'recovery\compile-development-v0.3.1.log'
    if (-not (Test-Path -LiteralPath $espCli -PathType Leaf)) { throw 'Arduino CLI is missing from tools. See recovery report.' }
    if (-not (Test-Path -LiteralPath $espConfig -PathType Leaf)) { throw 'Workspace build configuration is missing.' }
    New-Item -ItemType Directory -Path $espBuild -Force | Out-Null
    Write-Host 'Compile candidate for classic ESP32 / Core 3.3.11. This does not confirm your connected chip.'
    Write-Host 'No firmware will be uploaded. No COM port will be opened.'
    & $espCli --config-file $espConfig compile --fqbn esp32:esp32:esp32 --build-path $espBuild $espSketch 2>&1 | Tee-Object -FilePath $espLog
    $espCompileExit = $LASTEXITCODE
    if ($espCompileExit -ne 0) { throw "Compile did not succeed (exit $espCompileExit). See $espLog" }
    $espApplication = Join-Path $espBuild 'ESP32_WiFi_StandAlone_Test.ino.bin'
    if (-not (Test-Path -LiteralPath $espApplication -PathType Leaf)) { throw 'Compile returned success but application binary is missing.' }
    $espHashStream = [System.IO.File]::OpenRead($espApplication)
    $espSha256 = $null
    try {
        $espSha256 = [System.Security.Cryptography.SHA256]::Create()
        $espDigest = [BitConverter]::ToString($espSha256.ComputeHash($espHashStream)).Replace('-', '').ToLowerInvariant()
        Write-Host "Application SHA256: $espDigest"
    } finally {
        $espHashStream.Dispose()
        if ($espSha256) { $espSha256.Dispose() }
    }
    Write-Host "Compile succeeded. Candidate binaries are in $espBuild"
    Write-Host 'Customer release remains locked until chip, merged image and hardware/installer QC are approved.'
    exit 0
} catch {
    Write-Error $_.Exception.Message -ErrorAction Continue
    exit 1
}
