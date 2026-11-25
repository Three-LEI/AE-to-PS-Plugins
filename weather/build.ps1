param([switch]$CoreOnly)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$paths = & "$root/scripts/resolve-build-paths.ps1" -CoreOnly:$CoreOnly
$compiler = Join-Path $paths.Toolchain 'x86_64-w64-mingw32-clang++.exe'
$out = Join-Path $PSScriptRoot 'build'
New-Item -ItemType Directory -Path $out -Force | Out-Null

$flags = @('-std=c++20','-O3','-ffp-contract=off','-static','-Wall','-Wextra','-Wpedantic',
    '-I',"$PSScriptRoot/include",'-I',$out,'-I',"$root/native/ui/include",'-I',$paths.ImGui) + $paths.CompilerFlags
$core = "$PSScriptRoot/src/weather_core.cpp"
& $compiler @flags $core "$PSScriptRoot/tests/core_tests.cpp" -o "$out/core_tests.exe"
if ($LASTEXITCODE) { throw 'RainSnow core build failed' }
& "$out/core_tests.exe" | Tee-Object "$out/core-results.txt"
if ($LASTEXITCODE) { throw 'RainSnow core checks failed' }
if ($CoreOnly) { return }

& "$root/native/ui/verify-contract.ps1"
$hostFlags = @('-DWIN32=1','-D_WINDOWS','-D_UNICODE','-DUNICODE',
    '-isystem',"$($paths.Sdk)/photoshopapi/photoshop",'-isystem',"$($paths.Sdk)/photoshopapi/pica_sp")
$libs = @("$root/native/build/library_ui.lib","$root/native/build/psui.lib",
    '-ld3d11','-ldxgi','-limm32','-luser32','-lgdi32','-ldwmapi','-lshell32')
& "$root/scripts/build-legacy-resources.ps1" -Metadata "$PSScriptRoot/src/PiPL.json" -Output "$out/PiPL-legacy.rc" -Sdk $paths.Sdk
Push-Location "$PSScriptRoot/src"
try {
    & "$($paths.Toolchain)/x86_64-w64-mingw32-windres.exe" --codepage=65001 -i Weather.rc -o "$out/resource.o" -O coff
    if ($LASTEXITCODE) { throw 'RainSnow resources failed' }
} finally {
    Pop-Location
}
& $compiler @flags @hostFlags -shared $core "$PSScriptRoot/src/filter.cpp" "$PSScriptRoot/src/library_dialog.cpp" "$out/resource.o" @libs -o "$out/ZaYuRainSnow.8bf"
if ($LASTEXITCODE) { throw 'RainSnow plug-in build failed' }
Get-FileHash -LiteralPath "$out/ZaYuRainSnow.8bf"
