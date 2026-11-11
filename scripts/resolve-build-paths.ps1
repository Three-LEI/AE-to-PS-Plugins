param([string]$ProjectRoot=(Split-Path $PSScriptRoot -Parent),[switch]$CoreOnly)
$ErrorActionPreference='Stop'
$localFile=Join-Path $ProjectRoot 'build.local.json'
$localConfig=if(Test-Path -LiteralPath $localFile){Get-Content -LiteralPath $localFile -Raw -Encoding utf8|ConvertFrom-Json}else{@{}}
function Choose-Path([string]$EnvironmentName,[string]$Property,[string]$Fallback){
 $environmentValue=[Environment]::GetEnvironmentVariable($EnvironmentName)
 if($environmentValue){return $environmentValue}
 if($localConfig.$Property){return [string]$localConfig.$Property}
 return $Fallback
}
$compiler=Get-Command 'x86_64-w64-mingw32-clang++.exe' -ErrorAction SilentlyContinue
$toolchain=Choose-Path 'AEPS_TOOLCHAIN' 'toolchain' $(if($compiler){Split-Path $compiler.Source -Parent}else{''})
if(!$toolchain -or !(Test-Path -LiteralPath (Join-Path $toolchain 'x86_64-w64-mingw32-clang++.exe'))){throw 'Set AEPS_TOOLCHAIN to the LLVM-MinGW bin directory, or supply -Toolchain to the root build.ps1. See docs/BUILDING.md.'}
$sdk=Choose-Path 'AEPS_PHOTOSHOP_SDK' 'photoshop_sdk' (Join-Path $ProjectRoot 'downloads\photoshop-cpp-sdk-main\pluginsdk')
$imgui=Choose-Path 'AEPS_IMGUI_ROOT' 'imgui' (Join-Path $ProjectRoot 'third_party\imgui')
if(!$CoreOnly){
 foreach($file in @('photoshopapi\photoshop\PIFilter.h','photoshopapi\pica_sp\SPBasic.h')){if(!(Test-Path -LiteralPath (Join-Path $sdk $file))){throw 'Set AEPS_PHOTOSHOP_SDK to your separately obtained pluginsdk directory. Adobe SDK files are not included.'}}
}
if(!(Test-Path -LiteralPath (Join-Path $imgui 'imgui.h'))){throw 'Dear ImGui is missing: restore third_party/imgui or set AEPS_IMGUI_ROOT.'}
[pscustomobject]@{Toolchain=[IO.Path]::GetFullPath($toolchain);Sdk=[IO.Path]::GetFullPath($sdk);ImGui=[IO.Path]::GetFullPath($imgui);CompilerFlags=@("-ffile-prefix-map=$ProjectRoot=.","-ffile-prefix-map=$imgui=third_party/imgui")}
