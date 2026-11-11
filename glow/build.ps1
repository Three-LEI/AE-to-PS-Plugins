param([switch]$CoreOnly,[ValidateSet('Both','Glow','DeepGlow')][string]$BuildTarget='Both')
$ErrorActionPreference='Stop'
$project=Split-Path $PSScriptRoot -Parent
$aepsPaths=& (Join-Path (Split-Path $PSScriptRoot -Parent) "scripts\resolve-build-paths.ps1") -CoreOnly:$CoreOnly
$compilerRoot=$aepsPaths.Toolchain
$compiler=Join-Path $compilerRoot 'x86_64-w64-mingw32-clang++.exe'
$sdk=$aepsPaths.Sdk
$out=Join-Path $PSScriptRoot 'build'
New-Item -ItemType Directory -Force -Path $out | Out-Null
$flags=@('-std=c++20','-O3','-ffp-contract=off','-Wall','-Wextra','-Wpedantic','-static','-I',"$PSScriptRoot\include",'-I',"$project\native\ui\include",'-I',"$project\displacement\include",'-I',$aepsPaths.ImGui)
$flags+=$aepsPaths.CompilerFlags
$flags+=@('-I',"$PSScriptRoot\build")
& $compiler @flags "$PSScriptRoot\src\glow_core.cpp" "$PSScriptRoot\tests\core_tests.cpp" "$PSScriptRoot\tests\strength_tests.cpp" -o "$out\core_tests.exe"
if($LASTEXITCODE -ne 0){throw 'Glow core build failed'}
& "$out\core_tests.exe" | Tee-Object "$out\core-test-results.txt"
if($LASTEXITCODE -ne 0){throw 'Glow core tests failed'}
if($CoreOnly){return}
& "$project\native\ui\verify-contract.ps1"
$hostFlags=@('-DWIN32=1','-D_WINDOWS','-DUNICODE','-D_UNICODE','-isystem',"$sdk\photoshopapi\photoshop",'-isystem',"$sdk\photoshopapi\pica_sp")
$libraries=@("$project\native\build\library_ui.lib","$project\native\build\psui.lib",'-ld3d11','-ldxgi','-limm32','-luser32','-lgdi32','-ldwmapi','-lshell32')
$effectsToBuild=if($BuildTarget -eq 'Both'){@('Glow','DeepGlow')}else{@($BuildTarget)}
foreach($effect in $effectsToBuild){
 $effectFlags=@();if($effect -eq 'DeepGlow'){$effectFlags=@('-DZAYU_DEEP_GLOW')}
 [IO.File]::WriteAllText("$out\PiPL-$effect-host.json",[IO.File]::ReadAllText("$PSScriptRoot\src\PiPL-$effect.json",[Text.Encoding]::UTF8),[Text.Encoding]::GetEncoding(936))
 & (Join-Path (Split-Path $PSScriptRoot -Parent) "scripts\build-legacy-resources.ps1") -Metadata "$PSScriptRoot\src\PiPL-$effect.json" -Output "$out\PiPL-$effect-legacy.rc" -Sdk $aepsPaths.Sdk
 Push-Location "$PSScriptRoot\src"
 try{& (Join-Path $compilerRoot 'x86_64-w64-mingw32-windres.exe') --codepage=65001 -i "$effect.rc" -o "$out\$effect-resource.o" -O coff}finally{Pop-Location}
 if($LASTEXITCODE -ne 0){throw "$effect resource failed"}
 & $compiler @flags @hostFlags @effectFlags -shared "$PSScriptRoot\src\glow_core.cpp" "$PSScriptRoot\src\filter.cpp" "$PSScriptRoot\src\library_dialog.cpp" "$out\$effect-resource.o" @libraries -o "$out\ZaYu$effect.8bf"
 if($LASTEXITCODE -ne 0){throw "$effect plugin failed"}
 & $compiler @flags @hostFlags @effectFlags "$PSScriptRoot\src\glow_core.cpp" "$PSScriptRoot\tests\adapter_tests.cpp" @libraries -o "$out\$effect-adapter_tests.exe"
 if($LASTEXITCODE -ne 0){throw "$effect adapter build failed"}
 & "$out\$effect-adapter_tests.exe" | Tee-Object "$out\$effect-adapter-test-results.txt"
 if($LASTEXITCODE -ne 0){throw "$effect adapter tests failed"}
 & $compiler @flags @hostFlags @effectFlags "$PSScriptRoot\src\glow_core.cpp" "$PSScriptRoot\src\library_dialog.cpp" "$PSScriptRoot\tests\library_preview.cpp" @libraries -o "$out\$effect-dialog_preview.exe"
 if($LASTEXITCODE -ne 0){throw "$effect preview harness failed"}
 Get-FileHash -LiteralPath "$out\ZaYu$effect.8bf"
}
