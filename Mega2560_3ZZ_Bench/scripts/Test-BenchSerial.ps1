param([string]$Port = 'COM4')
$ErrorActionPreference = 'Stop'
$probe = [System.IO.Ports.SerialPort]::new($Port,115200)
$probe.DtrEnable = $true
$probe.WriteTimeout = 1000
$probe.NewLine = "`n"
function Read-Probe {
    $result = $probe.ReadExisting()
    if ($result) { Write-Host $result }
    return $result
}
function Assert-Reply([string]$Command,[string]$Expected) {
    $null = $probe.ReadExisting()
    Write-Host ('TX ' + $Command)
    $probe.WriteLine($Command)
    $reply = ''
    $deadline = [datetime]::UtcNow.AddSeconds(2)
    do {
        Start-Sleep -Milliseconds 50
        $reply += $probe.ReadExisting()
    } while (!$reply.Contains($Expected) -and [datetime]::UtcNow -lt $deadline)
    Write-Host $reply
    if (!$reply.Contains($Expected)) { throw ('Missing expected response: ' + $Expected) }
}
try {
    $probe.Open()
    Start-Sleep -Milliseconds 2500
    $null = Read-Probe
    Assert-Reply 'STOP' 'STOPPED'
    Assert-Reply 'CAM OFF' 'ACK CAM OFF'
    Assert-Reply 'RPM 800' 'RPM SET'
    Assert-Reply 'SENSOR TPS 500' 'ACK SENSOR TPS 500 mV'
    Assert-Reply 'SENSOR MAP 1000' 'ACK SENSOR MAP 1000 mV'
    Assert-Reply 'SENSOR MAF 1200' 'ACK SENSOR MAF 1200 mV'
    Assert-Reply 'SENSOR TPS 5001' 'ERROR SENSOR'
    Assert-Reply 'CAM 144 2' 'ERROR CAM'
    Assert-Reply 'CAM 0 2' 'ACK CAM 0 2'
    Assert-Reply 'START' 'BENCH TEST'
    Assert-Reply 'STATUS' 'RUN 800 RPM; CAM TEST ENABLED; ECU UNVERIFIED'
    Assert-Reply 'CAM OFF' 'ERROR STOP before changing CAM'
    Assert-Reply 'STOP' 'STOPPED'
    Assert-Reply 'CONFIG' 'CONFIG SENSOR MAF 0 mV TEST PWM'
    Assert-Reply 'CAM OFF' 'ACK CAM OFF'
    Assert-Reply 'SENSOR TPS 2500' 'ACK SENSOR TPS 2500 mV'
    Start-Sleep -Milliseconds 5200
    $timeoutReply = Read-Probe
    if (!$timeoutReply.Contains('SENSOR TIMEOUT TPS')) { throw 'Sensor timeout not observed' }
    Assert-Reply 'CONFIG' 'CONFIG SENSOR TPS 0 mV TEST PWM'
    Assert-Reply 'START' 'BENCH TEST'
    Start-Sleep -Milliseconds 5200
    $timeoutReply = Read-Probe
    if (!$timeoutReply.Contains('STOP: START timeout')) { throw 'Crank timeout not observed' }
    Assert-Reply 'STATUS' 'STOP 800 RPM; CAM DISABLED; ECU UNVERIFIED'
    Assert-Reply 'SENSOR OFF' 'SENSOR OFF'
    Assert-Reply 'PROFILE TOYOTA_3ZZ_TEST' 'ACK PROFILE TOYOTA_3ZZ_TEST'
    Assert-Reply 'CONFIG' 'CONFIG PINS CKP 7 CMP 2 TPS 5 MAP 6 MAF 8'
    Assert-Reply 'START' 'BENCH TEST'
    Start-Sleep -Milliseconds 600
    $null = $probe.ReadExisting()
    $probe.WriteLine('STATS')
    Start-Sleep -Milliseconds 100
    $statsReply = Read-Probe
    if ($statsReply -notmatch 'STATS CKP ([0-9]+) CMP ([0-9]+) D7 [01] D2 [01]') { throw 'STATS response missing' }
    if ([int]$Matches[1] -lt 100 -or [int]$Matches[2] -lt 1) { throw 'CKP/CMP software counters did not advance' }
    $beforeLive = [int]$Matches[1]
    Assert-Reply 'RPM 2500' 'RPM SET'
    Assert-Reply 'STATUS' 'RUN 2500 RPM; CAM TEST ENABLED; ECU UNVERIFIED'
    Assert-Reply 'RPM 300' 'RPM SET'
    Assert-Reply 'STATUS' 'RUN 300 RPM; CAM TEST ENABLED; ECU UNVERIFIED'
    $probe.WriteLine('STATS'); Start-Sleep -Milliseconds 100
    $liveStats = Read-Probe
    if ($liveStats -notmatch 'STATS CKP ([0-9]+) CMP ([0-9]+) D7 [01] D2 [01]') { throw 'Live STATS missing' }
    if ([int]$Matches[1] -le $beforeLive) { throw 'Live RPM reset or stalled CKP counter' }
    Assert-Reply 'STOP' 'STOPPED'
    Assert-Reply 'CAM OFF' 'ACK CAM OFF'
    Write-Host 'PASS: command acknowledgements, range checks, STOP and timeouts. Waveforms not measured.'
} finally {
    if ($probe.IsOpen) {
        try { $probe.WriteLine('STOP'); $probe.WriteLine('SENSOR OFF'); $probe.WriteLine('CAM OFF') } catch { Write-Warning $_.Exception.Message }
        $probe.Close()
    }
    $probe.Dispose()
}
