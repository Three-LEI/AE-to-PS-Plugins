param([string]$OutputRoot='')
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$paths=& "$root/scripts/resolve-build-paths.ps1"
$out=if($OutputRoot){[IO.Path]::GetFullPath($OutputRoot)}else{"$root/evidence/preview-quality-v01"}
New-Item -ItemType Directory -Path $out -Force|Out-Null
$modules=@('glow','chromatic','vector','lens','jpeg','surface','creative','roughen','lightfx')
$flags=@('-std=c++20','-O3','-ffp-contract=off','-static','-DWIN32=1','-D_WINDOWS','-DUNICODE','-D_UNICODE','-I',"$root/native/ui/include",'-I',$paths.ImGui,'-isystem',"$($paths.Sdk)/photoshopapi/photoshop",'-isystem',"$($paths.Sdk)/photoshopapi/pica_sp")+$paths.CompilerFlags
$units=@();foreach($module in $modules){$flags+=@('-I',"$root/$module/include");$units+=(Get-ChildItem "$root/$module/src/*_core.cpp").FullName}
& "$($paths.Toolchain)/x86_64-w64-mingw32-clang++.exe" @flags "$PSScriptRoot/adaptive-tests.cpp" @units -o "$out/adaptive-tests.exe"
if($LASTEXITCODE){throw 'Shared adaptive tests compilation failed'}
& "$out/adaptive-tests.exe" $out | Tee-Object "$out/shared-tests.txt"
if($LASTEXITCODE){throw 'Shared adaptive tests failed'}
