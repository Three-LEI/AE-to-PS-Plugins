param([string]$Output='docs/gallery-parameters.json')
$ErrorActionPreference='Stop';$root=Split-Path $PSScriptRoot -Parent;$paths=& "$root/scripts/resolve-build-paths.ps1"
$target=[IO.Path]::GetFullPath($Output);$folder=Split-Path $target -Parent;New-Item -ItemType Directory -Path $folder -Force|Out-Null
$flags=@('-std=c++20','-O3','-ffp-contract=off','-static','-DWIN32=1','-D_WINDOWS','-DUNICODE','-D_UNICODE','-I',"$root/native/ui/include",'-I',$paths.ImGui,'-isystem',"$($paths.Sdk)/photoshopapi/photoshop",'-isystem',"$($paths.Sdk)/photoshopapi/pica_sp")+$paths.CompilerFlags
$units=@();foreach($m in @('fractal','displacement','glow','chromatic','vector','patterns','cell','lens','jpeg','lightfx','energy','blinds')){$flags+=@('-I',"$root/$m/include");$units+=(Get-ChildItem "$root/$m/src/*_core.cpp").FullName}
$exe="$root/native/build/gallery-parameters.exe"
& "$($paths.Toolchain)/x86_64-w64-mingw32-clang++.exe" @flags "$root/scripts/gallery-parameters.cpp" @units -o $exe
if($LASTEXITCODE){throw 'Gallery parameter build failed'}
& $exe $target
if($LASTEXITCODE){throw 'Gallery parameter generation failed'}
