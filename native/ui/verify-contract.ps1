$ErrorActionPreference='Stop'
$uiProject=Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
foreach($uiEntry in @('native/src/tint_library_dialog.cpp','fractal/src/library_dialog.cpp','displacement/src/library_dialog.cpp','glow/src/library_dialog.cpp','chromatic/src/library_dialog.cpp','vector/src/library_dialog.cpp','patterns/src/library_dialog.cpp','cell/src/library_dialog.cpp','lens/src/library_dialog.cpp')){
 $uiCode=[IO.File]::ReadAllText((Join-Path $uiProject $uiEntry),[Text.Encoding]::UTF8)
 foreach($uiForbidden in @('\bWM_DRAWITEM\b','\bWM_ERASEBKGND\b','\bCreateWindow(?:Ex)?[AW]?\s*\(','\bGetDC\s*\(','\bFillRect\s*\(','\bInvalidateRect\s*\(')){
  if($uiCode -match $uiForbidden){throw "UI contract failed: $uiEntry contains $uiForbidden. Use the shared library panel instead."}
 }
 if($uiCode -notmatch 'run_library_panel'){throw "UI contract failed: $uiEntry must use run_library_panel"}
 if($uiCode -notmatch 'ui\.parameter_defaults\(' -or $uiCode -notmatch '全部重置 / Reset All'){throw "UI contract failed: $uiEntry must bind factory defaults and label the global reset"}
 if($uiCode -match 'ImGui::ColorButton\s*\('){throw "UI contract failed: $uiEntry must use shared color_swatch for picker and eyedropper support"}
 if($uiCode -match 'ImGui::(?:Input(?:Double|Float\d?|Int\d?|Scalar)|Drag(?:Float\d?|Int\d?|Scalar)|Slider(?:Float\d?|Int\d?|Scalar))\s*\('){throw "UI contract failed: $uiEntry must use shared numeric controls for blue text and modifier scrubbing"}
}
$sharedPanel=[IO.File]::ReadAllText((Join-Path $uiProject 'native\ui\src\library_panel.cpp'),[Text.Encoding]::UTF8)
if($sharedPanel -match 'host_sample_rgb\s*\('){throw 'UI contract failed: the full-screen eyedropper must not invoke legacy interactive host sampling.'}
Write-Output 'PASS: UI architecture contract; shared controls, no native control painting, no legacy interactive host sampling in the eyedropper.'
