#requires -Version 5.1
[CmdletBinding()]
param(
    [string]$Configuration = 'Release',
    [string]$OutputDirectory = 'dist'
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot '..')
$buildRoot = Join-Path $repoRoot "build\$Configuration"

if (-not (Test-Path $buildRoot)) {
    throw "Build output not found for configuration '$Configuration'. Run the build before packaging."
}

$versionInfo = & "$PSScriptRoot\get_version.ps1" -AsObject
$version = $versionInfo.Version
$fileSafeVersion = $versionInfo.FileSafeVersion
$packageName = "RvrseMonitor-$fileSafeVersion"
$stagingRoot = Join-Path $repoRoot "dist\staging\$packageName"

if (Test-Path $stagingRoot) {
    Remove-Item -Path $stagingRoot -Recurse -Force
}
New-Item -ItemType Directory -Path $stagingRoot | Out-Null

function Copy-Artifact {
    param(
        [string]$Source,
        [string]$DestinationRelative
    )

    if (-not (Test-Path $Source)) {
        throw "Required artifact missing: $Source"
    }

    $destinationPath = Join-Path $stagingRoot $DestinationRelative
    $destinationDirectory = Split-Path -Parent $destinationPath
    if ($destinationDirectory -and -not (Test-Path $destinationDirectory)) {
        New-Item -ItemType Directory -Path $destinationDirectory -Force | Out-Null
    }

    Copy-Item -Path $Source -Destination $destinationPath -Force
}

Copy-Artifact -Source (Join-Path $buildRoot 'RvrseMonitorApp.exe') -DestinationRelative 'RvrseMonitorApp.exe'
Copy-Artifact -Source (Join-Path $buildRoot 'RvrseMonitorTests.exe') -DestinationRelative 'RvrseMonitorTests.exe'

$pluginSource = Join-Path $buildRoot 'plugins'
if (Test-Path $pluginSource) {
    Copy-Item -Path $pluginSource -Destination (Join-Path $stagingRoot 'plugins') -Recurse -Force
}

foreach ($doc in @('README.md', 'LICENSE', 'CHANGELOG.md')) {
    $docSource = Join-Path $repoRoot $doc
    if (Test-Path $docSource) {
        Copy-Artifact -Source $docSource -DestinationRelative $doc
    }
}

$versionFile = Join-Path $stagingRoot 'VERSION.txt'
Set-Content -Path $versionFile -Value $version -Encoding UTF8

$checksumEntries = @()
Get-ChildItem -Path $stagingRoot -File -Recurse |
    Sort-Object FullName |
    ForEach-Object {
        $hash = Get-FileHash -Algorithm SHA256 -Path $_.FullName
        $relativePath = $_.FullName.Substring($stagingRoot.Length + 1).Replace('\', '/')
        $checksumEntries += ('{0}  {1}' -f $hash.Hash, $relativePath)
    }
Set-Content -Path (Join-Path $stagingRoot 'SHA256SUMS.txt') -Value $checksumEntries -Encoding ASCII

$outputRoot = Join-Path $repoRoot $OutputDirectory
New-Item -ItemType Directory -Path $outputRoot -Force | Out-Null
$zipPath = Join-Path $outputRoot "$packageName.zip"

if (Test-Path $zipPath) {
    Remove-Item -Path $zipPath -Force
}

Compress-Archive -Path (Join-Path $stagingRoot '*') -DestinationPath $zipPath -Force

$zipHash = Get-FileHash -Algorithm SHA256 -Path $zipPath
$zipChecksumPath = "$zipPath.sha256"
Set-Content -Path $zipChecksumPath -Value ("{0}  {1}" -f $zipHash.Hash, (Split-Path -Leaf $zipPath)) -Encoding ASCII

Remove-Item -Path (Join-Path $repoRoot 'dist\staging') -Recurse -Force

$result = [pscustomobject]@{
    Version = $version
    ZipPath = (Resolve-Path $zipPath).Path
    ZipChecksumPath = (Resolve-Path $zipChecksumPath).Path
}

Write-Host "Packaged $packageName ($Configuration) -> $($result.ZipPath)"
return $result
