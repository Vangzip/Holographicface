$ErrorActionPreference = 'Stop'

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot '..')
$mainPath = Join-Path $repoRoot 'point_cloud\main.cpp'
$projectPath = Join-Path $repoRoot 'point_cloud\testreadpcd.vcxproj'
$source = Get-Content -Raw $mainPath
$project = Get-Content -Raw $projectPath

$mergeFuncMatch = [regex]::Match($source, '(?s)void mergeFunc\(.*?\n}\s*\n\s*// Convert point clouds to mesh')
if (-not $mergeFuncMatch.Success) {
    Write-Host 'FAIL: could not locate mergeFunc in point_cloud/main.cpp'
    exit 1
}

$meshFuncMatch = [regex]::Match($source, '(?s)void meshFunc\(.*?\n}\s*\n\s*// Generate textured')
if (-not $meshFuncMatch.Success) {
    Write-Host 'FAIL: could not locate meshFunc in point_cloud/main.cpp'
    exit 1
}

$modelFuncMatch = [regex]::Match($source, '(?s)void modelFunc\(.*?\n}\s*\n\s*int main')
if (-not $modelFuncMatch.Success) {
    Write-Host 'FAIL: could not locate modelFunc in point_cloud/main.cpp'
    exit 1
}

$mergeFunc = $mergeFuncMatch.Value
$meshFunc = $meshFuncMatch.Value
$modelFunc = $modelFuncMatch.Value
$failures = @()

if ($source -notmatch '-merge') {
    $failures += 'command line parser should expose a -merge mode.'
}

if ($source -notmatch '-texture') {
    $failures += 'command line parser should expose a -texture alias for model generation.'
}

if ($mergeFunc -notmatch 'MERGED_POINT_CLOUD_FILE' -or $mergeFunc -notmatch 'writeMergeManifest') {
    $failures += 'mergeFunc should write merged_rgb.ply and merge_manifest.json.'
}

if ($mergeFunc -notmatch 'estimateTransformToMerged') {
    $failures += 'mergeFunc should estimate and record per-view transforms.'
}

if ($meshFunc -match 'getAllSubFiles\s*\([^;]*"_rgb\.ply"') {
    $failures += 'meshFunc must not scan per-view *_rgb.ply files.'
}

$meshApiCallCount = [regex]::Matches($meshFunc, 'meshAPI\s*\(').Count
if ($meshApiCallCount -ne 1) {
    $failures += "meshFunc should call meshAPI exactly once, found $meshApiCallCount call(s)."
}

if ($meshFunc -notmatch 'MERGED_POINT_CLOUD_FILE') {
    $failures += 'meshFunc should consume merged_rgb.ply only.'
}

if ($modelFunc -notmatch 'readMergeManifest' -or $modelFunc -notmatch 'writeMergedTexturedModel') {
    $failures += 'modelFunc should use merge_manifest.json for multi-view texturing.'
}

if ($source -notmatch 'MERGED_MODEL_OBJ_FILE' -or $source -notmatch 'MERGED_MODEL_MTL_FILE' -or $source -notmatch 'MERGED_TEXTURE_FILE') {
    $failures += 'pipeline should produce standard merged OBJ, MTL, and texture outputs.'
}

if ($source -match 'DEBUG-') {
    $failures += 'temporary debug instrumentation should not be committed.'
}

if ($project -match 'D:\\zp\\Holographicface') {
    $failures += 'project file should use HolographicfaceThirdPartyRoot instead of a hard-coded D:\zp path.'
}

if ($failures.Count -gt 0) {
    foreach ($failure in $failures) {
        Write-Host "FAIL: $failure"
    }
    exit 1
}

Write-Host 'PASS: pipeline uses -merge -> merged_rgb.ply/manifest -> one mesh -> manifest-driven texture.'
