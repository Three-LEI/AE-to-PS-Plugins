param(
    [Parameter(Mandatory)][string]$Renders,
    [string]$Output = 'docs/assets/social-preview.jpg'
)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing

$root = Split-Path $PSScriptRoot -Parent
$catalog = Get-Content -LiteralPath "$root/plugins.json" -Raw -Encoding UTF8 | ConvertFrom-Json
$rendersPath = [IO.Path]::GetFullPath($Renders)
$outputPath = [IO.Path]::GetFullPath($Output)
$tiles = @(
    @{ Slug='deep-glow';       Zh='深度发光'; En='Deep Glow';     X=48;  Y=174; Width=366; Height=378; Matte='#101020' }
    @{ Slug='pixel-dither';    Zh='像素抖动'; En='Pixel Dither';  X=428; Y=174; Width=258; Height=168; Matte='#f1f1ec' }
    @{ Slug='long-shadow';     Zh='长投影';   En='Long Shadow';   X=701; Y=174; Width=258; Height=168; Matte='#f4c9bd' }
    @{ Slug='channel-blur';    Zh='通道模糊'; En='Channel Blur';  X=974; Y=174; Width=258; Height=168; Matte='#f1f1ec' }
    @{ Slug='saber';           Zh='能量光束'; En='Saber';         X=428; Y=384; Width=258; Height=168; Matte='#101020' }
    @{ Slug='shine';           Zh='放射光束'; En='Shine';         X=701; Y=384; Width=258; Height=168; Matte='#101020' }
    @{ Slug='ascii-dither';    Zh='字符抖动'; En='ASCII Dither';  X=974; Y=384; Width=258; Height=168; Matte='#f1f1ec' }
)
foreach ($tile in $tiles) {
    $path = Join-Path $rendersPath "$($tile.Slug)-after.png"
    if (!(Test-Path -LiteralPath $path -PathType Leaf)) { throw "Missing plugin render: $($tile.Slug)" }
}

function Color([string]$hex) {
    [Drawing.ColorTranslator]::FromHtml($hex)
}

$cover = [Drawing.Bitmap]::new(1280,640,[Drawing.Imaging.PixelFormat]::Format24bppRgb)
$graphics = [Drawing.Graphics]::FromImage($cover)
$graphics.InterpolationMode = [Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
$graphics.PixelOffsetMode = [Drawing.Drawing2D.PixelOffsetMode]::HighQuality
$graphics.TextRenderingHint = [Drawing.Text.TextRenderingHint]::AntiAliasGridFit
$type = [Drawing.StringFormat]::GenericTypographic
$ink = [Drawing.SolidBrush]::new((Color '#101010'))
$muted = [Drawing.SolidBrush]::new((Color '#5f5f55'))
$accent = [Drawing.SolidBrush]::new((Color '#d8f84e'))
$rule = [Drawing.Pen]::new((Color '#b8b8ae'),1)
$border = [Drawing.Pen]::new((Color '#d2d2c9'),1)
$display = [Drawing.Font]::new('Segoe UI',86,[Drawing.FontStyle]::Bold,[Drawing.GraphicsUnit]::Pixel)
$heading = [Drawing.Font]::new('Microsoft YaHei UI',29,[Drawing.FontStyle]::Bold,[Drawing.GraphicsUnit]::Pixel)
$label = [Drawing.Font]::new('Microsoft YaHei UI',18,[Drawing.FontStyle]::Bold,[Drawing.GraphicsUnit]::Pixel)
$descriptor = [Drawing.Font]::new('Consolas',19,[Drawing.FontStyle]::Regular,[Drawing.GraphicsUnit]::Pixel)
$mono = [Drawing.Font]::new('Consolas',15,[Drawing.FontStyle]::Regular,[Drawing.GraphicsUnit]::Pixel)
$resources = @($graphics,$type,$ink,$muted,$accent,$rule,$border,$display,$heading,$label,$descriptor,$mono)

function Write-Title([string]$text, $font, $brush, [single]$x, [single]$y) {
    $graphics.DrawString($text,$font,$brush,$x,$y,$type)
}
function Write-Right([string]$text, $font, $brush, [single]$right, [single]$y) {
    $width = $graphics.MeasureString($text,$font,[Drawing.PointF]::Empty,$type).Width
    Write-Title $text $font $brush ($right-$width) $y
}

try {
    $graphics.Clear((Color '#f1f1ec'))
    Write-Title 'THREE_LEI' $mono $ink 48 25
    Write-Right 'WINDOWS X64 / MIT' $mono $muted 1232 25
    Write-Title 'AE → PS' $display $ink 46 43
    Write-Title "$($catalog.Count) 款 Photoshop 原生滤镜" $heading $ink 428 66
    Write-Title "$($catalog.Count) native Photoshop filters" $descriptor $muted 428 106
    $graphics.FillRectangle($accent,48,141,132,6)
    $graphics.FillRectangle($ink,48,141,13,6)
    $graphics.DrawLine($rule,48,157,1231,157)

    foreach ($tile in $tiles) {
        $rect = [Drawing.Rectangle]::new($tile.X,$tile.Y,$tile.Width,$tile.Height)
        $image = [Drawing.Image]::FromFile((Join-Path $rendersPath "$($tile.Slug)-after.png"))
        $matte = [Drawing.SolidBrush]::new((Color $tile.Matte))
        $sampling = [Drawing.Imaging.ImageAttributes]::new()
        try {
            $sampling.SetWrapMode([Drawing.Drawing2D.WrapMode]::TileFlipXY)
            $graphics.FillRectangle($matte,$rect)
            $scale = [Math]::Min($image.Width/$rect.Width,$image.Height/$rect.Height)
            $cropWidth = [single]($rect.Width*$scale)
            $cropHeight = [single]($rect.Height*$scale)
            $graphics.DrawImage($image,$rect,[single](($image.Width-$cropWidth)/2),
                [single](($image.Height-$cropHeight)/2),$cropWidth,$cropHeight,[Drawing.GraphicsUnit]::Pixel,$sampling)
            $graphics.DrawRectangle($border,$rect.X,$rect.Y,$rect.Width-1,$rect.Height-1)
        } finally {
            $sampling.Dispose()
            $matte.Dispose()
            $image.Dispose()
        }
        $captionY = $rect.Bottom+7
        Write-Title $tile.Zh $label $ink $rect.X $captionY
        $captionWidth = $graphics.MeasureString($tile.Zh,$label,[Drawing.PointF]::Empty,$type).Width
        Write-Title $tile.En $mono $muted ($rect.X+$captionWidth+12) ($captionY+3)
    }

    $graphics.DrawLine($rule,48,600,1231,600)
    Write-Title 'AE-TO-PS-PLUGINS' $mono $muted 48 611
    Write-Right 'THREE-LEI.CN' $mono $muted 1232 611
    New-Item -ItemType Directory -Path (Split-Path $outputPath -Parent) -Force | Out-Null
    $encoder = [Drawing.Imaging.ImageCodecInfo]::GetImageEncoders() | Where-Object MimeType -eq 'image/jpeg'
    $quality = [Drawing.Imaging.EncoderParameters]::new(1)
    try {
        $quality.Param[0] = [Drawing.Imaging.EncoderParameter]::new([Drawing.Imaging.Encoder]::Quality,[long]94)
        $cover.Save($outputPath,$encoder,$quality)
    } finally { $quality.Dispose() }
} finally {
    foreach ($resource in $resources) { $resource.Dispose() }
    $cover.Dispose()
}
"PASS: bilingual 1280×640 cover with $($tiles.Count) plugin renders."
