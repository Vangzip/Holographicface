param(
    [string]$ConfigPath = "",
    [ValidateSet("Release")]
    [string]$Configuration = "Release",
    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"

$holoDir = Split-Path -Parent $PSCommandPath
$repoRoot = Resolve-Path (Join-Path $holoDir "..\..")
if ([string]::IsNullOrWhiteSpace($ConfigPath)) {
    $ConfigPath = Join-Path $holoDir "holo_config.microtest.ini"
}
$configPath = [System.IO.Path]::GetFullPath($ConfigPath)
$configDir = Split-Path -Parent $configPath
$slnPath = Join-Path $holoDir "Holo.sln"
$exePath = Join-Path $holoDir "target\Holo.exe"
$inputStem = "0"

function Assert-True([bool]$Condition, [string]$Message) {
    if (-not $Condition) {
        throw $Message
    }
}

function Read-HoloIni([string]$Path) {
    $values = @{}
    foreach ($line in Get-Content -LiteralPath $Path) {
        $trimmed = $line.Trim()
        if ($trimmed.Length -eq 0 -or $trimmed.StartsWith("#") -or $trimmed.StartsWith(";")) {
            continue
        }
        if ($trimmed.StartsWith("[") -and $trimmed.EndsWith("]")) {
            continue
        }

        $pos = $trimmed.IndexOf("=")
        if ($pos -lt 0) {
            continue
        }

        $key = $trimmed.Substring(0, $pos).Trim().ToLowerInvariant()
        $value = $trimmed.Substring($pos + 1).Trim()
        $values[$key] = $value
    }
    return $values
}

function Get-ConfigValue($Values, [string]$Key, [string]$Fallback) {
    $lowerKey = $Key.ToLowerInvariant()
    if ($Values.ContainsKey($lowerKey) -and -not [string]::IsNullOrWhiteSpace($Values[$lowerKey])) {
        return $Values[$lowerKey]
    }
    return $Fallback
}

function Get-ConfigInt($Values, [string]$Key, [int]$Fallback) {
    $raw = Get-ConfigValue $Values $Key ""
    $parsed = 0
    if ([int]::TryParse($raw, [ref]$parsed)) {
        return $parsed
    }
    return $Fallback
}

function Resolve-ConfigPath([string]$BaseDir, [string]$Value, [string]$Fallback) {
    if ([string]::IsNullOrWhiteSpace($Value)) {
        $Value = $Fallback
    }
    if ([string]::IsNullOrWhiteSpace($Value)) {
        return ""
    }

    $expanded = [Environment]::ExpandEnvironmentVariables($Value)
    if ([System.IO.Path]::IsPathRooted($expanded)) {
        return [System.IO.Path]::GetFullPath($expanded)
    }
    return [System.IO.Path]::GetFullPath((Join-Path $BaseDir $expanded))
}

function Remove-TreeSafe([string]$Path, [string]$AllowedRoot) {
    $resolvedAllowed = [System.IO.Path]::GetFullPath($AllowedRoot).TrimEnd("\", "/")
    $fullPath = [System.IO.Path]::GetFullPath($Path).TrimEnd("\", "/")
    $allowedPrefix = $resolvedAllowed + [System.IO.Path]::DirectorySeparatorChar
    Assert-True (-not $fullPath.Equals($resolvedAllowed, [System.StringComparison]::OrdinalIgnoreCase)) "Refusing to remove allowed root itself: $fullPath"
    Assert-True ($fullPath.StartsWith($allowedPrefix, [System.StringComparison]::OrdinalIgnoreCase)) "Refusing to remove outside $resolvedAllowed`: $fullPath"
    if (Test-Path -LiteralPath $fullPath) {
        Remove-Item -LiteralPath $fullPath -Recurse -Force
    }
}

function Remove-GeneratedForStem([string]$Stem, [string]$InputDir) {
    $patterns = @(
        "$Stem`_rgb.ply",
        "$Stem`_mesh.ply",
        "$Stem`_mesh_model.obj",
        "$Stem`_mesh_model.mtl",
        "$Stem`_mesh_model.jpg"
    )
    foreach ($pattern in $patterns) {
        $path = Join-Path $InputDir $pattern
        if (Test-Path -LiteralPath $path) {
            Remove-Item -LiteralPath $path -Force
        }
    }
}

function Get-JpegSize([string]$Path) {
    Add-Type -AssemblyName System.Drawing
    $image = [System.Drawing.Image]::FromFile($Path)
    try {
        return @{ Width = $image.Width; Height = $image.Height }
    }
    finally {
        $image.Dispose()
    }
}

function Get-DecimalDigits([int]$Value) {
    $digits = 1
    while ($Value -ge 10) {
        $Value = [Math]::Floor($Value / 10)
        $digits += 1
    }
    return $digits
}

function Format-PaddedNumber([int]$Value, [int]$Digits) {
    return $Value.ToString("D$Digits")
}

Assert-True (Test-Path -LiteralPath $configPath) "Missing config: $configPath"

$config = Read-HoloIni $configPath
$inputDir = Resolve-ConfigPath $configDir (Get-ConfigValue $config "depth_input_dir" "face_roate") ""
$outputRoot = Resolve-ConfigPath $configDir (Get-ConfigValue $config "output_root" "output") ""
$multiviewDir = Resolve-ConfigPath $outputRoot (Get-ConfigValue $config "multiview_out_dir" "multiview") ""
$elementalDir = Resolve-ConfigPath $outputRoot (Get-ConfigValue $config "elemental_out_dir" "elemental") ""

$angle = Get-ConfigInt $config "multiview_angle" 30
$per = Get-ConfigInt $config "multiview_per" 9
$resolution = Get-ConfigInt $config "multiview_resolution" 150
$viewDigits = Get-ConfigInt $config "view_name_digits" 3
$targetRows = Get-ConfigInt $config "target_rows" 150
$targetCols = Get-ConfigInt $config "target_cols" 150
$viewCount = $angle * $per

Assert-True ($viewCount -gt 0) "multiview_angle * multiview_per must be positive"
Assert-True ($resolution -gt 0) "multiview_resolution must be positive"
Assert-True ($targetRows -gt 0 -and $targetCols -gt 0) "target_rows and target_cols must be positive"

Assert-True (Test-Path -LiteralPath (Join-Path $inputDir "$inputStem.tiff")) "Missing input depth: $inputDir\$inputStem.tiff"
Assert-True (Test-Path -LiteralPath (Join-Path $inputDir "$inputStem.jpg")) "Missing input color: $inputDir\$inputStem.jpg"

$extraInputs = @(Get-ChildItem -LiteralPath $inputDir -Filter "*.tiff" -File | Where-Object { $_.BaseName -ne $inputStem })
Assert-True ($extraInputs.Count -eq 0) "Microtest expects only $inputStem.tiff in $inputDir; remove: $($extraInputs.Name -join ', ')"

if (-not $SkipBuild) {
    Push-Location $repoRoot
    try {
        & msbuild $slnPath /t:Rebuild /p:Configuration=$Configuration /p:Platform=x64 /m /v:minimal
        if ($LASTEXITCODE -ne 0) {
            throw "MSBuild exited with code $LASTEXITCODE"
        }
    }
    finally {
        Pop-Location
    }
}

Assert-True (Test-Path -LiteralPath $exePath) "Missing Holo executable: $exePath"

Remove-TreeSafe $outputRoot $configDir
New-Item -ItemType Directory -Force -Path $outputRoot | Out-Null

Remove-GeneratedForStem $inputStem $inputDir

Push-Location $repoRoot
try {
    & $exePath --config $configPath --stage all
    if ($LASTEXITCODE -ne 0) {
        throw "Holo exited with code $LASTEXITCODE"
    }
}
finally {
    Pop-Location
}

$expectedMultiviewCount = $viewCount * $viewCount
$firstViewName = "$(Format-PaddedNumber 1 $viewDigits)$(Format-PaddedNumber 1 $viewDigits).jpg"
$lastViewName = "$(Format-PaddedNumber $viewCount $viewDigits)$(Format-PaddedNumber $viewCount $viewDigits).jpg"

$multiviewFiles = @(Get-ChildItem -LiteralPath $multiviewDir -Filter "*.jpg" -File)
Assert-True ($multiviewFiles.Count -eq $expectedMultiviewCount) "Expected $expectedMultiviewCount multiview images, got $($multiviewFiles.Count)"
Assert-True (Test-Path -LiteralPath (Join-Path $multiviewDir $firstViewName)) "Missing first multiview image $firstViewName"
Assert-True (Test-Path -LiteralPath (Join-Path $multiviewDir $lastViewName)) "Missing last multiview image $lastViewName"
$firstViewSize = Get-JpegSize (Join-Path $multiviewDir $firstViewName)
Assert-True ($firstViewSize.Width -eq $resolution -and $firstViewSize.Height -eq $resolution) "Expected multiview image size $resolution`x$resolution, got $($firstViewSize.Width)x$($firstViewSize.Height)"

$expectedElementalCount = $targetRows * $targetCols
$targetRowDigits = Get-DecimalDigits $targetRows
$targetColDigits = Get-DecimalDigits $targetCols
$firstElementalName = "$(Format-PaddedNumber 1 $targetRowDigits)$(Format-PaddedNumber 1 $targetColDigits).jpg"
$lastElementalName = "$(Format-PaddedNumber $targetRows $targetRowDigits)$(Format-PaddedNumber $targetCols $targetColDigits).jpg"

$elementalFiles = @(Get-ChildItem -LiteralPath $elementalDir -Filter "*.jpg" -File)
Assert-True ($elementalFiles.Count -eq $expectedElementalCount) "Expected $expectedElementalCount elemental images, got $($elementalFiles.Count)"
Assert-True (Test-Path -LiteralPath (Join-Path $elementalDir $firstElementalName)) "Missing first elemental image $firstElementalName"
Assert-True (Test-Path -LiteralPath (Join-Path $elementalDir $lastElementalName)) "Missing last elemental image $lastElementalName"
$firstElementalSize = Get-JpegSize (Join-Path $elementalDir $firstElementalName)
Assert-True ($firstElementalSize.Width -eq $viewCount -and $firstElementalSize.Height -eq $viewCount) "Expected elemental image size $viewCount`x$viewCount, got $($firstElementalSize.Width)x$($firstElementalSize.Height)"

Write-Host "[microtest] PASS: $inputStem input -> $expectedMultiviewCount multiview images $resolution`x$resolution and $expectedElementalCount elemental images $viewCount`x$viewCount"
