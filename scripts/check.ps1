[CmdletBinding()]
param(
    [string]$WindhawkPath = 'C:\Program Files\Windhawk',
    [switch]$Diagnose,
    [switch]$SmokeZoom
)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$buildPath = Join-Path $projectRoot 'build'
$compiler = Join-Path $WindhawkPath 'Compiler\bin\clang++.exe'
if (-not (Test-Path -LiteralPath $compiler)) {
    throw "No se encuentra el compilador de Windhawk: $compiler. Usa -WindhawkPath."
}
$iniPath = Join-Path $WindhawkPath 'windhawk.ini'
$engineEntry = Get-Content -LiteralPath $iniPath | Where-Object { $_ -match '^EnginePath=' } | Select-Object -First 1
if (-not $engineEntry) { throw 'Falta EnginePath en windhawk.ini.' }
$enginePath = Join-Path $WindhawkPath ($engineEntry -replace '^EnginePath=', '')
$library = Join-Path $enginePath '64\windhawk.lib'
if (-not (Test-Path -LiteralPath $library)) { throw "Falta la biblioteca x64: $library" }
New-Item -ItemType Directory -Path $buildPath -Force | Out-Null
$parseTokens = $null
$parseErrors = $null
[Management.Automation.Language.Parser]::ParseFile((Join-Path $PSScriptRoot 'install-local.ps1'),
    [ref]$parseTokens, [ref]$parseErrors) | Out-Null
if ($parseErrors.Count) { throw ($parseErrors | ForEach-Object { $_.Message } | Out-String) }

function Invoke-Compile([string]$Name, [string[]]$Arguments) {
    # Response files preserve C++ string macro quotes even in Windows PowerShell 5.1.
    $response = Join-Path $buildPath "$Name.rsp"
    $lines = $Arguments | ForEach-Object { '"' + ($_ -replace '\\', '/' -replace '"', '\"') + '"' }
    [IO.File]::WriteAllLines($response, $lines, [Text.UTF8Encoding]::new($false))
    & $compiler "@$response"
    if ($LASTEXITCODE -ne 0) { throw "Ha fallado la compilacion: $Name ($LASTEXITCODE)" }
}

$common = @('-target', 'x86_64-w64-mingw32', '-std=c++23', '-O2', '-static', '-Wall', '-Wextra', '-Werror',
            '-DUNICODE', '-D_UNICODE', '-DNOMINMAX', '-DWINVER=0x0A00', '-D_WIN32_WINNT=0x0A00')
$source = Join-Path $projectRoot 'mods\kde-pinch-zoom.wh.cpp'
$sourceText = Get-Content -LiteralPath $source -Raw -Encoding UTF8
$id = [regex]::Match($sourceText, '(?m)^// @id\s+(\S+)').Groups[1].Value
$version = [regex]::Match($sourceText, '(?m)^// @version\s+(\S+)').Groups[1].Value
if ($id -ne 'kde-pinch-zoom' -or -not $version) { throw 'Metadatos del mod invalidos.' }
function Invoke-Test([string]$Name, [string]$Source, [string[]]$Flags = @(), [string[]]$Arguments = @()) {
    $executable = Join-Path $buildPath "$Name.exe"
    Invoke-Compile $Name ($common + @((Join-Path $projectRoot $Source), '-o', $executable) + $Flags)
    $output = & $executable @Arguments
    if ($LASTEXITCODE -ne 0) {
        $output | Write-Host
        throw "Han fallado las pruebas de $Name ($LASTEXITCODE)."
    }
    $output | Select-Object -Last 1 | Write-Host
    $output | ForEach-Object { Write-Verbose $_ }
}

Invoke-Test 'mouse-shortcut-tests' 'tests\mouse_shortcut_tests.cpp'
Invoke-Test 'core-tests' 'tests\core_tests.cpp'
$links = @('-lhid', '-lwtsapi32', '-lshell32', '-luser32', '-ladvapi32', '-ldwmapi', '-lole32', '-lgdi32')
$winFlags = @('-municode') + $links
$hostExe = Join-Path $buildPath 'pinch-zoom-host.exe'
$modDll = Join-Path $buildPath 'kde-pinch-zoom.dll'
Invoke-Compile 'host' ($common + @('-DPINCH_ZOOM_STANDALONE', '-municode', $source, '-o', $hostExe) + $links)
Invoke-Compile 'mod' ($common + @('-shared', '-Wl,--export-all-symbols', '-DWH_MOD', ('-DWH_MOD_ID=L"' + $id + '"'),
    ('-DWH_MOD_VERSION=L"' + $version + '"'), '-include', (Join-Path $WindhawkPath 'Compiler\include\windhawk_api.h'),
    $source, $library, '-o', $modDll) + $links)
Invoke-Test 'runtime-tests' 'tests\runtime_tests.cpp' $winFlags @($modDll)
Invoke-Test 'broker-tests' 'tests\broker_tests.cpp' $winFlags
Invoke-Test 'native-adapter-tests' 'tests\native_adapter_tests.cpp' $winFlags
Invoke-Test 'native-gate-tests' 'tests\native_gate_tests.cpp' $winFlags
Invoke-Test 'launch-feedback-tests' 'tests\launch_feedback_tests.cpp' $winFlags
Invoke-Test 'keyboard-hold-tests' 'tests\keyboard_hold_tests.cpp' $winFlags
Invoke-Compile 'native-integration' ($common + @('-municode', (Join-Path $projectRoot 'tests\native_integration.cpp'),
    '-o', (Join-Path $buildPath 'native-integration.exe')) + $links)
Invoke-Compile 'native-performance' ($common + @('-municode', (Join-Path $projectRoot 'tests\native_performance.cpp'),
    '-o', (Join-Path $buildPath 'native-performance.exe')) + $links)

if ($Diagnose) {
    & $hostExe --diagnose --seconds 3
    if ($LASTEXITCODE -ne 0) { throw "Ha fallado el diagnostico ($LASTEXITCODE)." }
}
if ($SmokeZoom) {
    & $hostExe --smoke-zoom
    if ($LASTEXITCODE -ne 0) { throw "Ha fallado la prueba de ampliacion ($LASTEXITCODE)." }
}
$buildInfo = @{ version=$version; sourceSha256=(Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash;
    dllSha256=(Get-FileHash -LiteralPath (Join-Path $buildPath 'kde-pinch-zoom.dll') -Algorithm SHA256).Hash }
[IO.File]::WriteAllText((Join-Path $buildPath 'build-info.json'), ($buildInfo | ConvertTo-Json), [Text.UTF8Encoding]::new($false))
Write-Host 'Correcto: reconocedor, runtime/ABI, host de diagnostico y DLL x64 de Windhawk.'
