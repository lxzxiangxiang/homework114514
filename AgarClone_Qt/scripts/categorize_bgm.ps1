$bgmDir = "assets/bgm"
$originalsDir = "$bgmDir/originals"

Write-Output "=== BGM 分类开始 ==="

# 获取 originals 文件列表（key=短名匹配, value=完整文件名）
$files = @{}
Get-ChildItem $originalsDir | ForEach-Object { $files[$_.Name] = $_.Name }

function Copy-IfExist($filename, $destDir) {
    if ($files.ContainsKey($filename)) {
        Copy-Item (Join-Path $originalsDir $filename) (Join-Path $bgmDir $destDir) -Force
        Write-Output "  COPY $filename -> $destDir/"
        return $true
    } else {
        Write-Warning "  MISSING: $filename"
        return $false
    }
}

# ===== MENU =====
Write-Output "`n[menu]"
$menuFiles = @(
    "Mice on Venus - C418.mp3",
    "Haggstrom - C418.mp3",
    "Cat - C418.mp3",
    "Dog - C418.mp3",
    "Candyland - Tobu.mp3",
    "The Right Path - Thomas Greenberg.mp3",
    "Life - Tobu.mp3"
)
foreach ($f in $menuFiles) { Copy-IfExist $f "menu" }

# 匹配日文/中文（模糊匹配）
$originals = Get-ChildItem $originalsDir
foreach ($f in $originals) {
    $name = $f.Name
    if ($name -match "はじまり") { Copy-IfExist $name "menu" }
    if ($name -match "やわらかな光") { Copy-IfExist $name "menu" }
    if ($name -match "白日食堂") { Copy-IfExist $name "menu" }
    if ($name -match "钢琴") { Copy-IfExist $name "menu" }
    if ($name -match "烟袋斜街") { Copy-IfExist $name "menu" }
    if ($name -match "璃上有月") { Copy-IfExist $name "menu" }
    if ($name -match "无仙") { Copy-IfExist $name "menu" }
    if ($name -match "银花玉鉴") { Copy-IfExist $name "menu" }
    if ($name -match "村巷欢声") { Copy-IfExist $name "menu" }
    if ($name -match "沉浸感") { Copy-IfExist $name "menu" }
    if ($name -match "ひやむぎ") { Copy-IfExist $name "menu" }
}

# ===== GAME =====
Write-Output "`n[game]"
$gameFiles = @(
    "Higher - Tobu.mp3",
    "Sunburst - Tobu _ Itro.mp3",
    "Ryukyuvania - DarioVan.mp3"
)
foreach ($f in $gameFiles) { Copy-IfExist $f "game" }

foreach ($f in $originals) {
    $name = $f.Name
    if ($name -match "MEGALOVANIA") { Copy-IfExist $name "game" }
    if ($name -match "Hopes And Dreams") { Copy-IfExist $name "game" }
    if ($name -match "Brainiac Maniac") { Copy-IfExist $name "game" }
    if ($name -match "Astronomia") { Copy-IfExist $name "game" }
    if ($name -match "Go Big Or Go Extinct") { Copy-IfExist $name "game" }
    if ($name -match "Dragonflame") { Copy-IfExist $name "game" }
    if ($name -match "sans_") { Copy-IfExist $name "game" }
    if ($name -match "WINDOWS开机") { Copy-IfExist $name "game" }
    if ($name -match "color-X") { Copy-IfExist $name "game" }
    if ($name -match "上上下下") { Copy-IfExist $name "game" }
    if ($name -match "太空漫步") { Copy-IfExist $name "game" }
    if ($name -match "睡蕉之歌") { Copy-IfExist $name "game" }
    if ($name -match "Tom And Jerry") { Copy-IfExist $name "game" }
}

# ===== FAILURE =====
Write-Output "`n[failure]"
foreach ($f in $originals) {
    $name = $f.Name
    if ($name -match "流浪者之歌") { Copy-IfExist $name "failure" }
    if ($name -match "二泉映月") { Copy-IfExist $name "failure" }
    if ($name -match "His Theme") { Copy-IfExist $name "failure" }
    if ($name -match "Your New Home") { Copy-IfExist $name "failure" }
    if ($name -match "Travelers' encore") { Copy-IfExist $name "failure" }
    if ($name -match "Torches") { Copy-IfExist $name "failure" }
    if ($name -match "Por Una Cabeza") { Copy-IfExist $name "failure" }
    if ($name -match "舌尖上的中国") { Copy-IfExist $name "failure" }
    if ($name -match "III \(Find Yourself\)" -or $name -match "III.*Find Yourself") { Copy-IfExist $name "failure" }
    if ($name -match "Tom And Jerry") { Copy-IfExist $name "failure" }
}

# ===== SUCCESS =====
Write-Output "`n[success]"
foreach ($f in $originals) {
    $name = $f.Name
    if ($name -match "やったー") { Copy-IfExist $name "success" }
    if ($name -match "年夜饭") { Copy-IfExist $name "success" }
    if ($name -match "Unwelcome School") { Copy-IfExist $name "success" }
    if ($name -match "2：23AM") { Copy-IfExist $name "success" }
    if ($name -match "世界尽头酒馆") { Copy-IfExist $name "success" }
    if ($name -match "III \(Find Yourself\)" -or $name -match "III.*Find Yourself") { Copy-IfExist $name "success" }
}

# ===== 统计 =====
Write-Output "`n=== 统计 ==="
foreach ($cat in @("menu","game","failure","success")) {
    $count = (Get-ChildItem "$bgmDir/$cat" -File | Measure-Object).Count
    Write-Output "$cat : $count 首"
}
$sfxCount = (Get-ChildItem "$bgmDir/sfx" -File | Measure-Object).Count
$origCount = (Get-ChildItem "$originalsDir" -File | Measure-Object).Count
Write-Output "sfx : $sfxCount 个"
Write-Output "originals : $origCount 首 (备份)"
Write-Output "=== 完成 ==="