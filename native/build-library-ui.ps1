param()
$ErrorActionPreference='Stop'
$uiRoot=$PSScriptRoot
$uiProject=Split-Path $uiRoot -Parent
$uiPaths=& (Join-Path $uiProject 'scripts\resolve-build-paths.ps1')
$uiImgui=$uiPaths.ImGui
$uiTool=$uiPaths.Toolchain
$uiSdk=$uiPaths.Sdk
$uiOutput=Join-Path $uiRoot 'build'
$uiFlags=@('-std=c++20','-O2','-static','-DWIN32=1','-D_WINDOWS','-DUNICODE','-D_UNICODE','-I',"$uiRoot\ui\include",'-I',$uiImgui,'-I',"$uiImgui\backends",'-isystem',"$uiSdk\photoshopapi\photoshop",'-isystem',"$uiSdk\photoshopapi\pica_sp")
$uiFlags+=$uiPaths.CompilerFlags
$uiSources=@("$uiImgui\imgui.cpp","$uiImgui\imgui_draw.cpp","$uiImgui\imgui_widgets.cpp","$uiImgui\imgui_tables.cpp","$uiImgui\backends\imgui_impl_win32.cpp","$uiImgui\backends\imgui_impl_dx11.cpp","$uiRoot\ui\src\library_panel.cpp","$uiRoot\ui\src\numeric_scrub.cpp")
$uiObjects=@()
$uiHeaderTime=(Get-ChildItem -LiteralPath "$uiRoot\ui\include\psui" -Filter *.hpp | Sort-Object LastWriteTime -Descending | Select-Object -First 1).LastWriteTime
foreach($uiSource in $uiSources){
 $uiObject=Join-Path $uiOutput (([IO.Path]::GetFileNameWithoutExtension($uiSource))+'.obj')
 if(!(Test-Path -LiteralPath $uiObject) -or (Get-Item -LiteralPath $uiSource).LastWriteTime -gt (Get-Item -LiteralPath $uiObject).LastWriteTime -or $uiHeaderTime -gt (Get-Item -LiteralPath $uiObject).LastWriteTime){
  & (Join-Path $uiTool 'x86_64-w64-mingw32-clang++.exe') @uiFlags '-c' $uiSource '-o' $uiObject
  if($LASTEXITCODE -ne 0){throw "UI library build failed: $uiSource"}
 }
 $uiObjects+=$uiObject
}
& (Join-Path $uiTool 'llvm-ar.exe') 'rcs' "$uiOutput\library_ui.lib" @uiObjects
if($LASTEXITCODE -ne 0){throw 'UI archive failed'}
