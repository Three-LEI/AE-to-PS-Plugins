param([switch]$CoreOnly, [ValidateSet('All', 'Sphere', 'Cylinder')][string]$Target = 'All')
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$paths = & "$root/scripts/resolve-build-paths.ps1" -CoreOnly:$CoreOnly
$compiler = Join-Path $paths.Toolchain 'x86_64-w64-mingw32-clang++.exe'
$out = Join-Path $PSScriptRoot 'build'
New-Item -ItemType Directory -Path $out -Force | Out-Null

$flags = @('-std=c++20', '-O3', '-ffp-contract=off', '-static', '-Wall', '-Wextra', '-Wpedantic',
    '-I', "$PSScriptRoot/include", '-I', $out, '-I', "$root/native/ui/include", '-I', $paths.ImGui) + $paths.CompilerFlags
$core = "$PSScriptRoot/src/texturewrap_core.cpp"
& $compiler @flags $core "$PSScriptRoot/tests/core_tests.cpp" -o "$out/core_tests.exe"
if ($LASTEXITCODE) { throw 'Texture Wrap core build failed' }
& "$out/core_tests.exe" | Tee-Object "$out/core-results.txt"
if ($LASTEXITCODE) { throw 'Texture Wrap core checks failed' }
if ($CoreOnly) { return }

& "$root/native/ui/verify-contract.ps1"
$hostFlags = @('-DWIN32=1', '-D_WINDOWS', '-DUNICODE', '-D_UNICODE',
    '-isystem', "$($paths.Sdk)/photoshopapi/photoshop", '-isystem', "$($paths.Sdk)/photoshopapi/pica_sp")
$libs = @("$root/native/build/library_ui.lib", "$root/native/build/psui.lib",
    '-ld3d11', '-ldxgi', '-limm32', '-luser32', '-lgdi32', '-ldwmapi', '-lshell32')

$targets = if ($Target -eq 'All') { @('Sphere', 'Cylinder') } else { @($Target) }
foreach ($effect in $targets) {
    $variant = @()
    if ($effect -eq 'Cylinder') { $variant = @('-DZAYU_CYLINDER') }
    & "$root/scripts/build-legacy-resources.ps1" -Metadata "$PSScriptRoot/src/PiPL-$effect.json" `
        -Output "$out/PiPL-legacy.rc" -Sdk $paths.Sdk
    Push-Location "$PSScriptRoot/src"
    try {
        & "$($paths.Toolchain)/x86_64-w64-mingw32-windres.exe" --codepage=65001 -i TextureWrap.rc -o "$out/$effect.o" -O coff
        if ($LASTEXITCODE) { throw "$effect resources failed" }
    } finally { Pop-Location }

    & $compiler @flags @hostFlags @variant -shared $core "$PSScriptRoot/src/filter.cpp" `
        "$PSScriptRoot/src/library_dialog.cpp" "$out/$effect.o" @libs -o "$out/ZaYu$effect.8bf"
    if ($LASTEXITCODE) { throw "$effect plug-in build failed" }
    Get-FileHash -LiteralPath "$out/ZaYu$effect.8bf"
}
