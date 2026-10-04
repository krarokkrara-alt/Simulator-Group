param([switch]$ValidateOnly)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$script:result = 2
$script:packageRoot = $PSScriptRoot
$script:logFile = $null
$script:installBusy = $false
$script:unsafeTermination = $false

function Write-InstallLog([string]$Message) {
    $line = ('{0:u} {1}' -f [DateTime]::UtcNow, $Message)
    if ($script:logFile) { Add-Content -LiteralPath $script:logFile -Value $line -Encoding UTF8 }
}

function Get-PackageFile([string]$RelativePath) {
    if ([string]::IsNullOrWhiteSpace($RelativePath) -or [IO.Path]::IsPathRooted($RelativePath)) {
        throw 'Manifest file paths must be relative to the package.'
    }
    $root = [IO.Path]::GetFullPath($script:packageRoot).TrimEnd('\') + '\'
    $full = [IO.Path]::GetFullPath((Join-Path $root $RelativePath))
    if (-not $full.StartsWith($root, [StringComparison]::OrdinalIgnoreCase)) { throw 'File path escapes package.' }
    if (-not (Test-Path -LiteralPath $full -PathType Leaf)) { throw "Package file is missing: $RelativePath" }
    # Reject links/junctions in every path component inside the package.
    $item = Get-Item -LiteralPath $full
    while ($item.FullName.TrimEnd('\') -ne $root.TrimEnd('\')) {
        if ($item.Attributes -band [IO.FileAttributes]::ReparsePoint) { throw 'Package links are not supported.' }
        $item = Get-Item -LiteralPath (Split-Path -Parent $item.FullName)
    }
    return $full
}

function Test-Hash([string]$Path, [string]$Expected) {
    if ($Expected -notmatch '^[a-fA-F0-9]{64}$') { throw 'Manifest must contain a real SHA256 hash.' }
    $hashStream = [IO.File]::OpenRead($Path)
    $sha256 = $null
    try {
        $sha256 = [Security.Cryptography.SHA256]::Create()
        $actual = [BitConverter]::ToString($sha256.ComputeHash($hashStream)).Replace('-', '')
        if ($actual -ine $Expected) { throw "SHA256 mismatch: $Path" }
    } finally {
        $hashStream.Dispose()
        if ($sha256) { $sha256.Dispose() }
    }
}

function Read-Release {
    $manifestPath = Get-PackageFile 'manifest.json'
    $m = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
    if ($m.schemaVersion -ne 1 -or $m.releaseApproved -isnot [bool] -or -not $m.releaseApproved) {
        throw 'This package is not approved for release.'
    }
    if ($m.chip -cne 'esp32') { throw 'This installer supports only the approved ESP32 target.' }
    if ($m.image.kind -cne 'merged') { throw 'A build-verified merged image is required.' }
    if ($m.image.offset -isnot [string] -or $m.image.offset -notmatch '^0x[0-9a-fA-F]{1,8}$') {
        throw 'Exact merged image offset must be supplied by the build manifest.'
    }
    if ([string]::IsNullOrWhiteSpace($m.firmwareVersion) -or $m.firmwareVersion -eq 'NOT_BUILT') { throw 'Firmware version is missing.' }
    if ($m.tool.file -cne 'tools/esptool.exe' -or $m.tool.version -notmatch '^5\.[0-9]+\.[0-9]+$') {
        throw 'A pinned, packaged esptool v5 executable is required.'
    }
    if ($m.wifi.url -cne 'http://192.168.4.1' -or [string]::IsNullOrWhiteSpace($m.wifi.ssid) -or [string]::IsNullOrWhiteSpace($m.wifi.password)) {
        throw 'Wi-Fi connection instructions are incomplete.'
    }
    $imagePath = Get-PackageFile $m.image.file
    $toolPath = Get-PackageFile $m.tool.file
    Test-Hash $imagePath $m.image.sha256
    Test-Hash $toolPath $m.tool.sha256
    if ((Get-Item -LiteralPath $imagePath).Length -eq 0) { throw 'Firmware image is empty.' }
    return @{ Manifest = $m; Image = $imagePath; Tool = $toolPath }
}

function Invoke-Esptool([string]$ToolPath, [string[]]$ToolArguments, [int]$TimeoutMs = 180000) {
    # Arguments contain validated constants, COM names, hex offsets or a local quoted filename.
    foreach ($argument in $ToolArguments) {
        if ($argument.Contains('"') -or $argument.Contains("`r") -or $argument.Contains("`n")) { throw 'Invalid tool argument.' }
    }
    $info = New-Object Diagnostics.ProcessStartInfo
    $info.FileName = $ToolPath
    $info.Arguments = (($ToolArguments | ForEach-Object { '"' + $_ + '"' }) -join ' ')
    $info.WorkingDirectory = $script:packageRoot
    $info.UseShellExecute = $false
    $info.CreateNoWindow = $true
    $info.RedirectStandardOutput = $true
    $info.RedirectStandardError = $true
    Write-InstallLog ('esptool ' + $info.Arguments)
    $process = New-Object Diagnostics.Process
    $process.StartInfo = $info
    try {
        if (-not $process.Start()) { throw 'Could not start esptool.' }
        $stdout = $process.StandardOutput.ReadToEndAsync()
        $stderr = $process.StandardError.ReadToEndAsync()
        $timer = [Diagnostics.Stopwatch]::StartNew()
        while (-not $process.WaitForExit(100)) {
            if ('System.Windows.Forms.Application' -as [type]) { [Windows.Forms.Application]::DoEvents() }
            if ($timer.ElapsedMilliseconds -ge $TimeoutMs) {
                # Latch before attempting termination; clear only when the process tree is confirmed stopped.
                $script:unsafeTermination = $true
                # Windows PowerShell 5.1 has no Process.Kill(entireProcessTree).
                # taskkill targets only the PID observed above and its descendants.
                $killInfo = New-Object Diagnostics.ProcessStartInfo
                $killInfo.FileName = Join-Path ([Environment]::GetFolderPath('System')) 'taskkill.exe'
                $killInfo.Arguments = '/PID ' + $process.Id + ' /T /F'
                $killInfo.UseShellExecute = $false
                $killInfo.CreateNoWindow = $true
                $killInfo.RedirectStandardOutput = $true
                $killInfo.RedirectStandardError = $true
                $killer = New-Object Diagnostics.Process
                $killer.StartInfo = $killInfo
                try {
                    if (-not $killer.Start()) { throw 'Cannot start process-tree termination.' }
                    $killOut = $killer.StandardOutput.ReadToEndAsync()
                    $killErr = $killer.StandardError.ReadToEndAsync()
                    if (-not $killer.WaitForExit(10000)) { throw 'Process-tree termination did not finish.' }
                    if ($killOut.Wait(1000) -and $killErr.Wait(1000)) {
                        Write-InstallLog ('taskkill exit ' + $killer.ExitCode + ': ' + $killOut.GetAwaiter().GetResult() + $killErr.GetAwaiter().GetResult())
                    }
                    if ($killer.ExitCode -ne 0 -or -not $process.WaitForExit(5000)) {
                        throw 'Process-tree termination failed. Close the tool before retrying.'
                    }
                    $script:unsafeTermination = $false
                } finally { $killer.Dispose() }
                if ($stdout.Wait(5000) -and $stderr.Wait(5000)) {
                    Write-InstallLog ($stdout.GetAwaiter().GetResult() + "`r`n" + $stderr.GetAwaiter().GetResult())
                }
                throw 'esptool timed out; its process tree was stopped. Do not assume firmware was installed.'
            }
        }
        if (-not $stdout.Wait(5000) -or -not $stderr.Wait(5000)) { throw 'Tool output did not close in time.' }
        $output = $stdout.GetAwaiter().GetResult() + "`r`n" + $stderr.GetAwaiter().GetResult()
        Write-InstallLog $output
        Write-InstallLog ('esptool exit code: ' + $process.ExitCode)
        if ($process.ExitCode -ne 0) { throw "esptool failed (exit $($process.ExitCode)); see log." }
        return $output
    } finally { $process.Dispose() }
}

try {
    $logRoot = Join-Path $script:packageRoot 'logs'
    New-Item -ItemType Directory -Path $logRoot -Force | Out-Null
    $script:logFile = Join-Path $logRoot ('install-' + [DateTime]::Now.ToString('yyyyMMdd-HHmmss') + '-' + [Guid]::NewGuid().ToString('N') + '.log')
    Write-InstallLog 'Installer opened. No flash action has been authorized yet.'
    if ($ValidateOnly) {
        $null = Read-Release
        Write-InstallLog 'Static package validation passed. No tool executed; no board accessed.'
        exit 0
    }
    Add-Type -AssemblyName System.Windows.Forms
    Add-Type -AssemblyName System.Drawing
    $form = New-Object Windows.Forms.Form
    $form.Text = 'Mytech Product - ESP32 Installer (development)'
    $form.Size = New-Object Drawing.Size(650, 460)
    $form.StartPosition = 'CenterScreen'
    $form.Add_FormClosing({ param($sender, $eventArgs) if ($script:installBusy) { $eventArgs.Cancel = $true } })
    $heading = New-Object Windows.Forms.Label
    $heading.Text = 'เชื่อมบอร์ดด้วยสาย USB ข้อมูล เลือก COM แล้วกด Install เพื่อเขียนโปรแกรม'
    $heading.SetBounds(20, 20, 590, 45)
    $ports = New-Object Windows.Forms.ComboBox
    $ports.DropDownStyle = 'DropDownList'
    $ports.SetBounds(20, 75, 190, 30)
    $refresh = New-Object Windows.Forms.Button
    $refresh.Text = 'ค้นหา COM'
    $refresh.SetBounds(225, 75, 120, 30)
    $install = New-Object Windows.Forms.Button
    $install.Text = 'Install / ติดตั้ง'
    $install.SetBounds(365, 75, 240, 35)
    $status = New-Object Windows.Forms.TextBox
    $status.Multiline = $true
    $status.ReadOnly = $true
    $status.ScrollBars = 'Vertical'
    $status.SetBounds(20, 125, 590, 210)
    $script:packageReady = $false
    try {
        $initialRelease = Read-Release
        $script:packageReady = $true
        $status.Text = "แพ็กเกจผ่านการตรวจไฟล์เบื้องต้น รุ่น $($initialRelease.Manifest.firmwareVersion)`r`nเลือก COM และกด Install เพื่อเริ่ม`r`nLog: $script:logFile"
    } catch {
        $status.Text = "ยังไม่พร้อมติดตั้ง: $($_.Exception.Message)`r`nต้องมี firmware, manifest และเครื่องมือที่ผ่าน QC ครบ`r`nLog: $script:logFile"
    }
    $install.Enabled = $script:packageReady
    $guide = New-Object Windows.Forms.Button
    $guide.Text = 'เปิดหน้าเว็บหลังเชื่อม Wi-Fi แล้ว'
    $guide.SetBounds(20, 350, 360, 35)
    $guide.Enabled = $false
    $refreshPorts = {
        $ports.Items.Clear()
        foreach ($port in ([IO.Ports.SerialPort]::GetPortNames() | Sort-Object)) { $null = $ports.Items.Add($port) }
        if ($ports.Items.Count -gt 0) { $ports.SelectedIndex = 0 }
    }
    $refresh.Add_Click($refreshPorts)
    $guide.Add_Click({ Start-Process 'http://192.168.4.1' })
    $install.Add_Click({
        if ($script:installBusy -or $script:unsafeTermination) { return }
        $script:installBusy = $true
        $install.Enabled = $false
        $refresh.Enabled = $false
        $ports.Enabled = $false
        $guide.Enabled = $false
        $script:result = 1
        try {
            $selectedPort = [string]$ports.SelectedItem
            if ($selectedPort -notmatch '^COM[0-9]+$' -or $selectedPort -notin [IO.Ports.SerialPort]::GetPortNames()) { throw 'เลือก COM ของบอร์ดก่อนติดตั้ง' }
            $release = Read-Release
            $m = $release.Manifest
            Write-InstallLog "User clicked Install for $selectedPort; firmware $($m.firmwareVersion)."
            $status.Text = "กำลังตรวจเครื่องมือและชิปบน $selectedPort กรุณารอจนเสร็จ อย่าถอดสาย USB"
            $form.Refresh()
            $version = Invoke-Esptool $release.Tool @('version')
            if ($version -notmatch ('(?m)\besptool v' + [regex]::Escape($m.tool.version) + '(?:\s|$)')) { throw 'Bundled esptool version differs from manifest.' }
            # Explicit --chip guard refuses a different chip; flash-id itself does not write firmware.
            $null = Invoke-Esptool $release.Tool @('--chip', $m.chip, '--port', $selectedPort, 'flash-id')
            # Recheck files immediately before the write. Never add --force or erase-flash.
            Test-Hash $release.Image $m.image.sha256
            Test-Hash $release.Tool $m.tool.sha256
            $null = Invoke-Esptool $release.Tool @('--chip', $m.chip, '--port', $selectedPort, '--baud', '115200', '--before', 'default-reset', '--after', 'hard-reset', 'write-flash', $m.image.offset, $release.Image)
            Write-InstallLog 'Flash completed successfully. Hardware functionality has not been tested.'
            $status.Text = "ติดตั้งสำเร็จ`r`nเชื่อม Wi-Fi ด้วยตนเอง: $($m.wifi.ssid)`r`nรหัสผ่าน: $($m.wifi.password)`r`nเปิด $($m.wifi.url)`r`nโปรแกรมไม่สลับ Wi-Fi ให้อัตโนมัติ`r`nLog: $script:logFile"
            $guide.Enabled = $true
            $script:result = 0
        } catch {
            $script:result = 1
            $failureMessage = $_.Exception.Message
            try { Write-InstallLog ('FAILED: ' + $failureMessage) } catch { $failureMessage += '; log write failed: ' + $_.Exception.Message }
            if ($script:unsafeTermination) {
                $failureMessage += "`r`nไม่ทราบว่าเครื่องมือหยุดแล้วหรือไม่ ปิดการติดตั้งซ้ำในรอบนี้ คุณปิดหน้าต่างได้ แต่ต้องยืนยันว่าเครื่องมือและกระบวนการลูกหยุดแล้วก่อนเปิดโปรแกรมใหม่ ดูรายละเอียดใน log"
            }
            $status.Text = "ติดตั้งไม่สำเร็จ: $failureMessage`r`nLog: $script:logFile"
        } finally {
            $script:installBusy = $false
            $install.Enabled = $script:packageReady -and -not $script:unsafeTermination
            $refresh.Enabled = $true
            $ports.Enabled = $true
        }
    })
    $form.Controls.AddRange(@($heading, $ports, $refresh, $install, $status, $guide))
    & $refreshPorts
    $null = $form.ShowDialog()
    Write-InstallLog ('Installer closed. Exit code: ' + $script:result)
    exit $script:result
} catch {
    Write-InstallLog ('STARTUP/VALIDATION FAILED: ' + $_.Exception.Message)
    Write-Error $_.Exception.Message -ErrorAction Continue
    exit 1
}
