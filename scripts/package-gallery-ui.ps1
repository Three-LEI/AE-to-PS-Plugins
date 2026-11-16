param([Parameter(Mandatory)][string]$Captures,[string]$Assets='docs/assets',[string[]]$Only=@())
$ErrorActionPreference='Stop';Add-Type -AssemblyName System.Drawing
$root=Split-Path $PSScriptRoot -Parent;$capturePath=[IO.Path]::GetFullPath($Captures);$assetsPath=[IO.Path]::GetFullPath($Assets)
New-Item -ItemType Directory -Path "$assetsPath/effects" -Force|Out-Null
$manifest=Get-Content "$root/docs/assets/gallery-manifest.json" -Raw -Encoding UTF8|ConvertFrom-Json
# Validate the complete set before replacing any currently displayed gallery file.
$selected=@($manifest|Where-Object {!$Only.Count -or $_.slug -in $Only})
if($Only.Count -and $selected.Count -ne $Only.Count){throw 'Unknown or duplicate gallery selection'}
foreach($row in $selected){if(!(Test-Path -LiteralPath "$capturePath/$($row.slug).bmp")){throw "Missing parameter window: $($row.slug)"}}
foreach($row in $selected){
 $image=[Drawing.Image]::FromFile("$capturePath/$($row.slug).bmp")
 try{
  if($image.Width -lt 600 -or $image.Height -lt 350){throw "Unexpected capture dimensions: $($row.slug)"}
  $image.Save("$assetsPath/effects/$($row.slug).png",[Drawing.Imaging.ImageFormat]::Png)
  $row.image="assets/effects/$($row.slug).png";$row.width=$image.Width;$row.height=$image.Height
  $row|Add-Member -NotePropertyName presentation -NotePropertyValue 'parameter-window' -Force
  $row|Add-Member -NotePropertyName capture -NotePropertyValue 'production-dialog-standalone' -Force
  $row|Add-Member -NotePropertyName preview -NotePropertyValue 'full' -Force
 }finally{$image.Dispose()}
}
$manifest|ConvertTo-Json -Depth 5|Set-Content "$assetsPath/gallery-manifest.json" -Encoding UTF8
"PASS: $($selected.Count) lossless production parameter-window images; social-preview.jpg was not modified"
