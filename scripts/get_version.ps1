#requires -Version 5.1
[CmdletBinding()]
param(
    [switch]$AsObject,
    [switch]$Json
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Invoke-GitCommand {
    param([string[]]$Arguments)

    $output = & git @Arguments 2>$null
    if ($LASTEXITCODE -ne 0) {
        return $null
    }

    if ($null -eq $output) {
        return ""
    }

    return ($output -join "`n").Trim()
}

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot '..')
Push-Location $repoRoot | Out-Null

try {
    $describe = Invoke-GitCommand @('describe', '--tags', '--match', 'v*', '--dirty', '--long')
    if (-not $describe) {
        $describe = Invoke-GitCommand @('describe', '--tags', '--dirty', '--long')
    }

    $commit = Invoke-GitCommand @('rev-parse', 'HEAD')
    $shortCommit = Invoke-GitCommand @('rev-parse', '--short', 'HEAD')
}
finally {
    Pop-Location | Out-Null
}

if (-not $commit) {
    throw "Unable to determine git commit hash. Ensure this script runs inside a git repository."
}

$rawDescribe = $describe
$dirty = $false
$distance = 0
$shaFragment = $shortCommit
$tagComponent = ''

if ([string]::IsNullOrWhiteSpace($describe)) {
    $tagComponent = 'v0.0.0'
}
else {
    if ($describe.EndsWith('-dirty')) {
        $dirty = $true
        $describe = $describe.Substring(0, $describe.Length - 6)
    }

    $parts = $describe -split '-'
    if ($parts.Count -ge 3 -and $parts[-2] -match '^\d+$' -and $parts[-1] -match '^g[0-9a-f]+$') {
        $distance = [int]$parts[-2]
        $shaFragment = $parts[-1].Substring(1)
        $tagComponent = ($parts[0..($parts.Count - 3)] -join '-')
    }
    else {
        $tagComponent = $describe
    }
}

if ($tagComponent.StartsWith('v')) {
    $tagComponent = $tagComponent.Substring(1)
}

if ([string]::IsNullOrWhiteSpace($tagComponent)) {
    $tagComponent = '0.0.0'
}

$version = $tagComponent
if ($distance -gt 0) {
    $version = "$version+$distance.$shaFragment"
}

if ($dirty) {
    $version = "$version.dirty"
}

$fileSafeVersion = ($version -replace '[^0-9A-Za-z\.\-]', '_')

$result = [pscustomobject]@{
    Version = $version
    Tag = $tagComponent
    Distance = $distance
    Commit = $commit
    ShortCommit = $shortCommit
    Describe = $rawDescribe
    Dirty = $dirty
    FileSafeVersion = $fileSafeVersion
}

if ($Json) {
    $result | ConvertTo-Json -Depth 3
}
elseif ($AsObject) {
    return $result
}
else {
    Write-Output $result.Version
}
