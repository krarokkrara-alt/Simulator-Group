# Load only the three actual validation functions; never execute installer entry point.
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$source = Join-Path (Split-Path -Parent $PSScriptRoot) 'Installer.ps1'
$sourceHash = (Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash
$tokens = $null; $parseErrors = $null
$ast = [Management.Automation.Language.Parser]::ParseFile($source, [ref]$tokens, [ref]$parseErrors)
if ($parseErrors.Count) { throw 'Installer parse errors.' }
foreach ($name in @('Get-PackageFile', 'Test-Hash', 'Read-Release')) {
    $matches = @($ast.FindAll({ param($node) $node -is [Management.Automation.Language.FunctionDefinitionAst] -and $node.Name -eq $name }, $true))
    if ($matches.Count -ne 1) { throw "Expected exactly one actual function: $name" }
    . ([scriptblock]::Create($matches[0].Extent.Text))
}
$fixtureRoot = Join-Path $PSScriptRoot ('release-fixture-' + [Guid]::NewGuid().ToString('N'))
$script:packageRoot = Join-Path $fixtureRoot 'package'
$results = New-Object 'System.Collections.Generic.List[object]'
$fixtureHash = 'ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad' # Known SHA256 of abc.
function New-FixtureManifest {
    return @{
        schemaVersion = 1; releaseApproved = $true; firmwareVersion = 'FIXTURE_ONLY_NEVER_FLASH'
        chip = 'esp32'
        image = @{ kind = 'merged'; file = 'firmware/fixture.bin'; offset = '0x0'; sha256 = $fixtureHash }
        tool = @{ file = 'tools/esptool.exe'; version = '5.0.0'; sha256 = $fixtureHash }
        wifi = @{ ssid = 'FIXTURE_ONLY'; password = 'FIXTURE_ONLY'; url = 'http://192.168.4.1' }
    }
}
function Invoke-Case([string]$Name, [scriptblock]$Change, [bool]$Accept, [string]$ExpectedError = '') {
    $m = New-FixtureManifest
    $image = Join-Path $script:packageRoot 'firmware/fixture.bin'
    $tool = Join-Path $script:packageRoot 'tools/esptool.exe'
    [IO.File]::WriteAllBytes($image, [Text.Encoding]::ASCII.GetBytes('abc'))
    [IO.File]::WriteAllBytes($tool, [Text.Encoding]::ASCII.GetBytes('abc'))
    & $Change $m $image $tool
    $m | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $script:packageRoot 'manifest.json') -Encoding UTF8
    $actualAccept = $false; $errorText = ''
    try {
        $release = Read-Release
        if ($release.Manifest.firmwareVersion -ne 'FIXTURE_ONLY_NEVER_FLASH') { throw 'Fixture returned unexpected data.' }
        $actualAccept = $true
    } catch { $errorText = $_.Exception.Message }
    $passed = $actualAccept -eq $Accept -and ($Accept -or $errorText -match $ExpectedError)
    $results.Add([pscustomobject]@{ case = $Name; expectedAccept = $Accept; actualAccept = $actualAccept; pass = $passed; error = $errorText })
    if (-not $passed) { throw "Failed case ${Name}: $errorText" }
}
try {
    New-Item -ItemType Directory -Path (Join-Path $script:packageRoot 'firmware'), (Join-Path $script:packageRoot 'tools') -Force | Out-Null
    [IO.File]::WriteAllBytes((Join-Path $fixtureRoot 'outside.bin'), [Text.Encoding]::ASCII.GetBytes('abc'))
    Invoke-Case 'known-hash data fixture accepted without executing it' {} $true
    Invoke-Case 'uppercase known hash accepted' { param($m) $m.image.sha256 = $fixtureHash.ToUpperInvariant() } $true
    Invoke-Case 'unapproved refused' { param($m) $m.releaseApproved = $false } $false 'not approved'
    Invoke-Case 'string true refused' { param($m) $m.releaseApproved = 'true' } $false 'not approved'
    Invoke-Case 'numeric approval refused' { param($m) $m.releaseApproved = 1 } $false 'not approved'
    Invoke-Case 'wrong schema refused' { param($m) $m.schemaVersion = 2 } $false 'not approved'
    Invoke-Case 'missing firmware refused' { param($m) $m.image.file = 'firmware/missing.bin' } $false 'missing'
    Invoke-Case 'missing tool refused' { param($m, $image, $tool) [IO.File]::Delete($tool) } $false 'missing'
    Invoke-Case 'relative escape with existing outside file refused' { param($m) $m.image.file = '../outside.bin' } $false 'escapes package'
    Invoke-Case 'absolute existing file refused' { param($m) $m.image.file = Join-Path $fixtureRoot 'outside.bin' } $false 'must be relative'
    Invoke-Case 'firmware directory refused' { param($m) $m.image.file = 'firmware' } $false 'missing'
    Invoke-Case 'wrong image hash refused' { param($m) $m.image.sha256 = '0' * 64 } $false 'SHA256 mismatch'
    Invoke-Case 'malformed image hash refused' { param($m) $m.image.sha256 = 'bad' } $false 'real SHA256'
    Invoke-Case 'tampered image refused' { param($m, $image) [IO.File]::WriteAllBytes($image, [Text.Encoding]::ASCII.GetBytes('abd')) } $false 'SHA256 mismatch'
    Invoke-Case 'tampered tool refused' { param($m, $image, $tool) [IO.File]::WriteAllBytes($tool, [Text.Encoding]::ASCII.GetBytes('abd')) } $false 'SHA256 mismatch'
    Invoke-Case 'empty image with correct empty hash refused' { param($m, $image) [IO.File]::WriteAllBytes($image, [byte[]]@()); $m.image.sha256 = 'e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855' } $false 'image is empty'
    Invoke-Case 'application-only image refused' { param($m) $m.image.kind = 'application' } $false 'merged image'
    Invoke-Case 'wrong chip refused' { param($m) $m.chip = 'esp32s3' } $false 'ESP32 target'
    Invoke-Case 'chip case mismatch refused' { param($m) $m.chip = 'ESP32' } $false 'ESP32 target'
    Invoke-Case 'null offset refused' { param($m) $m.image.offset = $null } $false 'offset'
    Invoke-Case 'numeric offset refused' { param($m) $m.image.offset = 0 } $false 'offset'
    Invoke-Case 'decimal offset refused' { param($m) $m.image.offset = '4096' } $false 'offset'
    Invoke-Case 'offset command injection refused' { param($m) $m.image.offset = '0x0 --erase-all' } $false 'offset'
    Invoke-Case 'offset over 32 bits refused' { param($m) $m.image.offset = '0x100000000' } $false 'offset'
    Invoke-Case 'unbuilt version refused' { param($m) $m.firmwareVersion = 'NOT_BUILT' } $false 'version'
    Invoke-Case 'unpinned tool version refused' { param($m) $m.tool.version = 'latest' } $false 'pinned'
    Invoke-Case 'alternate tool path refused' { param($m) $m.tool.file = 'tools/other.exe' } $false 'pinned'
    Invoke-Case 'invalid UI URL refused' { param($m) $m.wifi.url = 'https://192.168.4.1' } $false 'Wi-Fi'
    if ((Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash -ne $sourceHash) { throw 'Installer changed during tests; rerun.' }
    [pscustomobject]@{
        installerSha256 = $sourceHash; powershell = $PSVersionTable.PSVersion.ToString()
        passed = $results.Count; failed = 0; noUi = $true; noCom = $true; noToolExecution = $true
        fixtureOnly = $true; cases = $results
    } | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $PSScriptRoot 'Release-Validation.Results.json') -Encoding UTF8
    Write-Host "PASS: $($results.Count) actual-function fixture cases; no UI, COM or executable ran."
} finally {
    # Delete only this verified, unique fixture directory below tests.
    $testsRoot = [IO.Path]::GetFullPath($PSScriptRoot).TrimEnd('\') + '\'
    $resolvedFixture = [IO.Path]::GetFullPath($fixtureRoot)
    if (-not $resolvedFixture.StartsWith($testsRoot, [StringComparison]::OrdinalIgnoreCase) -or
        [IO.Path]::GetFileName($resolvedFixture) -notmatch '^release-fixture-[a-f0-9]{32}$') { throw 'Unsafe fixture cleanup path.' }
    if (Test-Path -LiteralPath $resolvedFixture) { Remove-Item -LiteralPath $resolvedFixture -Recurse -Force }
}
