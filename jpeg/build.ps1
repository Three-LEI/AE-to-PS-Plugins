param([switch]$CoreOnly)
$ErrorActionPreference='Stop'
$project=Split-Path $PSScriptRoot -Parent
$aepsPaths=& (Join-Path (Split-Path $PSScriptRoot -Parent) "scripts\resolve-build-paths.ps1") -CoreOnly:$CoreOnly
$tool=$aepsPaths.Toolchain
$cxx=Join-Path $tool 'x86_64-w64-mingw32-clang++.exe'
$sdk=$aepsPaths.Sdk
$out=Join-Path $PSScriptRoot 'build'
New-Item -ItemType Directory -Path $out -Force | Out-Null
$flags=@('-std=c++20','-O3','-ffp-contract=off','-static','-Wall','-Wextra','-Wpedantic','-I',"$PSScriptRoot\include",'-I',"$project\native\ui\include",'-I',"$project\displacement\include",'-I',$aepsPaths.ImGui)
$flags+=$aepsPaths.CompilerFlags
& $cxx @flags "$PSScriptRoot\src\jpeg_core.cpp" "$PSScriptRoot\tests\core_tests.cpp" '-o' "$out\core_tests.exe"
if($LASTEXITCODE -ne 0){throw 'JPEG core build failed'}
& "$out\core_tests.exe" "$project\evidence\jpeg-v01\renders" | Tee-Object "$out\core-results.txt"
if($LASTEXITCODE -ne 0){throw 'JPEG core tests failed'}
if($CoreOnly){return}
& "$project\native\ui\verify-contract.ps1"
$hostFlags=@('-DWIN32=1','-D_WINDOWS','-DUNICODE','-D_UNICODE','-isystem',"$sdk\photoshopapi\photoshop",'-isystem',"$sdk\photoshopapi\pica_sp")
$libs=@("$project\native\build\library_ui.lib","$project\native\build\psui.lib",'-ld3d11','-ldxgi','-limm32','-luser32','-lgdi32','-ldwmapi','-lshell32')
[IO.File]::WriteAllText("$out\PiPL-host.json",[IO.File]::ReadAllText("$PSScriptRoot\src\PiPL.json",[Text.Encoding]::UTF8),[Text.Encoding]::GetEncoding(936))
 & (Join-Path (Split-Path $PSScriptRoot -Parent) "scripts\build-legacy-resources.ps1") -Metadata "$PSScriptRoot\src\PiPL.json" -Output "$out\PiPL-legacy.rc" -Sdk $aepsPaths.Sdk
Push-Location "$PSScriptRoot\src";try{& "$tool\x86_64-w64-mingw32-windres.exe" --codepage=65001 -i JPEGDamage.rc -o "$out\resource.o" -O coff}finally{Pop-Location}
if($LASTEXITCODE -ne 0){throw 'JPEG resources failed'}
& $cxx @flags @hostFlags -shared "$PSScriptRoot\src\jpeg_core.cpp" "$PSScriptRoot\src\filter.cpp" "$PSScriptRoot\src\library_dialog.cpp" "$out\resource.o" @libs '-o' "$out\ZaYuJPEGDamage.8bf"
if($LASTEXITCODE -ne 0){throw 'JPEG plugin failed'}
& $cxx @flags @hostFlags "$PSScriptRoot\src\jpeg_core.cpp" "$PSScriptRoot\tests\adapter_tests.cpp" @libs '-o' "$out\adapter_tests.exe"
if($LASTEXITCODE -ne 0){throw 'JPEG adapter build failed'}
& "$out\adapter_tests.exe" | Tee-Object "$out\adapter-results.txt"
if($LASTEXITCODE -ne 0){throw 'JPEG adapter tests failed'}
& $cxx @flags @hostFlags "$PSScriptRoot\src\jpeg_core.cpp" "$PSScriptRoot\src\library_dialog.cpp" "$PSScriptRoot\tests\library_preview.cpp" @libs '-o' "$out\dialog_preview.exe"
if($LASTEXITCODE -ne 0){throw 'JPEG UI build failed'}
Get-FileHash -LiteralPath "$out\ZaYuJPEGDamage.8bf"
