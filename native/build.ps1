param([switch]$CoreOnly)
$ErrorActionPreference='Stop'
$root=$PSScriptRoot
$aepsPaths=& (Join-Path (Split-Path $PSScriptRoot -Parent) "scripts\resolve-build-paths.ps1") -CoreOnly:$CoreOnly
$toolBin=$aepsPaths.Toolchain
$cxx=Join-Path $toolBin 'x86_64-w64-mingw32-clang++.exe'
$sdk=$aepsPaths.Sdk
New-Item -ItemType Directory -Force -Path (Join-Path $root 'build') | Out-Null
$flags=@('-std=c++20','-O3','-ffp-contract=off','-Wall','-Wextra','-Wpedantic','-static','-I',"$root\include",'-I',"$root\ui\include")
$flags+=$aepsPaths.CompilerFlags
$flags+=@('-I',"$PSScriptRoot\build")
& $cxx @flags "$root\src\tint_core.cpp" "$root\tests\core_tests.cpp" '-o' "$root\build\core_tests.exe"
if($LASTEXITCODE -ne 0){throw 'C++ core compilation failed'}
& "$root\build\core_tests.exe" | Tee-Object -FilePath "$root\build\core-test-results.txt"
if($LASTEXITCODE -ne 0){throw 'C++ core tests failed'}
if($CoreOnly){return}
& (Join-Path $root 'ui\verify-contract.ps1')
& (Join-Path $root 'build-library-ui.ps1')
if($LASTEXITCODE -ne 0){throw 'UI library failed'}
& "$root\tests\build-scrub-tests.ps1" -SkipLibraryBuild
if($LASTEXITCODE -ne 0){throw 'Numeric scrub tests failed'}
$flags+=@('-I', $aepsPaths.ImGui)
& $cxx @flags "$root\tests\metrics_tests.cpp" '-o' "$root\build\metrics_tests.exe"
if($LASTEXITCODE -ne 0){throw 'UI metrics tests failed to build'}
& "$root\build\metrics_tests.exe" | Tee-Object "$root\build\metrics-test-results.txt"
if($LASTEXITCODE -ne 0){throw 'UI metrics tests failed'}
if(!(Test-Path -LiteralPath "$sdk\photoshopapi\photoshop\PIFilter.h")){throw 'Official Photoshop C++ SDK headers are missing'}
$resourceCompiler=Join-Path $toolBin 'x86_64-w64-mingw32-windres.exe'
Push-Location $root
try {
    # PiPL menu strings use the Windows legacy host code page. Keep sources
    # UTF-8 and transcode only the embedded host resource; dialogs stay Unicode.
    [IO.File]::WriteAllText("$root\build\TintPiPL-host.json",[IO.File]::ReadAllText("$root\src\TintPiPL.json",[Text.Encoding]::UTF8),[Text.Encoding]::GetEncoding(936))
 & (Join-Path (Split-Path $PSScriptRoot -Parent) "scripts\build-legacy-resources.ps1") -Metadata "$root\src\TintPiPL.json" -Output "$root\build\TintPiPL-legacy.rc" -Sdk $aepsPaths.Sdk
    & $resourceCompiler '--codepage=65001' '-i' "$root\src\tint.rc" '-o' "$root\build\tint-resource.o" '-O' 'coff'
    if($LASTEXITCODE -ne 0){throw 'Resource compilation failed'}
    $hostFlags=@('-DWIN32=1','-D_WINDOWS','-D_USRDLL','-DUNICODE','-D_UNICODE','-isystem',"$sdk\photoshopapi\photoshop",'-isystem',"$sdk\photoshopapi\pica_sp")
    foreach($unit in @('widgets','photoshop_host','proxy','host_layers','preview_background','scroll_panel','combo','paint')) {
        & $cxx @flags @hostFlags '-c' "$root\ui\src\$unit.cpp" '-o' "$root\build\$unit.obj"
        if($LASTEXITCODE -ne 0){throw "PSUI compilation failed: $unit"}
    }
    & (Join-Path $toolBin 'llvm-ar.exe') 'rcs' "$root\build\psui.lib" "$root\build\widgets.obj" "$root\build\photoshop_host.obj" "$root\build\proxy.obj" "$root\build\host_layers.obj" "$root\build\preview_background.obj" "$root\build\scroll_panel.obj" "$root\build\combo.obj" "$root\build\paint.obj"
    if($LASTEXITCODE -ne 0){throw 'PSUI static library failed'}
    & $cxx @flags @hostFlags '-shared' "$root\src\tint_core.cpp" "$root\src\photoshop_filter.cpp" "$root\src\tint_library_dialog.cpp" "$root\build\tint-resource.o" "$root\build\library_ui.lib" "$root\build\psui.lib" '-ld3d11' '-ldxgi' '-limm32' '-luser32' '-lgdi32' '-ldwmapi' '-lshell32' '-o' "$root\build\ZaYuTint.8bf"
    if($LASTEXITCODE -ne 0){throw 'Photoshop .8bf compilation failed'}
    & $cxx @flags @hostFlags "$root\src\tint_core.cpp" "$root\tests\adapter_tests.cpp" '-o' "$root\build\adapter_tests.exe"
    if($LASTEXITCODE -ne 0){throw 'Adapter test compilation failed'}
    & "$root\build\adapter_tests.exe" | Tee-Object -FilePath "$root\build\adapter-test-results.txt"
    if($LASTEXITCODE -ne 0){throw 'Adapter tests failed'}
    & $cxx @flags @hostFlags "$root\tests\ui_tests.cpp" "$root\build\library_ui.lib" "$root\build\psui.lib" '-ld3d11' '-ldxgi' '-limm32' '-luser32' '-lgdi32' '-ldwmapi' '-lshell32' '-o' "$root\build\ui_tests.exe"
    if($LASTEXITCODE -ne 0){throw 'UI test compilation failed'}
    & "$root\build\ui_tests.exe" | Tee-Object -FilePath "$root\build\ui-test-results.txt"
    if($LASTEXITCODE -ne 0){throw 'UI regression tests failed'}
    & $cxx @flags @hostFlags "$root\src\tint_core.cpp" "$root\src\tint_library_dialog.cpp" "$root\tests\library_preview.cpp" "$root\build\tint-resource.o" "$root\build\library_ui.lib" "$root\build\psui.lib" '-ld3d11' '-ldxgi' '-limm32' '-luser32' '-lgdi32' '-ldwmapi' '-lshell32' '-lcomdlg32' '-o' "$root\build\dialog_preview.exe"
    if($LASTEXITCODE -ne 0){throw 'UI preview compilation failed'}
} finally {Pop-Location}
Get-FileHash -LiteralPath "$root\build\ZaYuTint.8bf" -Algorithm SHA256
