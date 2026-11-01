$ErrorActionPreference = 'Stop'
if ($PSVersionTable.PSEdition -ne 'Desktop' -or $PSVersionTable.PSVersion.Major -ne 5) {
    throw 'Build with Windows PowerShell 5.1: powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\scripts\Build-ControlPanel.ps1'
}
$projectDir = Split-Path -Parent $PSScriptRoot
$sourcePath = Join-Path $projectDir 'src\ControlPanelLauncher.cs'
$outputPath = Join-Path $projectDir 'Mega3ZZ-Control.exe'
$automationAssembly = Join-Path $PSHOME 'System.Management.Automation.dll'
if (!(Test-Path -LiteralPath $automationAssembly)) { $automationAssembly = [System.Management.Automation.PowerShell].Assembly.Location }
if (!(Test-Path -LiteralPath $sourcePath)) { throw ('Missing source: ' + $sourcePath) }
if (!(Test-Path -LiteralPath $automationAssembly)) { throw ('Missing Windows PowerShell assembly: ' + $automationAssembly) }
if (Test-Path -LiteralPath $outputPath) {
    throw 'Mega3ZZ-Control.exe already exists. Close it and rename the existing executable before rebuilding.'
}
$references = @('System.dll', 'System.Core.dll', 'System.Windows.Forms.dll', 'System.Drawing.dll', $automationAssembly)
Add-Type -Path $sourcePath -OutputAssembly $outputPath -OutputType WindowsApplication -ReferencedAssemblies $references
if (!(Test-Path -LiteralPath $outputPath)) { throw 'Build did not produce Mega3ZZ-Control.exe' }
Write-Host ('Built: ' + $outputPath)
