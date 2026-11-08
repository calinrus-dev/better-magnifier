[CmdletBinding()]
param([switch]$Exercise)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$session = (Get-Process -Id $PID).SessionId
$before = @{}
Get-Process explorer,Magnify -ErrorAction SilentlyContinue | Where-Object SessionId -eq $session | ForEach-Object {
    $before[$_.Id] = @{ name=$_.ProcessName; cpu=$_.CPU; private=$_.PrivateMemorySize64; handles=$_.Handles }
}
$watch = [Diagnostics.Stopwatch]::StartNew()
$handoff = $null
if ($Exercise) {
    $benchmark = Join-Path $projectRoot 'build\native-performance.exe'
    if (-not (Test-Path -LiteralPath $benchmark)) { throw 'Run scripts/check.ps1 first.' }
    $output = & $benchmark --live
    if ($LASTEXITCODE -ne 0) { throw "Native benchmark failed ($LASTEXITCODE): $output" }
    $handoff = $output | ConvertFrom-Json
} else { Start-Sleep -Seconds 5 }
$elapsed = $watch.Elapsed.TotalSeconds
$rows = @(Get-Process explorer,Magnify -ErrorAction SilentlyContinue | Where-Object SessionId -eq $session | ForEach-Object {
    if ($before.ContainsKey($_.Id)) {
        [pscustomobject]@{ process=$_.ProcessName; cpuPercentOneCore=[Math]::Round(($_.CPU-$before[$_.Id].cpu)/$elapsed*100,2);
            workingSetMiB=[Math]::Round($_.WorkingSet64/1MB,1); privateMiB=[Math]::Round($_.PrivateMemorySize64/1MB,1);
            privateDeltaMiB=[Math]::Round(($_.PrivateMemorySize64-$before[$_.Id].private)/1MB,2);
            handleDelta=$_.Handles-$before[$_.Id].handles; threads=$_.Threads.Count }
    }
})
$result = [pscustomobject]@{ exercise=[bool]$Exercise; elapsedSeconds=[Math]::Round($elapsed,2); processes=$rows; handoff=$handoff }
$result | ConvertTo-Json -Depth 5
