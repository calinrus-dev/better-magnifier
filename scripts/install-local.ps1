[CmdletBinding()]
param(
    [switch]$Disable,
    [ValidateRange(1,100)][Nullable[int]]$WheelStep,
    [ValidateRange(0,2000)][Nullable[int]]$WheelFreeSpinMaxRate,
    [ValidateRange(0,150)][Nullable[int]]$Acceleration,
    [ValidateRange(0,409600)][Nullable[int]]$MaximumZoom
)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$resultPath = Join-Path $projectRoot 'build\install-result.json'
$modId = 'local@kde-pinch-zoom'
$registryPath = 'SOFTWARE\Windhawk\Engine\Mods\' + $modId
$result = @{ success = $false; action = 'install'; message = '' }
$machine = $null
$key = $null
$previous = $null
$previousSettings = $null
$previousSource = $null
$sourceDestination = $null
$modified = $false
$legacyKey = $null
$legacyDisabled = $null
function Save-Values($RegistryKey) {
    $saved = @{}
    if ($null -ne $RegistryKey) {
        foreach ($name in $RegistryKey.GetValueNames()) {
            $saved[$name] = @{ Value = $RegistryKey.GetValue($name); Kind = $RegistryKey.GetValueKind($name) }
        }
    }
    return $saved
}
function Restore-Values($RegistryKey, $Saved) {
    foreach ($name in $RegistryKey.GetValueNames()) { if (-not $Saved.ContainsKey($name)) { $RegistryKey.DeleteValue($name, $false) } }
    foreach ($name in $Saved.Keys) { $RegistryKey.SetValue($name, $Saved[$name].Value, $Saved[$name].Kind) }
}
try {
    $principal = [Security.Principal.WindowsPrincipal]::new([Security.Principal.WindowsIdentity]::GetCurrent())
    if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
        throw 'Windhawk stores machine-wide mods in HKLM. Run this script as administrator.'
    }
    $machine = [Microsoft.Win32.RegistryKey]::OpenBaseKey('LocalMachine', 'Registry64')
    if ($Disable) {
        $result.action = 'disable'
        $key = $machine.OpenSubKey($registryPath, $true)
        if ($null -eq $key) { throw 'This local mod is not installed.' }
        $key.SetValue('Disabled', 1, 'DWord')
        $key.Flush()
        $result.success = $true
        $result.message = 'Only local@kde-pinch-zoom was disabled.'
    } else {
        $sourcePath = Join-Path $projectRoot 'mods\kde-pinch-zoom.wh.cpp'
        $dllPath = Join-Path $projectRoot 'build\kde-pinch-zoom.dll'
        $hostPath = Join-Path $projectRoot 'build\pinch-zoom-host.exe'
        $manifestPath = Join-Path $projectRoot 'build\build-info.json'
        foreach ($path in @($sourcePath, $dllPath, $hostPath, $manifestPath)) {
            if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { throw "Missing file: $path. Run scripts/check.ps1 first." }
        }
        $source = Get-Content -LiteralPath $sourcePath -Raw -Encoding UTF8
        $version = [regex]::Match($source, '(?m)^// @version\s+(\S+)').Groups[1].Value
        if ($version -notmatch '^0\.[34567]\.\d+$') { throw 'This installer is scoped to versions 0.3.x through 0.7.x.' }
        $manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
        if ($manifest.version -ne $version -or $manifest.sourceSha256 -ne (Get-FileHash -LiteralPath $sourcePath -Algorithm SHA256).Hash -or
            $manifest.dllSha256 -ne (Get-FileHash -LiteralPath $dllPath -Algorithm SHA256).Hash) {
            throw 'Source and DLL do not match this build. Run scripts/check.ps1 again.'
        }
        $settingsRoot = $machine.OpenSubKey('SOFTWARE\Windhawk')
        if ($null -eq $settingsRoot) { throw 'Installed Windhawk was not found.' }
        $settingsRoot.Close()
        $dataRoot = Join-Path $env:ProgramData 'Windhawk'
        $modsPath = Join-Path $dataRoot 'Engine\Mods\64'
        $sourcesPath = Join-Path $dataRoot 'ModsSource'
        if (-not (Test-Path -LiteralPath $modsPath -PathType Container)) { throw 'Windhawk x64 mod directory was not found.' }
        $fingerprint = (Get-FileHash -LiteralPath $dllPath -Algorithm SHA256).Hash.Substring(0, 12).ToLowerInvariant()
        $libraryName = $modId + '_' + $version + '_' + $fingerprint + '.dll'
        $existing = $machine.OpenSubKey($registryPath)
        if ($null -ne $existing) {
            $previous = Save-Values $existing
            $existingSettings = $existing.OpenSubKey('Settings')
            $previousSettings = Save-Values $existingSettings
            if ($existingSettings) { $existingSettings.Close() }
            $existing.Close()
        }
        $sourceDestination = Join-Path $sourcesPath ($modId + '.wh.cpp')
        if (Test-Path -LiteralPath $sourceDestination) { $previousSource = [IO.File]::ReadAllBytes($sourceDestination) }
        # Disable only this mod while updating its files and configuration.
        $key = $machine.CreateSubKey($registryPath, $true)
        $modified = $true
        $key.SetValue('Disabled', 1, 'DWord')
        $key.Flush()
        Copy-Item -LiteralPath $dllPath -Destination (Join-Path $modsPath $libraryName) -Force
        Copy-Item -LiteralPath $sourcePath -Destination $sourceDestination -Force
        $key.SetValue('LibraryFileName', $libraryName, 'String')
        $key.SetValue('Architecture', 'amd64', 'String')
        $key.SetValue('Include', 'explorer.exe|magnify.exe', 'String')
        $key.SetValue('Exclude', '', 'String')
        $key.SetValue('Version', $version, 'String')
        $key.SetValue('LoggingEnabled', 1, 'DWord')
        $key.SetValue('DebugLoggingEnabled', 1, 'DWord')
        $key.SetValue('SettingsChangeTime', [int][DateTimeOffset]::UtcNow.ToUnixTimeSeconds(), 'DWord')
        $settings = $key.CreateSubKey('Settings', $true)
        try {
            $defaults = @{enabled=1; observeOnly=0; sensitivity=100; acceleration=35; invertPinch=0; threshold=3;
                maxZoom=0; suppressStartupFeedback=1; smoothWheel=1; winWheel=1; wheelWindowSwitch=1; windowButtons=0; disableReading=1; wheelStep=50; wheelDuration=100; wheelMaxRate=400; wheelFreeSpinMaxRate=1000; instantHorizontalSwitch=1; normalZoomInOverview=1; keyboardInterop=1; keyboardStep=0; keyboardHoldMultiplier=2; hideMagnifier=1; keepPointerStable=1; mouseFollow='edges'; edgeMargin=12;
                smoothing=0; fallbackHeight=60; debugLogging=0}
            foreach ($name in $defaults.Keys) {
                if ($null -eq $settings.GetValue($name, $null)) {
                    $kind = if ($defaults[$name] -is [string]) { 'String' } else { 'DWord' }
                    $settings.SetValue($name, $defaults[$name], $kind)
                }
            }
            if ($null -ne $WheelStep) { $settings.SetValue('wheelStep', [int]$WheelStep, 'DWord') }
            if ($null -ne $WheelFreeSpinMaxRate) { $settings.SetValue('wheelFreeSpinMaxRate', [int]$WheelFreeSpinMaxRate, 'DWord') }
            if ($null -ne $Acceleration) { $settings.SetValue('acceleration', [int]$Acceleration, 'DWord') }
            if ($null -ne $MaximumZoom) { $settings.SetValue('maxZoom', [int]$MaximumZoom, 'DWord') }
        } finally { $settings.Close() }
        # Explicitly requested merge: disable the old headless mod, keep it installed.
        $legacyKey = $machine.OpenSubKey('SOFTWARE\Windhawk\Engine\Mods\magnifier-headless', $true)
        if ($legacyKey) {
            $legacyDisabled = $legacyKey.GetValue('Disabled', 0)
            if ($null -eq $key.GetValue('LegacyHeadlessWasDisabled', $null)) { $key.SetValue('LegacyHeadlessWasDisabled', $legacyDisabled, 'DWord') }
            $legacyKey.SetValue('Disabled', 1, 'DWord')
            $legacyKey.Flush()
        }
        # Restart the native app cleanly after replacing the old window-hiding hook.
        $session = (Get-Process -Id $PID).SessionId
        Get-Process -Name Magnify -ErrorAction SilentlyContinue | Where-Object { $_.SessionId -eq $session } | ForEach-Object {
            $nativeId = $_.Id
            try { Stop-Process -Id $nativeId -ErrorAction Stop }
            catch { if (Get-Process -Id $nativeId -ErrorAction SilentlyContinue) { throw } }
        }
        & $hostPath --reset
        if ($LASTEXITCODE -ne 0) { throw 'Could not restore the desktop to 100% before enabling the mod.' }
        $key.SetValue('Disabled', 0, 'DWord')
        $key.Flush()
        $result.success = $true
        $result.message = "Installed local@kde-pinch-zoom $version with native Magnifier integration."
        $result.library = $libraryName
    }
} catch {
    $result.message = $_.Exception.Message
    if ($modified -and $key) {
        try {
            if ($null -ne $previous) {
                Restore-Values $key $previous
                $restoreSettings = $key.CreateSubKey('Settings', $true)
                try { Restore-Values $restoreSettings $previousSettings } finally { $restoreSettings.Close() }
                if ($previousSource) { [IO.File]::WriteAllBytes($sourceDestination, $previousSource) }
            } else { $key.SetValue('Disabled', 1, 'DWord') }
            if ($legacyKey -and $null -ne $legacyDisabled) { $legacyKey.SetValue('Disabled', $legacyDisabled, 'DWord') }
            $result.rollback = 'Restored the previous mod configuration.'
        } catch { $result.rollback = $_.Exception.Message }
    }
} finally {
    if ($legacyKey) { $legacyKey.Close() }
    if ($null -ne $key) { $key.Close() }
    if ($null -ne $machine) { $machine.Close() }
    [IO.File]::WriteAllText($resultPath, ($result | ConvertTo-Json -Compress), [Text.UTF8Encoding]::new($false))
}
Write-Output $result.message
if (-not $result.success) { exit 1 }
