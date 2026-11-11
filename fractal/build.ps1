param([switch]$CoreOnly)
$ErrorActionPreference='Stop'
$tintRoot=Split-Path $PSScriptRoot -Parent
$tintNative=Join-Path $tintRoot 'native'
$aepsPaths=& (Join-Path (Split-Path $PSScriptRoot -Parent) "scripts\resolve-build-paths.ps1") -CoreOnly:$CoreOnly
$tintTool=$aepsPaths.Toolchain
$tintCxx=Join-Path $tintTool 'x86_64-w64-mingw32-clang++.exe'
$tintSdk=$aepsPaths.Sdk
$tintOut=Join-Path $PSScriptRoot 'build'
New-Item -ItemType Directory -Path $tintOut -Force | Out-Null
$tintFlags=@('-std=c++20','-O3','-ffp-contract=off','-Wall','-Wextra','-Wpedantic','-static','-I',"$PSScriptRoot\include",'-I',"$tintNative\ui\include")
$tintFlags+=$aepsPaths.CompilerFlags
$tintFlags+=@('-I',"$PSScriptRoot\build")
& $tintCxx @tintFlags "$PSScriptRoot\src\fractal_core.cpp" "$PSScriptRoot\tests\core_tests.cpp" '-o' "$tintOut\core_tests.exe"
if($LASTEXITCODE -ne 0){throw 'Fractal core build failed'}
Push-Location $tintOut
try { & "$tintOut\core_tests.exe" | Tee-Object "$tintOut\core-test-results.txt" } finally {Pop-Location}
if($LASTEXITCODE -ne 0){throw 'Fractal core checks failed'}
if($CoreOnly){return}
& (Join-Path $tintNative 'ui\verify-contract.ps1')
$tintFlags+=@('-I', $aepsPaths.ImGui)
if(!(Test-Path -LiteralPath "$tintNative\build\psui.lib")){throw 'Build native/build.ps1 first to create shared PSUI library'}
$tintHost=@('-DWIN32=1','-D_WINDOWS','-D_USRDLL','-DUNICODE','-D_UNICODE','-isystem',"$tintSdk\photoshopapi\photoshop",'-isystem',"$tintSdk\photoshopapi\pica_sp")
Push-Location $PSScriptRoot
try {
 [IO.File]::WriteAllText("$tintOut\PiPL-host.json",[IO.File]::ReadAllText("$PSScriptRoot\src\PiPL.json",[Text.Encoding]::UTF8),[Text.Encoding]::GetEncoding(936))
 & (Join-Path (Split-Path $PSScriptRoot -Parent) "scripts\build-legacy-resources.ps1") -Metadata "$PSScriptRoot\src\PiPL.json" -Output "$tintOut\PiPL-legacy.rc" -Sdk $aepsPaths.Sdk
 & (Join-Path $tintTool 'x86_64-w64-mingw32-windres.exe') '--codepage=65001' '-i' src/fractal.rc '-o' "$tintOut\resource.o" '-O' coff
 if($LASTEXITCODE -ne 0){throw 'Fractal resources failed'}
 & $tintCxx @tintFlags @tintHost '-shared' src/fractal_core.cpp src/filter.cpp src/library_dialog.cpp "$tintOut\resource.o" "$tintNative\build\library_ui.lib" "$tintNative\build\psui.lib" '-ld3d11' '-ldxgi' '-limm32' '-luser32' '-lgdi32' '-ldwmapi' '-lshell32' '-o' "$tintOut\ZaYuFractalNoise.8bf"
 if($LASTEXITCODE -ne 0){throw 'Fractal .8bf failed'}
 & $tintCxx @tintFlags @tintHost src/fractal_core.cpp tests/adapter_tests.cpp '-o' "$tintOut\adapter_tests.exe"
 if($LASTEXITCODE -ne 0){throw 'Fractal adapter checks build failed'}
 & "$tintOut\adapter_tests.exe" | Tee-Object "$tintOut\adapter-test-results.txt"
 if($LASTEXITCODE -ne 0){throw 'Fractal adapter checks failed'}
 & $tintCxx @tintFlags @tintHost src/fractal_core.cpp src/library_dialog.cpp tests/library_preview.cpp "$tintOut\resource.o" "$tintNative\build\library_ui.lib" "$tintNative\build\psui.lib" '-ld3d11' '-ldxgi' '-limm32' '-luser32' '-lgdi32' '-ldwmapi' '-lshell32' '-lcomdlg32' '-o' "$tintOut\dialog_preview.exe"
 if($LASTEXITCODE -ne 0){throw 'Fractal preview app failed'}
} finally {Pop-Location}
Get-FileHash -LiteralPath "$tintOut\ZaYuFractalNoise.8bf"
