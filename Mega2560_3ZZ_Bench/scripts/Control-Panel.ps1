# Windows PowerShell 5.1 / WinForms. Bench firmware only; no ECU profile assumed.
param([switch]$ValidateOnly)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing
if ($ValidateOnly) { Write-Output 'WinForms dependencies loaded; script parsed.'; return }
[System.Windows.Forms.Application]::EnableVisualStyles()
$script:serial = $null
$script:ready = $false
$script:wantsRun = $false
$script:rx = ''
$script:openAt = [datetime]::MinValue
$script:lastRx = [datetime]::MinValue
$script:lastPoll = [datetime]::MinValue
$script:lastKeepalive = [datetime]::MinValue
$script:initialized = $false
$script:appliedRpm = 0
$script:pendingRpm = 0
$script:pendingAt = [datetime]::MinValue
$script:lastTick = [datetime]::UtcNow
$script:activeSensors = @{}
$script:pendingSensors = @{}
$script:lastSensorKeepalive = [datetime]::MinValue
$script:lastRpmSend = [datetime]::MinValue
$script:syncRpm = $false
$script:pendingProfile = $false
$script:profileRequestedAt = [datetime]::MinValue

$form = New-Object System.Windows.Forms.Form
$form.Text = 'Mega 2560 | 3ZZ-FE Bench Control'
$form.Size = New-Object System.Drawing.Size(850,820)
$form.MinimumSize = New-Object System.Drawing.Size(850,820)
$form.StartPosition = 'CenterScreen'
$form.Font = New-Object System.Drawing.Font('Segoe UI',10)
$form.BackColor = [System.Drawing.Color]::FromArgb(240,244,248)

