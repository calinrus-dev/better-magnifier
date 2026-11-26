[CmdletBinding()]
param(
    [switch]$Disable,
    [ValidateRange(1,100)][Nullable[int]]$WheelStep,
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
$sourceChanged = $false
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
function Restore-Values($RegistryKey, $Saved, [string]$Skip = $null) {
    foreach ($name in $RegistryKey.GetValueNames()) {
        if ((!$Skip -or $name -ne $Skip) -and -not $Saved.ContainsKey($name)) { $RegistryKey.DeleteValue($name, $false) }
    }
    foreach ($name in $Saved.Keys) {
        if (!$Skip -or $name -ne $Skip) { $RegistryKey.SetValue($name, $Saved[$name].Value, $Saved[$name].Kind) }
    }
}
function Get-LoadedModLibraries([int]$Session, [string[]]$ProcessNames) {
    $processes = Get-Process -Name $ProcessNames -ErrorAction SilentlyContinue
    foreach ($process in $processes) {
        try {
            if ($process.SessionId -ne $Session) { continue }
            foreach ($module in $process.Modules) {
                if ($module.ModuleName -like ($modId + '_*.dll')) {
                    [pscustomobject]@{ Library = $module.ModuleName; Host = "$($process.ProcessName) [$($process.Id)]" }
                }
            }
        } catch {
            if (-not $process.HasExited) { throw }
        } finally {
            $process.Dispose()
        }
    }
}
function Wait-PreviousModUnloaded([int]$Session, [string[]]$ProcessNames, [string]$NewLibrary) {
    $timeout = [Diagnostics.Stopwatch]::StartNew()
    do {
        $remaining = @(Get-LoadedModLibraries $Session $ProcessNames | Where-Object Library -ne $NewLibrary)
        if ($remaining.Count -eq 0) { return }
        Start-Sleep -Milliseconds 180
    } while ($timeout.Elapsed.TotalSeconds -lt 8)
    $hostNames = ($remaining.Host | Sort-Object -Unique) -join ', '
    throw "The previous Windhawk DLL is still loaded in $hostNames. The version was not switched."
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
        $manifestPath = Join-Path $projectRoot 'build\build-info.json'
        foreach ($path in @($sourcePath, $dllPath, $manifestPath)) {
            if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { throw "Missing file: $path. Run scripts/check.ps1 first." }
        }
        $source = Get-Content -LiteralPath $sourcePath -Raw -Encoding UTF8
        $version = [regex]::Match($source, '(?m)^// @version\s+(\S+)').Groups[1].Value
        if ($version -notmatch '^(0\.[345678]\.\d+|1\.\d+\.\d+)$') { throw 'Unsupported Better Visual version.' }
        $includes = @([regex]::Matches($source, '(?m)^// @include[ \t]+([A-Za-z0-9._-]+\.exe)[ \t]*\r?$') |
            ForEach-Object { $_.Groups[1].Value })
        if ($includes -notcontains 'explorer.exe' -or $includes -notcontains 'magnify.exe') {
            throw 'Missing required Explorer or Magnifier target in the mod metadata.'
        }
        $manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
        $sourceHash = (Get-FileHash -LiteralPath $sourcePath -Algorithm SHA256).Hash
        $dllHash = (Get-FileHash -LiteralPath $dllPath -Algorithm SHA256).Hash
        if ($manifest.version -ne $version -or $manifest.sourceSha256 -ne $sourceHash -or $manifest.dllSha256 -ne $dllHash) {
            throw 'Source and DLL do not match this build. Run scripts/check.ps1 again.'
        }
        $settingsRoot = $machine.OpenSubKey('SOFTWARE\Windhawk')
        if ($null -eq $settingsRoot) { throw 'Installed Windhawk was not found.' }
        $settingsRoot.Close()
        $dataRoot = Join-Path $env:ProgramData 'Windhawk'
        $modsPath = Join-Path $dataRoot 'Engine\Mods\64'
        $sourcesPath = Join-Path $dataRoot 'ModsSource'
        if (-not (Test-Path -LiteralPath $modsPath -PathType Container)) { throw 'Windhawk x64 mod directory was not found.' }
        $fingerprint = $dllHash.Substring(0, 12).ToLowerInvariant()
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
        $installedDll = Join-Path $modsPath $libraryName
        if (-not (Test-Path -LiteralPath $installedDll -PathType Leaf)) {
            Copy-Item -LiteralPath $dllPath -Destination $installedDll
        }
        if ((Get-FileHash -LiteralPath $installedDll -Algorithm SHA256).Hash -ne $manifest.dllSha256) {
            throw 'The destination DLL does not match the checked build.'
        }
        $previousIncludes = if ($previous -and $previous.ContainsKey('Include')) { $previous.Include.Value -split '\|' }
        $processNames = @((@('explorer.exe', 'magnify.exe') + @($previousIncludes)) |
            Where-Object { $_ -match '^[A-Za-z0-9._-]+\.exe$' } |
            ForEach-Object { [IO.Path]::GetFileNameWithoutExtension($_) } | Sort-Object -Unique)
        $session = (Get-Process -Id $PID).SessionId
        $olderLibraries = @(Get-LoadedModLibraries $session $processNames | Where-Object Library -ne $libraryName)
        $key = $machine.CreateSubKey($registryPath, $true)
        $modified = $true
        if ($olderLibraries.Count -gt 0 -or $key.GetValue('LibraryFileName', '') -ne $libraryName) {
            # Let Windhawk finish unloading the previous hosts before exposing new IPC.
            $key.SetValue('Disabled', 1, 'DWord')
            $key.Flush()
            if ($olderLibraries.Count -gt 0) { Wait-PreviousModUnloaded $session $processNames $libraryName }
        }
        $sourceChanged = $true
        Copy-Item -LiteralPath $sourcePath -Destination $sourceDestination -Force
        $key.SetValue('LibraryFileName', $libraryName, 'String')
        $key.SetValue('Architecture', 'amd64', 'String')
        $key.SetValue('Include', ($includes -join '|'), 'String')
        $key.SetValue('Exclude', '', 'String')
        $key.SetValue('Version', $version, 'String')
        $key.SetValue('SettingsChangeTime', [int][DateTimeOffset]::UtcNow.ToUnixTimeSeconds(), 'DWord')
        $settings = $key.CreateSubKey('Settings', $true)
        try {
            $defaults = @{enabled=1; observeOnly=0; sensitivity=100; acceleration=35; invertPinch=0; threshold=3;
                maxZoom=0; suppressStartupFeedback=1; smoothWheel=1; winWheel=1; wheelWindowSwitch=1; windowButtons=0; disableReading=1; wheelStep=20; instantHorizontalSwitch=1; normalZoomInOverview=1; keyboardInterop=1; keyboardStep=0; keyboardHoldMultiplier=2; hideMagnifier=1; keepPointerStable=1; mouseFollow='edges'; edgeMargin=12;
                smoothing=0; fallbackHeight=60; debugLogging=0; darkLegacyWindows=1}
            foreach ($name in $defaults.Keys) {
                if ($null -eq $settings.GetValue($name, $null)) {
                    $kind = if ($defaults[$name] -is [string]) { 'String' } else { 'DWord' }
                    $settings.SetValue($name, $defaults[$name], $kind)
                }
            }
            if ($null -ne $WheelStep) { $settings.SetValue('wheelStep', [int]$WheelStep, 'DWord') }
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
        $key.SetValue('Disabled', 0, 'DWord')
        $key.Flush()
        $result.success = $true
        $result.message = "Configured local@kde-pinch-zoom $version. Windhawk manages loading the configured Windows hosts."
        $result.library = $libraryName
    }
} catch {
    $result.message = $_.Exception.Message
    if ($modified -and $key) {
        try {
            $key.SetValue('Disabled', 1, 'DWord')
            $key.Flush()
            if ($null -ne $previous) {
                Restore-Values $key $previous 'Disabled'
                $restoreSettings = $key.CreateSubKey('Settings', $true)
                try { Restore-Values $restoreSettings $previousSettings } finally { $restoreSettings.Close() }
            }
            if ($sourceChanged) {
                if ($null -ne $previousSource) {
                    [IO.File]::WriteAllBytes($sourceDestination, $previousSource)
                } elseif (Test-Path -LiteralPath $sourceDestination -PathType Leaf) {
                    Remove-Item -LiteralPath $sourceDestination
                }
            }
            if ($legacyKey -and $null -ne $legacyDisabled) {
                $legacyKey.SetValue('Disabled', $legacyDisabled, 'DWord')
                $legacyKey.Flush()
            }
            if ($null -ne $previous) {
                if ($previous.ContainsKey('Disabled')) {
                    $key.SetValue('Disabled', $previous['Disabled'].Value, $previous['Disabled'].Kind)
                } else { $key.DeleteValue('Disabled', $false) }
            }
            $key.Flush()
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
