param([switch]$CoreOnly)
$ErrorActionPreference='Stop'
$project=Split-Path $PSScriptRoot -Parent
$aepsPaths=& (Join-Path (Split-Path $PSScriptRoot -Parent) "scripts\resolve-build-paths.ps1") -CoreOnly:$CoreOnly
$tool=$aepsPaths.Toolchain
$compiler=Join-Path $tool 'x86_64-w64-mingw32-clang++.exe'
$sdk=$aepsPaths.Sdk
$out=Join-Path $PSScriptRoot 'build'
New-Item -ItemType Directory -Path $out -Force | Out-Null
$flags=@('-std=c++20','-O3','-ffp-contract=off','-Wall','-Wextra','-Wpedantic','-static','-I',"$PSScriptRoot\include",'-I',"$project\native\ui\include",'-I',"$project\displacement\include",'-I',$aepsPaths.ImGui)
$flags+=$aepsPaths.CompilerFlags
& $compiler @flags "$PSScriptRoot\src\cell_core.cpp" "$PSScriptRoot\tests\core_tests.cpp" -o "$out\core_tests.exe"
if($LASTEXITCODE -ne 0){throw 'Cell core build failed'}
& "$out\core_tests.exe" | Tee-Object "$out\core-test-results.txt"
if($LASTEXITCODE -ne 0){throw 'Cell core tests failed'}
& $compiler @flags "$PSScriptRoot\src\cell_core.cpp" "$PSScriptRoot\tests\reference_render.cpp" -o "$out\reference_render.exe"
if($LASTEXITCODE -ne 0){throw 'Cell reference renderer build failed'}
if($CoreOnly){return}
& "$project\native\ui\verify-contract.ps1"
$hostFlags=@('-DWIN32=1','-D_WINDOWS','-DUNICODE','-D_UNICODE','-isystem',"$sdk\photoshopapi\photoshop",'-isystem',"$sdk\photoshopapi\pica_sp")
$libraries=@("$project\native\build\library_ui.lib","$project\native\build\psui.lib",'-ld3d11','-ldxgi','-ld3dcompiler','-limm32','-luser32','-lgdi32','-ldwmapi','-lshell32')
[IO.File]::WriteAllText("$out\PiPL-host.json",[IO.File]::ReadAllText("$PSScriptRoot\src\PiPL.json",[Text.Encoding]::UTF8),[Text.Encoding]::GetEncoding(936))
Push-Location "$PSScriptRoot\src"
try {& "$tool\x86_64-w64-mingw32-windres.exe" --codepage=65001 -i cell.rc -o "$out\cell-resource.o" -O coff} finally {Pop-Location}
if($LASTEXITCODE -ne 0){throw 'Cell resources failed'}
& $compiler @flags @hostFlags -shared "$PSScriptRoot\src\cell_core.cpp" "$PSScriptRoot\src\filter.cpp" "$PSScriptRoot\src\library_dialog.cpp" "$out\cell-resource.o" @libraries -o "$out\ZaYuCellPattern.8bf"
if($LASTEXITCODE -ne 0){throw 'Cell plugin build failed'}
& $compiler @flags @hostFlags "$PSScriptRoot\src\cell_core.cpp" "$PSScriptRoot\tests\adapter_tests.cpp" @libraries -o "$out\adapter_tests.exe"
if($LASTEXITCODE -ne 0){throw 'Cell adapter build failed'}
& "$out\adapter_tests.exe" | Tee-Object "$out\adapter-test-results.txt"
if($LASTEXITCODE -ne 0){throw 'Cell adapter tests failed'}
& $compiler @flags @hostFlags "$PSScriptRoot\src\cell_core.cpp" "$PSScriptRoot\src\library_dialog.cpp" "$PSScriptRoot\tests\library_preview.cpp" @libraries -o "$out\dialog_preview.exe"
if($LASTEXITCODE -ne 0){throw 'Cell production UI harness failed'}
Get-FileHash -LiteralPath "$out\ZaYuCellPattern.8bf"