function Add-Label($text,$x,$y,$w,$h,$parent = $form) {
    $control = New-Object System.Windows.Forms.Label
    $control.Text = $text; $control.SetBounds($x,$y,$w,$h)
    $parent.Controls.Add($control); return $control
}
function Add-Button($text,$x,$y,$w,$h,$parent = $form) {
    $control = New-Object System.Windows.Forms.Button
    $control.Text = $text; $control.SetBounds($x,$y,$w,$h)
    $parent.Controls.Add($control); return $control
}
$title = Add-Label 'MEGA 2560  /  BENCH SIGNAL CONTROL' 24 18 780 30
$title.Font = New-Object System.Drawing.Font('Segoe UI',16,[System.Drawing.FontStyle]::Bold)
$warning = Add-Label 'BENCH TEST | CAM phase unverified | ECU UNVERIFIED. Never connect directly to ECU NE/G.' 24 58 780 30
$warning.ForeColor = [System.Drawing.Color]::DarkRed
$vehicle = Add-Label 'Toyota  |  Altis 3ZZ-FE  |  ECU 89661-02860  |  Test profile, ECU phase unverified' 24 90 780 26
$null = Add-Label 'Serial port / 115200 baud' 24 120 210 25
$ports = New-Object System.Windows.Forms.ComboBox
$ports.DropDownStyle = 'DropDownList'; $ports.SetBounds(24,150,155,30)
$form.Controls.Add($ports)
$refresh = Add-Button 'Refresh' 190 148 100 32
$connect = Add-Button 'Connect' 302 148 120 32
$disconnect = Add-Button 'Disconnect' 434 148 130 32
$state = Add-Label 'DISCONNECTED' 24 198 780 30
$state.Font = New-Object System.Drawing.Font('Segoe UI',13,[System.Drawing.FontStyle]::Bold)
$pins = Add-Label 'CKP D7 | CMP D2 | IGF D3 | TPS D5 | MAP D6 | MAF D8 | 0-Vcc nominal ~5V' 24 228 780 20
$null = Add-Label 'RPM (100 - 6000)' 24 248 180 24
$rpm = New-Object System.Windows.Forms.NumericUpDown
$rpm.Minimum = 100; $rpm.Maximum = 6000; $rpm.Value = 800; $rpm.Increment = 100
$rpm.SetBounds(24,280,155,32); $form.Controls.Add($rpm)
$apply = Add-Button 'Apply RPM' 190 278 120 36
$start = Add-Button 'START' 330 278 120 42
$start.BackColor = [System.Drawing.Color]::Honeydew
$stop = Add-Button 'STOP' 464 278 120 42
$stop.BackColor = [System.Drawing.Color]::MistyRose
$null = Add-Label 'Live RPM while RUN' 600 244 208 24
$rpmSlider = New-Object System.Windows.Forms.TrackBar
$rpmSlider.Minimum = 100; $rpmSlider.Maximum = 6000; $rpmSlider.Value = 800
$rpmSlider.SmallChange = 100; $rpmSlider.LargeChange = 500; $rpmSlider.TickFrequency = 1000
$rpmSlider.SetBounds(600,270,208,52); $form.Controls.Add($rpmSlider)
$loadProfile = Add-Button 'Load Toyota CKP+CMP test' 596 278 212 42
$tooltips = New-Object System.Windows.Forms.ToolTip
$tooltips.SetToolTip($start,'START enables CKP D7 + CMP D2 and IGF TEST D3 if enabled, nominal 0-Vcc (~5V). IGF OFF persists across STOP/START; reset defaults ON. NOT IGT feedback. Keepalive every second; stops after 5s without START. Actual Vpp unmeasured.')
$tooltips.SetToolTip($pins,'CKP/CMP nominal logic 0-5 V (~5 Vpp) into a high-impedance instrument. Actual amplitude must be measured; no direct ECU connection.')
$tooltips.SetToolTip($loadProfile,'Explicit bench preset: RPM 800, CAM phase 0, width 2. Loading does not START output. CAM phase is not validated for this ECU.')
$hint = Add-Label 'Connect, wait for STOP, load test preset or Apply RPM, then START. Reconnect never starts output. Measure actual Vpp with a high-impedance scope.' 24 332 780 42
$tabs = New-Object System.Windows.Forms.TabControl
$tabs.SetBounds(24,380,784,220); $form.Controls.Add($tabs)
$sensorTab = New-Object System.Windows.Forms.TabPage
$sensorTab.Text = 'TPS / MAP / MAF'
$camTab = New-Object System.Windows.Forms.TabPage
$camTab.Text = 'CAM test configuration'
$diagnosticsTab = New-Object System.Windows.Forms.TabPage
$diagnosticsTab.Text = 'Diagnostics / software counters'
$tabs.TabPages.Add($sensorTab); $tabs.TabPages.Add($camTab); $tabs.TabPages.Add($diagnosticsTab)
$igfTab = New-Object System.Windows.Forms.TabPage
$igfTab.Text = 'IGF TEST'
$tabs.TabPages.Add($igfTab)
$igfNote = Add-Label 'D3: free-running IGF TEST, 4 pulses / 720 degrees, width 5 crank degrees. NOT IGT-triggered or ECU-qualified.' 12 8 730 48 $igfTab
$igfNote.ForeColor = [System.Drawing.Color]::DarkRed
$igfOn = Add-Button 'IGF ON' 12 66 140 34 $igfTab
$igfOff = Add-Button 'IGF OFF' 166 66 140 34 $igfTab
$igfState = Add-Label 'Reset defaults ON; IGF OFF persists across STOP/START. Configuration unconfirmed. STOP/timeout ends pulses.' 12 108 730 36 $igfTab
$igfStats = Add-Label 'Software counters not read. D3 logic 0-Vcc nominal ~5V; waveform/Vpp unmeasured.' 12 148 730 40 $igfTab
$diagnostics = Add-Button 'Read software counters' 12 12 220 34 $diagnosticsTab
$statsLabel = Add-Label 'Counters not read. This does not measure physical output.' 12 62 730 66 $diagnosticsTab
$statsNote = Add-Label 'STATS reports firmware counters and pin logic only. Use a high-impedance oscilloscope to measure waveform and actual Vpp.' 12 132 730 50 $diagnosticsTab
$statsNote.ForeColor = [System.Drawing.Color]::DarkRed
$sensorWarning = Add-Label 'PWM TEST ONLY. Requires filter/buffer and voltage measurement. NOT ECU-ready.' 8 4 750 38 $sensorTab
$sensorWarning.ForeColor = [System.Drawing.Color]::DarkRed
$script:sensorInputs = @{}
$script:sensorApply = @{}
$script:sensorStatus = @{}
$sensorNames = @('TPS','MAP','MAF')
$sensorTitles = @('TPS / VTA (mV)','MAP optional / ECU unconfirmed (mV)','MAF = VG (mV)')
for ($sensorIndex = 0; $sensorIndex -lt 3; $sensorIndex++) {
    $name = $sensorNames[$sensorIndex]; $left = 8 + 254 * $sensorIndex
    $null = Add-Label $sensorTitles[$sensorIndex] $left 44 246 36 $sensorTab
    $input = New-Object System.Windows.Forms.NumericUpDown
    $input.Minimum = 0; $input.Maximum = 5000; $input.Value = 0; $input.Increment = 100
    $input.SetBounds($left,82,120,30); $sensorTab.Controls.Add($input)
    $button = Add-Button ('Apply ' + $name) ($left + 128) 80 112 34 $sensorTab
    $button.Tag = $name
    $script:sensorInputs[$name] = $input; $script:sensorApply[$name] = $button
    $script:sensorStatus[$name] = Add-Label 'Not confirmed' $left 116 246 32 $sensorTab
}
$sensorsOff = Add-Button 'SENSORS OFF' 8 150 156 32 $sensorTab
$null = Add-Label '0-5000 mV nominal PWM target; not measured voltage.' 176 152 580 32 $sensorTab
$camNote = Add-Label 'Single-pulse / 720-degree BENCH TEST only. Phase is unverified, not an ECU profile.' 8 4 750 38 $camTab
$camNote.ForeColor = [System.Drawing.Color]::DarkRed
$null = Add-Label 'Phase (0-143, each 5 crank degrees)' 8 48 320 24 $camTab
$null = Add-Label 'Width (1-12, each 5 crank degrees)' 350 48 320 24 $camTab
$camPhase = New-Object System.Windows.Forms.NumericUpDown
$camPhase.Minimum = 0; $camPhase.Maximum = 143; $camPhase.SetBounds(8,78,150,30); $camTab.Controls.Add($camPhase)
$camWidth = New-Object System.Windows.Forms.NumericUpDown
$camWidth.Minimum = 1; $camWidth.Maximum = 12; $camWidth.Value = 2; $camWidth.SetBounds(350,78,150,30); $camTab.Controls.Add($camWidth)
$camApply = Add-Button 'Apply CAM test' 8 124 170 34 $camTab
$camOff = Add-Button 'Disarm until START' 190 124 140 34 $camTab
$camState = Add-Label 'CAM configuration unconfirmed' 350 126 400 50 $camTab
$log = New-Object System.Windows.Forms.TextBox
$log.Multiline = $true; $log.ReadOnly = $true; $log.ScrollBars = 'Vertical'
$log.Font = New-Object System.Drawing.Font('Consolas',9)
$log.SetBounds(24,610,784,154)
$log.Anchor = 'Top,Bottom,Left,Right'
$form.Controls.Add($log)

