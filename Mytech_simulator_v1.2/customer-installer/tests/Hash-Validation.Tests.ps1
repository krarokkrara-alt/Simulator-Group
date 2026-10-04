# Exercise the actual installer function without running UI or opening a port.
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$source = Join-Path (Split-Path -Parent $PSScriptRoot) 'Installer.ps1'
$tokens = $null; $parseErrors = $null
$ast = [Management.Automation.Language.Parser]::ParseFile($source, [ref]$tokens, [ref]$parseErrors)
if ($parseErrors.Count) { throw 'Installer parse errors.' }
$fn = $ast.Find({ param($node) $node -is [Management.Automation.Language.FunctionDefinitionAst] -and $node.Name -eq 'Test-Hash' }, $true)
if (-not $fn) { throw 'Test-Hash not found.' }
. ([scriptblock]::Create($fn.Extent.Text))
$fixture = Join-Path $PSScriptRoot 'hash-fixture.tmp'
try {
    [IO.File]::WriteAllBytes($fixture, [Text.Encoding]::ASCII.GetBytes('abc'))
    $expected = 'ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad'
    Test-Hash $fixture $expected
    Test-Hash $fixture $expected.ToUpperInvariant()
    foreach ($bad in @('bad', ('0' * 64))) {
        $rejected = $false
        try { Test-Hash $fixture $bad } catch { $rejected = $true }
        if (-not $rejected) { throw 'Invalid hash was accepted.' }
    }
    [IO.File]::WriteAllBytes($fixture, [Text.Encoding]::ASCII.GetBytes('abd'))
    $rejected = $false
    try { Test-Hash $fixture $expected } catch { $rejected = $true }
    if (-not $rejected) { throw 'Modified file was accepted.' }
    Write-Host 'PASS: correct/uppercase SHA256 accepted; malformed/wrong/tampered hashes rejected.'
} finally {
    if ([IO.File]::Exists($fixture)) { [IO.File]::Delete($fixture) }
}
