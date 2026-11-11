param(
 [ValidateSet('All','native','fractal','displacement','glow','chromatic','vector','patterns','cell','lens','jpeg','surface','creative','roughen')][string]$Module='All',
 [string]$Toolchain,[string]$PhotoshopSdk,[string]$ImGui,[switch]$CoreOnly
)
$ErrorActionPreference='Stop'
$names=@('AEPS_TOOLCHAIN','AEPS_PHOTOSHOP_SDK','AEPS_IMGUI_ROOT')
$previous=@{};foreach($name in $names){$previous[$name]=[Environment]::GetEnvironmentVariable($name)}
try{
 if($Toolchain){$env:AEPS_TOOLCHAIN=$Toolchain};if($PhotoshopSdk){$env:AEPS_PHOTOSHOP_SDK=$PhotoshopSdk};if($ImGui){$env:AEPS_IMGUI_ROOT=$ImGui}
 $paths=& "$PSScriptRoot\scripts\resolve-build-paths.ps1" -CoreOnly:$CoreOnly
 $modules=if($Module -eq 'All'){@('native','fractal','displacement','glow','chromatic','vector','patterns','cell','lens','jpeg','surface','creative','roughen')}else{@($Module)}
 if(!$CoreOnly -and $Module -ne 'All' -and $Module -ne 'native' -and (!(Test-Path -LiteralPath "$PSScriptRoot\native\build\library_ui.lib") -or !(Test-Path -LiteralPath "$PSScriptRoot\native\build\psui.lib"))){throw 'Run the root build.ps1 once with -Module native to build shared UI libraries first.'}
 Push-Location $PSScriptRoot
 try{foreach($entry in $modules){Write-Output "Building $entry (CoreOnly=$CoreOnly)";& "$PSScriptRoot\$entry\build.ps1" -CoreOnly:$CoreOnly}}finally{Pop-Location}
 Write-Output 'PASS: requested builds and their required checks completed. No Photoshop installation was modified.'
}finally{foreach($name in $names){[Environment]::SetEnvironmentVariable($name,$previous[$name])}}