function Write-Log([string]$message) {
    if ($log.TextLength -gt 24000) { $log.Text = $log.Text.Substring($log.TextLength - 16000) }
    $log.AppendText(('[' + [datetime]::Now.ToString('HH:mm:ss') + '] ' + $message + [Environment]::NewLine))
}
function Update-Controls {
    $isOpen = $null -ne $script:serial -and $script:serial.IsOpen
    $readyForInput = $script:ready -and !$script:pendingProfile
    $ports.Enabled = !$isOpen; $refresh.Enabled = !$isOpen; $connect.Enabled = !$isOpen -and $ports.Items.Count -gt 0
    $disconnect.Enabled = $isOpen; $stop.Enabled = $isOpen
    $rpm.Enabled = $readyForInput
    $rpmSlider.Enabled = $readyForInput
    $apply.Enabled = $readyForInput -and $script:pendingRpm -eq 0
    $start.Enabled = $readyForInput -and !$script:wantsRun -and $script:pendingRpm -eq 0 -and $script:appliedRpm -eq [int]$rpm.Value
    $loadProfile.Enabled = $readyForInput -and !$script:wantsRun -and $script:pendingRpm -eq 0
    $diagnostics.Enabled = $isOpen -and $readyForInput
    $igfOn.Enabled = $isOpen -and $readyForInput
    $igfOff.Enabled = $isOpen -and $readyForInput
    foreach ($name in $script:sensorInputs.Keys) {
        $script:sensorInputs[$name].Enabled = $isOpen -and $readyForInput
        $script:sensorApply[$name].Enabled = $isOpen -and $readyForInput
    }
    $sensorsOff.Enabled = $isOpen -and $readyForInput
    $camPhase.Enabled = $readyForInput -and !$script:wantsRun
    $camWidth.Enabled = $readyForInput -and !$script:wantsRun
    $camApply.Enabled = $readyForInput -and !$script:wantsRun
    $camOff.Enabled = $readyForInput -and !$script:wantsRun
}
function Clear-SensorRequests([string]$text = 'PWM OFF requested / unconfirmed') {
    $script:activeSensors.Clear(); $script:pendingSensors.Clear()
    foreach ($name in $script:sensorStatus.Keys) { $script:sensorStatus[$name].Text = $text }
}
function Close-Serial([string]$reason) {
    $script:wantsRun = $false; $script:ready = $false
    $script:pendingRpm = 0; $script:appliedRpm = 0
    $script:pendingProfile = $false
    Clear-SensorRequests 'Disconnected / unconfirmed'
    if ($null -ne $script:serial) {
        try { if ($script:serial.IsOpen) { $script:serial.WriteLine('STOP'); $script:serial.WriteLine('SENSOR OFF') } } catch { Write-Log ('STOP/OFF send error: ' + $_.Exception.Message) }
        try { $script:serial.Close(); $script:serial.Dispose() } catch { Write-Log ('Close error: ' + $_.Exception.Message) }
        $script:serial = $null
    }
    $camState.Text = 'CAM configuration unconfirmed'
    $igfState.Text = 'Disconnected; IGF configuration unconfirmed'
    $igfStats.Text = 'Software counters unavailable; physical D3 output unmeasured'
    $statsLabel.Text = 'Disconnected. Counters are software only; physical output unmeasured.'
    $state.Text = 'DISCONNECTED - ' + $reason; Write-Log $reason; Update-Controls
}
function Send-Command([string]$command) {
    if ($null -eq $script:serial -or !$script:serial.IsOpen) { throw 'Serial port is not connected.' }
    $script:serial.WriteLine($command)
}
function Send-RpmTarget {
    $script:pendingRpm = [int]$rpm.Value
    $script:pendingAt = [datetime]::UtcNow; $script:lastRpmSend = $script:pendingAt
    Send-Command ('RPM ' + $script:pendingRpm)
    Write-Log ('TX RPM ' + $script:pendingRpm)
}
function Refresh-Ports {
    $previous = [string]$ports.SelectedItem
    $ports.Items.Clear()
    foreach ($name in ([System.IO.Ports.SerialPort]::GetPortNames() | Sort-Object)) { $null = $ports.Items.Add($name) }
    if ($ports.Items.Contains($previous)) { $ports.SelectedItem = $previous }
    elseif ($ports.Items.Contains('COM4')) { $ports.SelectedItem = 'COM4' }
    elseif ($ports.Items.Count -gt 0) { $ports.SelectedIndex = 0 }
    Update-Controls
}
$refresh.Add_Click({ try { Refresh-Ports } catch { Write-Log ('ERROR: ' + $_.Exception.Message) } })
$connect.Add_Click({
    try {
        $script:serial = New-Object System.IO.Ports.SerialPort([string]$ports.SelectedItem,115200)
        $script:serial.DtrEnable = $true; $script:serial.NewLine = "`n"
        $script:serial.WriteTimeout = 250; $script:serial.ReadTimeout = 50
        $script:serial.Open()
        $script:openAt = [datetime]::UtcNow; $script:lastRx = $script:openAt
        $script:initialized = $false; $script:ready = $false; $script:wantsRun = $false
        $script:appliedRpm = 0; $script:pendingRpm = 0; $script:rx = ''
        $script:pendingProfile = $false
        $script:lastTick = $script:openAt
        Clear-SensorRequests 'Not confirmed'
        $state.Text = 'CONNECTING - waiting for board reset...'
        Write-Log ('Opened ' + $script:serial.PortName + '; output will not auto-start.'); Update-Controls
    } catch { Close-Serial ('ERROR: ' + $_.Exception.Message) }
})
$disconnect.Add_Click({ Close-Serial 'Disconnected by user (STOP sent where possible).' })
$apply.Add_Click({
    try {
        Send-RpmTarget; Update-Controls
    } catch { Close-Serial ('ERROR: ' + $_.Exception.Message) }
})
$rpm.Add_ValueChanged({
    if (!$script:syncRpm) {
        $script:syncRpm = $true
        try { $rpmSlider.Value = [int]$rpm.Value } finally { $script:syncRpm = $false }
    }
    Update-Controls
})
$rpmSlider.Add_ValueChanged({
    if (!$script:syncRpm) {
        $script:syncRpm = $true
        try { $rpm.Value = $rpmSlider.Value } finally { $script:syncRpm = $false }
    }
    Update-Controls
})
$loadProfile.Add_Click({
    try {
        Clear-SensorRequests
        $script:pendingProfile = $true; $script:profileRequestedAt = [datetime]::UtcNow
        Send-Command 'PROFILE TOYOTA_3ZZ_TEST'
        $state.Text = 'Loading Toyota bench preset; output will remain stopped'
        Write-Log 'TX PROFILE TOYOTA_3ZZ_TEST (not a validated ECU phase)'; Update-Controls
    } catch { Close-Serial ('ERROR: ' + $_.Exception.Message) }
})
$diagnostics.Add_Click({
    try { Send-Command 'STATS'; Write-Log 'TX STATS (software counters only)' }
    catch { Close-Serial ('ERROR: ' + $_.Exception.Message) }
})
$igfOn.Add_Click({
    try { Send-Command 'IGF ON'; $igfState.Text = 'IGF ON requested; waiting for ACK'; Write-Log 'TX IGF ON (free-running TEST only)' }
    catch { Close-Serial ('ERROR: ' + $_.Exception.Message) }
})
$igfOff.Add_Click({
    try { Send-Command 'IGF OFF'; $igfState.Text = 'IGF OFF requested; waiting for ACK'; Write-Log 'TX IGF OFF' }
    catch { Close-Serial ('ERROR: ' + $_.Exception.Message) }
})
foreach ($name in $script:sensorApply.Keys) {
    $script:sensorApply[$name].Add_Click({
        param($sender,$eventArgs)
        try {
            $sensor = [string]$sender.Tag
            $value = [int]$script:sensorInputs[$sensor].Value
            Send-Command ('SENSOR ' + $sensor + ' ' + $value)
            $script:pendingSensors[$sensor] = @{ Value = $value; At = [datetime]::UtcNow }
            $script:sensorStatus[$sensor].Text = 'Requested ' + $value + ' mV...'
            Write-Log ('TX SENSOR ' + $sensor + ' ' + $value)
        } catch { Close-Serial ('ERROR: ' + $_.Exception.Message) }
    })
}
$sensorsOff.Add_Click({
    try { Clear-SensorRequests; Send-Command 'SENSOR OFF'; Write-Log 'TX SENSOR OFF' }
    catch { Close-Serial ('ERROR: ' + $_.Exception.Message) }
})
$camApply.Add_Click({
    try {
        Send-Command ('CAM ' + [int]$camPhase.Value + ' ' + [int]$camWidth.Value)
        $camState.Text = 'CAM configuration requested...'; Write-Log 'TX CAM test configuration'
    } catch { Close-Serial ('ERROR: ' + $_.Exception.Message) }
})
$camOff.Add_Click({
    try { Send-Command 'CAM OFF'; $camState.Text = 'Disarmed; next START enables CMP with CKP'; Write-Log 'TX CAM OFF (next START enables CMP)' }
    catch { Close-Serial ('ERROR: ' + $_.Exception.Message) }
})
$start.Add_Click({
    try {
        $script:wantsRun = $true; $script:lastKeepalive = [datetime]::UtcNow
        Send-Command 'START'; Send-Command 'STATUS'
        $state.Text = 'START CKP + CMP requested (IGF if enabled) - waiting for RUN'; Write-Log 'TX START (CKP D7 + CMP D2 + IGF TEST D3 if enabled)'; Update-Controls
    } catch { Close-Serial ('ERROR: ' + $_.Exception.Message) }
})
$stop.Add_Click({
    $script:wantsRun = $false; $script:ready = $false; $script:pendingRpm = 0
    $script:pendingProfile = $false
    Clear-SensorRequests
    try { Send-Command 'STOP'; Send-Command 'STATUS'; $state.Text = 'STOP requested'; Write-Log 'TX STOP'; Update-Controls }
    catch { Close-Serial ('ERROR: ' + $_.Exception.Message) }
})

