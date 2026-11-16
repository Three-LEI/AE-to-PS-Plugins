param([string[]]$Profiles=@('zh-200-legacy','en-100'),[string[]]$Only=@(),[string]$OutputRoot='')
$ErrorActionPreference='Stop';$root=Split-Path $PSScriptRoot -Parent
$paths=& "$root/scripts/resolve-build-paths.ps1";$out=if($OutputRoot){[IO.Path]::GetFullPath($OutputRoot)}else{"$root/evidence/adaptive-preview-v01"};New-Item -ItemType Directory -Path $out -Force|Out-Null
$flags=@('-std=c++20','-O3','-ffp-contract=off','-static','-DWIN32=1','-D_WINDOWS','-DUNICODE','-D_UNICODE','-I',"$root/native/ui/include",'-I',"$root/displacement/include",'-I',$paths.ImGui,'-isystem',"$($paths.Sdk)/photoshopapi/photoshop",'-isystem',"$($paths.Sdk)/photoshopapi/pica_sp")+$paths.CompilerFlags
$libs=@("$root/native/build/library_ui.lib","$root/native/build/psui.lib",'-ld3d11','-ldxgi','-limm32','-luser32','-lgdi32','-ldwmapi','-lshell32')
$modules=@('native','fractal','displacement','glow','chromatic','vector','patterns','cell','lens','jpeg','surface','creative','roughen')
$entries=Get-Content "$root/plugins.json" -Raw -Encoding UTF8|ConvertFrom-Json
foreach($entry in $entries|Where-Object {$_.module -notin @('lightfx','energy','blinds') -and (!$Only.Count -or $_.file -in $Only)}){
 $module=$entry.module;$index=[array]::IndexOf($modules,$module);$name=[IO.Path]::GetFileNameWithoutExtension($entry.file)
 $variant=@();switch($name){'ZaYuDeepGlow'{$variant=@('-DZAYU_DEEP_GLOW')};'ZaYuCheckerboard'{$variant=@('-DZAYU_CHECKERBOARD')};'ZaYuCCLens'{$variant=@('-DLENS_CC')};'ZaYuBlobbylize'{$variant=@('-DSURFACE_BLOB')};'ZaYuLightSweep'{$variant=@('-DCREATIVE_KIND=1')};'ZaYuKaleida'{$variant=@('-DCREATIVE_KIND=2')}}
 $core=(Get-ChildItem "$root/$module/src/*_core.cpp").FullName;$dialog=if($module -eq 'native'){"$root/native/src/tint_library_dialog.cpp"}else{"$root/$module/src/library_dialog.cpp"}
 & "$($paths.Toolchain)/x86_64-w64-mingw32-clang++.exe" @flags @variant "-DADAPT_MODULE=$index" -I "$root/$module/include" "$PSScriptRoot/adaptive-panel.cpp" $core $dialog @libs -o "$out/$name-adaptive.exe"
 if($LASTEXITCODE){throw "Adaptive panel compilation failed: $name"}
 foreach($profile in $Profiles){& "$out/$name-adaptive.exe" $profile | Tee-Object -Append "$out/ui-results.txt";if($LASTEXITCODE){throw "Adaptive panel failed: $name $profile"}}
}
