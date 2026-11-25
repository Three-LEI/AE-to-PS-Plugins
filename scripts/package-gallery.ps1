param([Parameter(Mandatory)][string]$Renders,[Parameter(Mandatory)][string]$Parameters,[string]$Assets='docs/assets',[string]$Proof='',[switch]$CoverOnly)
$ErrorActionPreference='Stop';Add-Type -AssemblyName System.Drawing
$root=Split-Path $PSScriptRoot -Parent;$rendersPath=[IO.Path]::GetFullPath($Renders);$assetsPath=[IO.Path]::GetFullPath($Assets)
New-Item -ItemType Directory -Path "$assetsPath/effects" -Force|Out-Null
$rows=Get-Content -LiteralPath $Parameters -Raw -Encoding UTF8|ConvertFrom-Json
$catalog=Get-Content "$root/plugins.json" -Raw -Encoding UTF8|ConvertFrom-Json
$names=@{};foreach($entry in $catalog){$names[$entry.file]=$entry.name}
$slugNames=@{'tint'='ZaYuTint.8bf';'fractal-noise'='ZaYuFractalNoise.8bf';'displacement-map'='ZaYuDisplacementMap.8bf';'glow'='ZaYuGlow.8bf';'deep-glow'='ZaYuDeepGlow.8bf';'chromatic-dispersion'='ZaYuChromaticDispersion.8bf';'vector-blur'='ZaYuVectorBlur.8bf';'grid'='ZaYuGrid.8bf';'checkerboard'='ZaYuCheckerboard.8bf';'cell-pattern'='ZaYuCellPattern.8bf';'optics-compensation'='ZaYuOpticsCompensation.8bf';'lens'='ZaYuCCLens.8bf';'jpeg-damage'='ZaYuJPEGDamage.8bf';'blobbylize'='ZaYuBlobbylize.8bf';'glass'='ZaYuGlass.8bf';'turbulent-displace'='ZaYuTurbulentDisplace.8bf';'light-sweep'='ZaYuLightSweep.8bf';'kaleida'='ZaYuKaleida.8bf';'roughen-edges'='ZaYuRoughenEdges.8bf';'starglow'='ZaYuStarglow.8bf';'shine'='ZaYuShine.8bf';'saber'='ZaYuSaber.8bf';'venetian-blinds'='ZaYuVenetianBlinds.8bf'}
$encoder=[Drawing.Imaging.ImageCodecInfo]::GetImageEncoders()|Where-Object MimeType -eq 'image/jpeg';$quality=[Drawing.Imaging.EncoderParameters]::new(1);$quality.Param[0]=[Drawing.Imaging.EncoderParameter]::new([Drawing.Imaging.Encoder]::Quality,[long]88)
$titleFont=[Drawing.Font]::new('Microsoft YaHei UI',21,[Drawing.FontStyle]::Bold,[Drawing.GraphicsUnit]::Pixel)
$smallFont=[Drawing.Font]::new('Segoe UI',19,[Drawing.FontStyle]::Regular,[Drawing.GraphicsUnit]::Pixel)
$white=[Drawing.SolidBrush]::new([Drawing.Color]::FromArgb(238,243,248));$muted=[Drawing.SolidBrush]::new([Drawing.Color]::FromArgb(162,180,198))
function Checker($graphics,[int]$left,[int]$top,[int]$size){
 $a=[Drawing.SolidBrush]::new([Drawing.Color]::FromArgb(33,39,47));$b=[Drawing.SolidBrush]::new([Drawing.Color]::FromArgb(43,50,59))
 for($y=0;$y -lt $size;$y+=16){for($x=0;$x -lt $size;$x+=16){$brush=if((($x/16+$y/16)%2) -eq 0){$a}else{$b};$graphics.FillRectangle($brush,$left+$x,$top+$y,[Math]::Min(16,$size-$x),[Math]::Min(16,$size-$y))}}
 $a.Dispose();$b.Dispose()
}
$index=0;$manifest=@()
if(!$CoverOnly){
$proofBitmap=if($Proof){[Drawing.Bitmap]::new(1536,1112)}else{$null};$proofGraphics=if($Proof){[Drawing.Graphics]::FromImage($proofBitmap)}else{$null}
if($Proof){$proofGraphics.Clear([Drawing.Color]::FromArgb(16,22,29));$proofGraphics.InterpolationMode=[Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic}
foreach($row in $rows){
 $before=[Drawing.Image]::FromFile("$rendersPath/$($row.slug)-before.png");$after=[Drawing.Image]::FromFile("$rendersPath/$($row.slug)-after.png")
 $card=[Drawing.Bitmap]::new(1024,616);$g=[Drawing.Graphics]::FromImage($card);$g.Clear([Drawing.Color]::FromArgb(16,22,29));$g.InterpolationMode=[Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
 $g.DrawString($names[$slugNames[$row.slug]],$titleFont,$white,[single]18,[single]15)
 Checker $g 0 56 512;Checker $g 512 56 512;$g.DrawImage($before,0,56,512,512);$g.DrawImage($after,512,56,512,512)
 $g.DrawString('BEFORE / INPUT',$smallFont,$muted,[single]18,[single]580);$g.DrawString('AFTER / PLUGIN OUTPUT',$smallFont,$white,[single]530,[single]580)
 $card.Save("$assetsPath/effects/$($row.slug).jpg",$encoder,$quality)
 if($Proof){$x=($index%6)*256;$y=[Math]::Floor($index/6)*278;Checker $proofGraphics $x $y 256;$proofGraphics.DrawImage($after,[int]$x,[int]$y,256,256);$proofGraphics.DrawString($row.slug,$smallFont,$white,[single]($x+5),[single]($y+257))}
 $manifest+=[pscustomobject]@{slug=$row.slug;name=$names[$slugNames[$row.slug]];file=$slugNames[$row.slug];input=$row.input;image="assets/effects/$($row.slug).jpg";width=1024;height=616}
 $g.Dispose();$card.Dispose();$before.Dispose();$after.Dispose();$index++
}
Copy-Item -LiteralPath "$rendersPath/tint-before.png" -Destination "$assetsPath/demo-source.png"
& python "$root/scripts/strip-demo-metadata.py" "$assetsPath/demo-source.png"
if($LASTEXITCODE){throw 'Demo metadata cleanup failed'}
if($Proof){$proofBitmap.Save([IO.Path]::GetFullPath($Proof),[Drawing.Imaging.ImageFormat]::Png);$proofGraphics.Dispose();$proofBitmap.Dispose()}
}
$hero=[Drawing.Bitmap]::new(1280,640);$hg=[Drawing.Graphics]::FromImage($hero);$hg.Clear([Drawing.Color]::FromArgb(12,19,28));$hg.InterpolationMode=[Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
$big=[Drawing.Font]::new('Segoe UI',66,[Drawing.FontStyle]::Bold,[Drawing.GraphicsUnit]::Pixel);$medium=[Drawing.Font]::new('Segoe UI',29,[Drawing.FontStyle]::Regular,[Drawing.GraphicsUnit]::Pixel);$cyan=[Drawing.SolidBrush]::new([Drawing.Color]::FromArgb(76,206,255))
$hg.DrawString('AE → PS',$big,$white,[single]46,[single]102);$hg.DrawString("$($catalog.Count) native Photoshop filters",$medium,$white,[single]48,[single]208)
$hg.DrawString('Glow · Glass · Energy · Noise',$smallFont,$muted,[single]50,[single]275);$hg.DrawString('Smart Filters · Fast previews',$smallFont,$cyan,[single]50,[single]324)
$hg.DrawString('Windows x64  /  MIT source',$smallFont,$muted,[single]50,[single]460);$hg.DrawString('Three-LEI / AE-to-PS-Plugins',$smallFont,$muted,[single]50,[single]504)
$tiles=@('deep-glow','chromatic-dispersion','glass','saber','starglow','roughen-edges')
for($i=0;$i -lt $tiles.Count;$i++){$im=[Drawing.Image]::FromFile("$rendersPath/$($tiles[$i])-after.png");$x=650+($i%3)*200;$y=78+[Math]::Floor($i/3)*256;Checker $hg $x $y 186;$hg.DrawImage($im,[int]$x,[int]$y,186,186);$hg.DrawString($tiles[$i],$smallFont,$white,[single]$x,[single]($y+194));$im.Dispose()}
$hero.Save("$assetsPath/social-preview.jpg",$encoder,$quality);$hg.Dispose();$hero.Dispose();$big.Dispose();$medium.Dispose();$cyan.Dispose();$titleFont.Dispose();$smallFont.Dispose();$white.Dispose();$muted.Dispose();$quality.Dispose()
if($CoverOnly){'PASS: 1280x640 sharing cover'}
else{
 $manifest|ConvertTo-Json -Depth 5|Set-Content "$assetsPath/gallery-manifest.json" -Encoding UTF8
 "PASS: $($manifest.Count) native before/after cards, source image and 1280x640 sharing cover"
}
