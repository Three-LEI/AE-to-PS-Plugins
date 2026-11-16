param([string]$OutputRoot='')
$ErrorActionPreference='Stop';$root=Split-Path $PSScriptRoot -Parent;$base=if($OutputRoot){[IO.Path]::GetFullPath($OutputRoot)}else{"$root/evidence/preview-quality-v01"}
New-Item -ItemType Directory -Path $base -Force|Out-Null
$paths=& "$root/scripts/resolve-build-paths.ps1"
$flags=@('-std=c++20','-O3','-ffp-contract=off','-static','-DWIN32=1','-D_WINDOWS','-DUNICODE','-D_UNICODE','-I',"$root/blinds/include",'-I',"$root/native/ui/include",'-I',$paths.ImGui,'-isystem',"$($paths.Sdk)/photoshopapi/photoshop",'-isystem',"$($paths.Sdk)/photoshopapi/pica_sp")+$paths.CompilerFlags
& "$($paths.Toolchain)/x86_64-w64-mingw32-clang++.exe" @flags "$root/blinds/src/blinds_core.cpp" "$root/qa/preview-quality-tests.cpp" -o "$base/quality-tests.exe"
if($LASTEXITCODE){throw 'Quality test build failed'}
& "$base/quality-tests.exe" "$base/comparison.csv"|Tee-Object "$base/quality-results.txt"
if($LASTEXITCODE){throw 'Quality verification failed'}
