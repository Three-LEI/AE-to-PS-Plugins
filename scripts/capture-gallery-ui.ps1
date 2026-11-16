param([Parameter(Mandatory)][string]$Renders,[string]$Output='evidence/ui-gallery-v01',[string[]]$Only=@())
$ErrorActionPreference='Stop';$root=Split-Path $PSScriptRoot -Parent;$paths=& "$root/scripts/resolve-build-paths.ps1";$out=[IO.Path]::GetFullPath($Output);$rendersPath=[IO.Path]::GetFullPath($Renders)
New-Item -ItemType Directory -Path "$out/settings" -Force|Out-Null;New-Item -ItemType Directory -Path "$out/captures" -Force|Out-Null
& python "$root/scripts/generate-gallery-ui-settings.py" --output "$out/settings";if($LASTEXITCODE){throw 'Gallery settings failed'}
$flags=@('-std=c++20','-O3','-ffp-contract=off','-static','-municode','-DWIN32=1','-D_WINDOWS','-DUNICODE','-D_UNICODE','-I',"$root/native/ui/include",'-I',"$root/displacement/include",'-I',$paths.ImGui,'-isystem',"$($paths.Sdk)/photoshopapi/photoshop",'-isystem',"$($paths.Sdk)/photoshopapi/pica_sp")+$paths.CompilerFlags
$libs=@("$root/native/build/library_ui.lib","$root/native/build/psui.lib",'-ld3d11','-ldxgi','-limm32','-luser32','-lgdi32','-ldwmapi','-lshell32','-lole32','-lwindowscodecs')
$modules=@('native','fractal','displacement','glow','chromatic','vector','patterns','cell','lens','jpeg','surface','creative','roughen','lightfx','energy','blinds')
$manifest=Get-Content "$root/docs/assets/gallery-manifest.json" -Raw -Encoding UTF8|ConvertFrom-Json;$entries=Get-Content "$root/plugins.json" -Raw -Encoding UTF8|ConvertFrom-Json
foreach($row in $manifest|Where-Object {!$Only.Count -or $_.slug -in $Only}){
 $entry=$entries|Where-Object file -eq $row.file;$module=$entry.module;$index=[array]::IndexOf($modules,$module);$variant=@()
 switch($entry.file){'ZaYuDeepGlow.8bf'{$variant=@('-DZAYU_DEEP_GLOW')};'ZaYuCheckerboard.8bf'{$variant=@('-DZAYU_CHECKERBOARD')};'ZaYuCCLens.8bf'{$variant=@('-DLENS_CC')};'ZaYuBlobbylize.8bf'{$variant=@('-DSURFACE_BLOB')};'ZaYuLightSweep.8bf'{$variant=@('-DCREATIVE_KIND=1')};'ZaYuKaleida.8bf'{$variant=@('-DCREATIVE_KIND=2')};'ZaYuShine.8bf'{$variant=@('-DLIGHTFX_KIND=1')}}
 $core=(Get-ChildItem "$root/$module/src/*_core.cpp").FullName;$dialog=if($module -eq 'native'){"$root/native/src/tint_library_dialog.cpp"}else{"$root/$module/src/library_dialog.cpp"};$extra=@();if($module -eq 'energy'){$extra+="$root/energy/src/energy_host.cpp"}
 $settings=("$out/settings/$($row.slug).hpp").Replace('\','/');$exe="$out/$($row.slug).exe"
 & "$($paths.Toolchain)/x86_64-w64-mingw32-clang++.exe" @flags @variant "-DGALLERY_MODULE=$index" ('-DGALLERY_SETTINGS="'+$settings+'"') -I "$root/$module/include" -I "$root/$module/build" "$root/scripts/gallery-ui.cpp" $core $dialog @extra @libs -o $exe
 if($LASTEXITCODE){throw "Gallery UI compilation failed: $($row.slug)"}
 & $exe "$rendersPath/$($row.slug)-before.png" "$out/captures/$($row.slug).bmp"|Tee-Object -Append "$out/capture-results.txt"
 if($LASTEXITCODE){throw "Gallery UI capture failed: $($row.slug)"}
}