$timer = New-Object System.Windows.Forms.Timer
$timer.Interval = 100
$timer.Add_Tick({
    if ($null -eq $script:serial) { return }
    try {
        if (!$script:serial.IsOpen) { throw 'Port closed unexpectedly.' }
        $now = [datetime]::UtcNow
        # Do not resume keepalive automatically after sleep/UI stalls.
        if (($script:wantsRun -or $script:activeSensors.Count -gt 0 -or $script:pendingSensors.Count -gt 0) -and ($now - $script:lastTick).TotalSeconds -gt 2) {
            $script:wantsRun = $false; $script:ready = $false
            Clear-SensorRequests
            Send-Command 'STOP'; Send-Command 'STATUS'
            Write-Log 'Timer interruption: STOP sent; press START again after confirmation.'
        }
        $script:lastTick = $now
        # ReadExisting returns currently buffered characters; never ReadLine on the UI thread.
        $chunk = $script:serial.ReadExisting()
        if ($chunk.Length -gt 0) {
            $script:rx += $chunk
            if ($script:rx.Length -gt 8192) { throw 'Receive buffer limit exceeded.' }
            $processed = 0
            while ($script:rx.Contains("`n") -and $processed -lt 40) {
                $index = $script:rx.IndexOf("`n")
                $message = $script:rx.Substring(0,$index).Trim()
                $script:rx = $script:rx.Substring($index + 1); $processed++
                if (!$message) { continue }
                Write-Log ('RX ' + $message)
                if ($message -match '^(RUN|STOP) ([0-9]+) RPM; CAM (DISABLED|TEST ENABLED); ECU UNVERIFIED$') {
                    $script:lastRx = $now
                    $camState.Text = 'CAM ' + $Matches[3] + ' / phase unverified'
                    if ($script:initialized) {
                        $script:appliedRpm = [int]$Matches[2]
                        if ($Matches[1] -eq 'STOP') { $script:ready = $true; $script:wantsRun = $false; $state.Text = 'STOP confirmed | RPM ' + $Matches[2] }
                        elseif ($script:wantsRun) { $script:ready = $true; $state.Text = 'RUN confirmed | RPM ' + $Matches[2] + ' | BENCH TEST' }
                        else { $script:ready = $false; Send-Command 'STOP'; $state.Text = 'Unexpected RUN - STOP sent' }
                    }
                } elseif ($message -eq 'ACK PROFILE TOYOTA_3ZZ_TEST') {
                    if ($script:pendingProfile) {
                        $script:pendingProfile = $false; $script:ready = $false; $script:wantsRun = $false
                        $script:appliedRpm = 800; $rpm.Value = 800; $camPhase.Value = 0; $camWidth.Value = 2
                        $camState.Text = 'Toyota test phase 0 / width 2; ECU phase unverified'
                        Send-Command 'STATUS'
                    }
                } elseif ($message -match '^STATS CKP ([0-9]+) CMP ([0-9]+) D7 ([01]) D2 ([01])$') {
                    $statsLabel.Text = 'SOFTWARE ONLY: CKP count ' + $Matches[1] + ' | CMP count ' + $Matches[2] +
                        "`r`nPin logic snapshot: D7=" + $Matches[3] + ' D2=' + $Matches[4] + ' | Actual waveform/Vpp unmeasured'
                } elseif ($message -match '^IGFSTATS EDGES ([0-9]+) D3 ([01]) ENABLED ([01])$') {
                    $igfStats.Text = 'SOFTWARE ONLY: IGF edges ' + $Matches[1] + ' | D3=' + $Matches[2] + ' | enabled=' + $Matches[3] + '; actual waveform/Vpp unmeasured'
                } elseif ($message -match '^ACK IGF (ON|OFF)$') {
                    if ($Matches[1] -eq 'ON') { $igfState.Text = 'ACK IGF ON: free-running TEST enabled; pulses only during RUN' }
                    else { $igfState.Text = 'ACK IGF OFF: test disabled' }
                } elseif ($message -match '^(?:ACK )?SENSOR (TPS|MAP|MAF) ([0-9]+) mV$') {
                    # Sensor acknowledgement never changes pending/applied RPM.
                    $sensor = $Matches[1]; $value = [int]$Matches[2]
                    if ($script:pendingSensors.ContainsKey($sensor) -and $script:pendingSensors[$sensor].Value -eq $value) {
                        $script:activeSensors[$sensor] = $value; $script:pendingSensors.Remove($sensor)
                    }
                    if ($script:activeSensors.ContainsKey($sensor)) { $script:sensorStatus[$sensor].Text = 'PWM target ' + $value + ' mV (unmeasured)' }
                } elseif ($message -eq 'SENSOR OFF') {
                    Clear-SensorRequests 'PWM OFF confirmed'
                } elseif ($message -match '^SENSOR TIMEOUT (TPS|MAP|MAF)$') {
                    $sensor = $Matches[1]
                    $script:activeSensors.Remove($sensor); $script:pendingSensors.Remove($sensor)
                    $script:sensorStatus[$sensor].Text = 'PWM OFF: timeout; apply again'
                } elseif ($message -eq 'STOPPED') {
                    Clear-SensorRequests 'PWM OFF (STOP confirmed)'
                    $igfState.Text = 'STOP confirmed: IGF pulses stopped; configuration shown by STATS'
                } elseif ($message -match '^(?:ACK )?CAM OFF$') {
                    $camState.Text = 'CMP disarmed while stopped; START enables it'
                } elseif ($message -match '^(?:ACK )?CAM ([0-9]+) ([0-9]+)') {
                    $camState.Text = 'CAM test: phase ' + $Matches[1] + ', width ' + $Matches[2] + '; unverified'
                } elseif ($message -eq 'RPM SET') {
                    if ($script:pendingRpm -gt 0) { $script:appliedRpm = $script:pendingRpm; $script:pendingRpm = 0 }
                } elseif ($message -like 'STOP: *' -or $message -like 'ERROR*' -or $message -like 'Mega bench test only.*') {
                    $script:wantsRun = $false
                    $script:pendingProfile = $false
                    Clear-SensorRequests
                    if ($message -like 'Mega bench test only.*' -and $script:initialized) {
                        $script:ready = $false; $script:pendingRpm = 0; $script:appliedRpm = 0; Send-Command 'STOP'; Send-Command 'SENSOR OFF'; Send-Command 'STATUS'
                    }
                    if ($message -like 'ERROR*' -or $message -like 'STOP: *') {
                        $script:ready = $false; $script:pendingRpm = 0; Send-Command 'STOP'; Send-Command 'STATUS'
                    }
                    $state.Text = $message
                }
            }
        }
        if (!$script:initialized -and ($now - $script:openAt).TotalSeconds -ge 2.5) {
            $script:initialized = $true; $script:lastRx = $now; $script:lastPoll = $now
            Send-Command 'STOP'; Send-Command 'SENSOR OFF'; Send-Command 'STATUS'; $state.Text = 'Waiting for STOP status...'
        }
        if ($script:initialized) {
            if (($now - $script:lastRx).TotalSeconds -gt 3.5) { throw 'No valid STATUS response for 3.5s; stopping and disconnecting.' }
            if ($script:pendingRpm -gt 0 -and ($now - $script:pendingAt).TotalSeconds -gt 2) { throw 'RPM acknowledgement timeout.' }
            if ($script:pendingProfile -and ($now - $script:profileRequestedAt).TotalSeconds -gt 2) { throw 'Profile acknowledgement timeout.' }
            if ($script:ready -and $script:wantsRun -and !$script:pendingProfile -and
                $script:pendingRpm -eq 0 -and [int]$rpm.Value -ne $script:appliedRpm -and
                ($now - $script:lastRpmSend).TotalMilliseconds -ge 150) { Send-RpmTarget }
            foreach ($sensor in @($script:pendingSensors.Keys)) {
                if (($now - $script:pendingSensors[$sensor].At).TotalSeconds -gt 2) { throw ('Sensor acknowledgement timeout: ' + $sensor) }
            }
            if (($now - $script:lastSensorKeepalive).TotalSeconds -ge 1) {
                foreach ($sensor in @($script:activeSensors.Keys)) {
                    if (!$script:pendingSensors.ContainsKey($sensor)) { Send-Command ('SENSOR ' + $sensor + ' ' + $script:activeSensors[$sensor]) }
                }
                $script:lastSensorKeepalive = $now
            }
            if ($script:wantsRun -and ($now - $script:lastKeepalive).TotalSeconds -ge 1) {
                Send-Command 'START'; $script:lastKeepalive = $now
            }
            if (($now - $script:lastPoll).TotalSeconds -ge 1) { Send-Command 'STATUS'; $script:lastPoll = $now }
        }
        Update-Controls
    } catch { Close-Serial ('ERROR: ' + $_.Exception.Message) }
})
$form.Add_FormClosing({ $timer.Stop(); Close-Serial 'Control panel closing.' })
try { Refresh-Ports; $timer.Start(); $null = $form.ShowDialog() }
finally { $timer.Stop(); $timer.Dispose(); Close-Serial 'Control panel closed.'; $tooltips.Dispose(); $form.Dispose() }
