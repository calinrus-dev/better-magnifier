[CmdletBinding()]
param([switch]$Disable)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$resultPath = Join-Path $projectRoot 'build\install-result.json'
$modId = 'local@kde-pinch-zoom'
$registryPath = 'SOFTWARE\Windhawk\Engine\Mods\' + $modId
$result = @{ success = $false; action = 'install'; message = '' }
$machine = $null
$key = $null
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
        foreach ($path in @($sourcePath, $dllPath, $hostPath)) {
            if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { throw "Missing file: $path. Run scripts/check.ps1 first." }
        }
        $source = Get-Content -LiteralPath $sourcePath -Raw -Encoding UTF8
        $version = [regex]::Match($source, '(?m)^// @version\s+(\S+)').Groups[1].Value
        if ($version -ne '0.2.0') { throw 'This installer is scoped to version 0.2.0.' }
        $settingsRoot = $machine.OpenSubKey('SOFTWARE\Windhawk')
        if ($null -eq $settingsRoot) { throw 'Installed Windhawk was not found.' }
        $settingsRoot.Close()
        $dataRoot = Join-Path $env:ProgramData 'Windhawk'
        $modsPath = Join-Path $dataRoot 'Engine\Mods\64'
        $sourcesPath = Join-Path $dataRoot 'ModsSource'
        if (-not (Test-Path -LiteralPath $modsPath -PathType Container)) { throw 'Windhawk x64 mod directory was not found.' }
        $fingerprint = (Get-FileHash -LiteralPath $dllPath -Algorithm SHA256).Hash.Substring(0, 12).ToLowerInvariant()
        $libraryName = $modId + '_' + $version + '_' + $fingerprint + '.dll'
        # Disable only this mod while updating its files and configuration.
        $key = $machine.CreateSubKey($registryPath, $true)
        $key.SetValue('Disabled', 1, 'DWord')
        $key.Flush()
        Copy-Item -LiteralPath $dllPath -Destination (Join-Path $modsPath $libraryName) -Force
        Copy-Item -LiteralPath $sourcePath -Destination (Join-Path $sourcesPath ($modId + '.wh.cpp')) -Force
        $key.SetValue('LibraryFileName', $libraryName, 'String')
        $key.SetValue('Architecture', 'amd64', 'String')
        $key.SetValue('Include', 'explorer.exe', 'String')
        $key.SetValue('Exclude', '', 'String')
        $key.SetValue('Version', $version, 'String')
        $key.SetValue('LoggingEnabled', 1, 'DWord')
        $key.SetValue('DebugLoggingEnabled', 1, 'DWord')
        $key.SetValue('SettingsChangeTime', [int][DateTimeOffset]::UtcNow.ToUnixTimeSeconds(), 'DWord')
        $settings = $key.CreateSubKey('Settings', $true)
        try {
            $defaults = @{enabled=1; observeOnly=0; sensitivity=100; threshold=3; maxZoom=400; smoothing=0; fallbackHeight=60; debugLogging=1}
            foreach ($name in $defaults.Keys) { $settings.SetValue($name, $defaults[$name], 'DWord') }
        } finally { $settings.Close() }
        # The user asked us to close the old magnifier before this test.
        $session = (Get-Process -Id $PID).SessionId
        Get-Process -Name Magnify -ErrorAction SilentlyContinue | Where-Object { $_.SessionId -eq $session } | Stop-Process -ErrorAction Stop
        & $hostPath --reset
        if ($LASTEXITCODE -ne 0) { throw 'Could not restore the desktop to 100% before enabling the mod.' }
        $key.SetValue('Disabled', 0, 'DWord')
        $key.Flush()
        $result.success = $true
        $result.message = 'Installed local@kde-pinch-zoom 0.2.0. Windhawk will pick up the new configuration.'
        $result.library = $libraryName
    }
} catch {
    $result.message = $_.Exception.Message
} finally {
    if ($null -ne $key) { $key.Close() }
    if ($null -ne $machine) { $machine.Close() }
    [IO.File]::WriteAllText($resultPath, ($result | ConvertTo-Json -Compress), [Text.UTF8Encoding]::new($false))
}
Write-Output $result.message
if (-not $result.success) { exit 1 }
