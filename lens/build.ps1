param([switch]$CoreOnly,[ValidateSet('Both','OpticsCompensation','CCLens')][string]$BuildTarget='Both')
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
& $cxx @flags "$PSScriptRoot\src\lens_core.cpp" "$PSScriptRoot\tests\core_tests.cpp" '-o' "$out\core_tests.exe"
if($LASTEXITCODE -ne 0){throw 'Lens core build failed'}
& "$out\core_tests.exe" | Tee-Object "$out\core-results.txt"
if($LASTEXITCODE -ne 0){throw 'Lens core tests failed'}
if($CoreOnly){return}
& "$project\native\ui\verify-contract.ps1"
$hostFlags=@('-DWIN32=1','-D_WINDOWS','-DUNICODE','-D_UNICODE','-isystem',"$sdk\photoshopapi\photoshop",'-isystem',"$sdk\photoshopapi\pica_sp")
$libs=@("$project\native\build\library_ui.lib","$project\native\build\psui.lib",'-ld3d11','-ldxgi','-ld3dcompiler','-limm32','-luser32','-lgdi32','-ldwmapi','-lshell32')
$targets=if($BuildTarget -eq 'Both'){@('OpticsCompensation','CCLens')}else{@($BuildTarget)}
foreach($effect in $targets){
 $variant=@();if($effect -eq 'CCLens'){$variant=@('-DLENS_CC')}
 [IO.File]::WriteAllText("$out\PiPL-$effect-host.json",[IO.File]::ReadAllText("$PSScriptRoot\src\PiPL-$effect.json",[Text.Encoding]::UTF8),[Text.Encoding]::GetEncoding(936))
 Push-Location "$PSScriptRoot\src";try{& "$tool\x86_64-w64-mingw32-windres.exe" --codepage=65001 -i "$effect.rc" -o "$out\$effect-resource.o" -O coff}finally{Pop-Location}
 if($LASTEXITCODE -ne 0){throw 'Lens resource failed'}
 & $cxx @flags @hostFlags @variant -shared "$PSScriptRoot\src\lens_core.cpp" "$PSScriptRoot\src\filter.cpp" "$PSScriptRoot\src\library_dialog.cpp" "$out\$effect-resource.o" @libs '-o' "$out\ZaYu$effect.8bf"
 if($LASTEXITCODE -ne 0){throw 'Lens plugin failed'}
 & $cxx @flags @hostFlags @variant "$PSScriptRoot\src\lens_core.cpp" "$PSScriptRoot\tests\adapter_tests.cpp" @libs '-o' "$out\$effect-adapter_tests.exe"
 if($LASTEXITCODE -ne 0){throw 'Lens adapter build failed'}
 & "$out\$effect-adapter_tests.exe" | Tee-Object "$out\$effect-adapter-results.txt"
 if($LASTEXITCODE -ne 0){throw 'Lens adapter failed'}
 & $cxx @flags @hostFlags @variant "$PSScriptRoot\src\lens_core.cpp" "$PSScriptRoot\src\library_dialog.cpp" "$PSScriptRoot\tests\library_preview.cpp" @libs '-o' "$out\$effect-dialog_preview.exe"
 if($LASTEXITCODE -ne 0){throw 'Lens UI build failed'}
 Get-FileHash -LiteralPath "$out\ZaYu$effect.8bf"
}
