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
    Write-Host 'PASS: command acknowledgements, range checks, STOP and timeouts. Waveforms not measured.'
} finally {
    if ($probe.IsOpen) {
        try { $probe.WriteLine('STOP'); $probe.WriteLine('SENSOR OFF'); $probe.WriteLine('CAM OFF') } catch { Write-Warning $_.Exception.Message }
        $probe.Close()
    }
    $probe.Dispose()
}
