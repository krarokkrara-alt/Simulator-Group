Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$source = Join-Path (Split-Path -Parent $PSScriptRoot) 'Installer.ps1'
$sourceHash = (Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash
$tokens = $null; $parseErrors = $null
$ast = [Management.Automation.Language.Parser]::ParseFile($source, [ref]$tokens, [ref]$parseErrors)
if ($parseErrors.Count) { throw 'Installer parse errors.' }
foreach ($name in @('Write-InstallLog', 'Invoke-Esptool')) {
    $fn = $ast.Find({ param($node) $node -is [Management.Automation.Language.FunctionDefinitionAst] -and $node.Name -eq $name }, $true)
    if (-not $fn) { throw "Actual function missing: $name" }
    . ([scriptblock]::Create($fn.Extent.Text))
}
$fixtureRoot = Join-Path $PSScriptRoot ('process-fixture-' + [Guid]::NewGuid().ToString('N'))
$script:packageRoot = $fixtureRoot
$script:logFile = $null
$script:unsafeTermination = $false
$psExecutable = Join-Path ([Environment]::GetFolderPath('System')) 'WindowsPowerShell/v1.0/powershell.exe'
$results = New-Object 'System.Collections.Generic.List[object]'
$observedProcesses = @()
try {
    New-Item -ItemType Directory -Path $fixtureRoot | Out-Null
    $script:logFile = Join-Path $fixtureRoot 'process.log'
    $helper = Join-Path $fixtureRoot 'helper.ps1'
    @'
param([string]$Mode, [string]$Evidence)
$ErrorActionPreference = 'Stop'
if ($Mode -eq 'success') { Write-Output 'FIXTURE stdout'; [Console]::Error.WriteLine('FIXTURE stderr'); exit 0 }
if ($Mode -eq 'failure') { Write-Output 'FIXTURE nonzero'; exit 7 }
if ($Mode -ne 'timeout') { exit 9 }
Set-Content -LiteralPath ($Evidence + '.parent') -Value $PID
$child = Start-Process -FilePath (Join-Path ([Environment]::GetFolderPath('System')) 'WindowsPowerShell/v1.0/powershell.exe') -ArgumentList '-NoProfile -Command "Start-Sleep -Seconds 25"' -WindowStyle Hidden -PassThru
Set-Content -LiteralPath ($Evidence + '.child') -Value $child.Id
Start-Sleep -Seconds 25
'@ | Set-Content -LiteralPath $helper -Encoding UTF8
    $watch = [Diagnostics.Stopwatch]::StartNew()
    $output = Invoke-Esptool $psExecutable @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', $helper, 'success') 5000
    if ($output -notmatch 'FIXTURE stdout' -or $output -notmatch 'FIXTURE stderr') { throw 'Success output capture failed.' }
    $results.Add([pscustomobject]@{case='success stdout and stderr'; pass=$true; elapsedMs=$watch.ElapsedMilliseconds})
    $watch.Restart(); $errorText = ''
    try { $null = Invoke-Esptool $psExecutable @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', $helper, 'failure') 5000 } catch { $errorText = $_.Exception.Message }
    if ($errorText -notmatch 'exit 7') { throw "Nonzero guard failed: $errorText" }
    $results.Add([pscustomobject]@{case='nonzero exit 7 rejected'; pass=$true; elapsedMs=$watch.ElapsedMilliseconds; error=$errorText})
    $evidence = Join-Path $fixtureRoot 'observed'
    $watch.Restart(); $errorText = ''
    try { $null = Invoke-Esptool $psExecutable @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', $helper, 'timeout', $evidence) 3000 } catch { $errorText = $_.Exception.Message }
    foreach ($suffix in @('.parent', '.child')) {
        $pidFile = $evidence + $suffix
        if (-not (Test-Path -LiteralPath $pidFile)) { throw "Missing observed process evidence: $suffix" }
        $fixturePid = [int](Get-Content -LiteralPath $pidFile -Raw)
        $observedProcesses += $fixturePid
    }
    $timeoutEvidence = [pscustomobject]@{error=$errorText; observedPids=$observedProcesses; unsafeTermination=$script:unsafeTermination; log=[string](Get-Content -LiteralPath $script:logFile -Raw)}
    $timeoutEvidence | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $PSScriptRoot 'Process-Validation.TimeoutEvidence.json') -Encoding UTF8
    foreach ($fixturePid in $observedProcesses) {
        if (Get-Process -Id $fixturePid -ErrorAction SilentlyContinue) { throw "Observed fixture process still alive: $fixturePid; Invoke error: $errorText" }
    }
    if ($errorText -notmatch 'timed out; its process tree was stopped' -or $script:unsafeTermination) { throw "Timeout/tree guard failed: $errorText" }
    $results.Add([pscustomobject]@{case='timeout observed parent and child both stopped'; pass=$true; elapsedMs=$watch.ElapsedMilliseconds; error=$errorText; observedPids=$observedProcesses; unsafeTermination=$script:unsafeTermination})
    if ((Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash -ne $sourceHash) { throw 'Installer changed during tests.' }
    [pscustomobject]@{installerSha256=$sourceHash; powershell=$PSVersionTable.PSVersion.ToString(); helperRuntime=$psExecutable; passed=$results.Count; failed=0; fixtureOnly=$true; noUi=$true; noCom=$true; actualEsptoolExecuted=$false; cases=$results} |
        ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $PSScriptRoot 'Process-Validation.Results.json') -Encoding UTF8
    Write-Host 'PASS: success, nonzero and timeout/process-tree fixtures; no observed residual processes.'
} finally {
    # Helpers have a bounded 25-second lifetime even if termination fails.
    # Do not kill by name, or delete evidence while a known helper still runs.
    $alive = @($observedProcesses | Where-Object { Get-Process -Id $_ -ErrorAction SilentlyContinue })
    if ($alive.Count -eq 0) {
        $testsRoot = [IO.Path]::GetFullPath($PSScriptRoot).TrimEnd('\') + '\'
        $resolved = [IO.Path]::GetFullPath($fixtureRoot)
        if (-not $resolved.StartsWith($testsRoot, [StringComparison]::OrdinalIgnoreCase) -or [IO.Path]::GetFileName($resolved) -notmatch '^process-fixture-[a-f0-9]{32}$') { throw 'Unsafe fixture cleanup.' }
        if (Test-Path -LiteralPath $resolved) { Remove-Item -LiteralPath $resolved -Recurse -Force }
    }
}
