#!/usr/bin/env pwsh
# Pre-Push Validation Script for Rvrse Monitor
# Run this before every git push to ensure code quality

param(
    [switch]$SkipUI,
    [switch]$QuickTest
)

$ErrorActionPreference = "Stop"
$script:FailCount = 0
$script:WarnCount = 0

# Colors for output
function Write-Success { Write-Host "✅ $args" -ForegroundColor Green }
function Write-Failure { Write-Host "❌ $args" -ForegroundColor Red; $script:FailCount++ }
function Write-Warning { Write-Host "⚠️  $args" -ForegroundColor Yellow; $script:WarnCount++ }
function Write-Info { Write-Host "ℹ️  $args" -ForegroundColor Cyan }
function Write-Section { Write-Host "`n========== $args ==========" -ForegroundColor Magenta }

# Ensure we're in the repository root
if (-not (Test-Path "RvrseMonitor.sln")) {
    Write-Failure "Not in repository root. Run from d:\Rvrse Monitor\"
    exit 1
}

Write-Host @"
╔═══════════════════════════════════════════════╗
║   Rvrse Monitor - Pre-Push Validation        ║
╚═══════════════════════════════════════════════╝
"@ -ForegroundColor Cyan

# 1. Check Git Status
Write-Section "Git Status Check"
$gitStatus = git status --porcelain
if ($gitStatus) {
    Write-Info "Uncommitted changes detected:"
    git status --short
    Write-Warning "You have uncommitted changes. Commit them before pushing."
} else {
    Write-Success "Working directory clean"
}

# 2. Check for secrets/credentials
Write-Section "Security Check"
$dangerousPatterns = @(
    "password\s*=",
    "api[_-]?key",
    "secret[_-]?key",
    "token\s*=",
    "credentials",
    "Authorization: Bearer"
)

$secretsFound = $false
foreach ($pattern in $dangerousPatterns) {
    $matches = git diff --cached | Select-String -Pattern $pattern -CaseSensitive:$false
    if ($matches) {
        Write-Failure "Potential secret found in staged changes: $pattern"
        $secretsFound = $true
    }
}

if (-not $secretsFound) {
    Write-Success "No obvious secrets detected in staged changes"
}

# 3. Ensure MSBuild is available
Write-Section "Environment Check"
$msbuildPath = "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin"
if (Test-Path $msbuildPath) {
    $env:PATH = "$msbuildPath;$env:PATH"
    Write-Success "MSBuild found and added to PATH"
} else {
    Write-Failure "MSBuild not found at $msbuildPath"
    Write-Info "Install Visual Studio 2022 or update the path in this script"
    exit 1
}

# 4. Debug Build
Write-Section "Debug Build (x64)"
Write-Info "Building Debug configuration..."
$debugBuild = & msbuild RvrseMonitor.sln -p:Configuration=Debug -p:Platform=x64 -m -nologo -verbosity:minimal 2>&1
if ($LASTEXITCODE -ne 0) {
    Write-Failure "Debug build failed"
    Write-Host $debugBuild -ForegroundColor Red
    exit 1
} else {
    Write-Success "Debug build succeeded"
}

# 5. Release Build
Write-Section "Release Build (x64)"
Write-Info "Building Release configuration..."
$releaseBuild = & msbuild RvrseMonitor.sln -p:Configuration=Release -p:Platform=x64 -m -nologo -verbosity:minimal 2>&1
if ($LASTEXITCODE -ne 0) {
    Write-Failure "Release build failed"
    Write-Host $releaseBuild -ForegroundColor Red
    exit 1
} else {
    Write-Success "Release build succeeded"
}

# 6. Unit Tests
Write-Section "Unit Tests"
if (Test-Path "build\Release\RvrseMonitorTests.exe") {
    Write-Info "Running unit tests..."
    Push-Location build\Release
    $testOutput = & .\RvrseMonitorTests.exe 2>&1
    $testExitCode = $LASTEXITCODE
    Pop-Location

    if ($testExitCode -eq 0) {
        Write-Success "All unit tests passed"

        # Parse test output for summary
        $testOutput | Select-String -Pattern "\[PASS\]|\[FAIL\]" | ForEach-Object {
            if ($_ -match "\[PASS\]") {
                Write-Host "  $_" -ForegroundColor Green
            } else {
                Write-Host "  $_" -ForegroundColor Red
            }
        }
    } else {
        Write-Failure "Unit tests failed"
        Write-Host $testOutput -ForegroundColor Red
    }
} else {
    Write-Warning "Unit test executable not found at build\Release\RvrseMonitorTests.exe"
}

# 7. Performance Check
Write-Section "Performance Validation"
$telemetryFiles = Get-ChildItem -Path . -Filter "telemetry_*.json" -Recurse | Sort-Object LastWriteTime -Descending | Select-Object -First 1
if ($telemetryFiles) {
    Write-Info "Checking latest telemetry: $($telemetryFiles.Name)"
    $telemetry = Get-Content $telemetryFiles.FullName | ConvertFrom-Json

    if ($telemetry.metrics) {
        foreach ($metric in $telemetry.metrics) {
            if ($metric.name -eq "ProcessSnapshot") {
                $avgMs = $metric.average_ms
                if ($avgMs -lt 10) {
                    Write-Success "ProcessSnapshot: $avgMs ms (Good)"
                } elseif ($avgMs -lt 20) {
                    Write-Warning "ProcessSnapshot: $avgMs ms (Acceptable, but watch for regressions)"
                } else {
                    Write-Failure "ProcessSnapshot: $avgMs ms (Performance regression! Target: <10ms)"
                }
            }
            if ($metric.name -eq "HandleSnapshot") {
                $avgMs = $metric.average_ms
                if ($avgMs -lt 30) {
                    Write-Success "HandleSnapshot: $avgMs ms (Good)"
                } elseif ($avgMs -lt 50) {
                    Write-Warning "HandleSnapshot: $avgMs ms (Acceptable)"
                } else {
                    Write-Failure "HandleSnapshot: $avgMs ms (Performance regression! Target: <30ms)"
                }
            }
        }
    }
} else {
    Write-Warning "No telemetry file found. Run the Release build to generate metrics."
}

# 8. Binary Size Check
Write-Section "Binary Size Check"
$exePath = "build\Release\RvrseMonitorApp.exe"
if (Test-Path $exePath) {
    $exeSize = (Get-Item $exePath).Length / 1KB
    Write-Info "RvrseMonitorApp.exe: $([math]::Round($exeSize, 2)) KB"

    if ($exeSize -gt 5000) {
        Write-Warning "Executable is quite large (>5 MB). Consider checking for bloat."
    } else {
        Write-Success "Binary size looks reasonable"
    }
} else {
    Write-Warning "Release executable not found"
}

# 9. Quick UI Smoke Test (Optional)
if (-not $SkipUI -and -not $QuickTest) {
    Write-Section "UI Smoke Test (Manual)"
    Write-Info "Please perform manual UI testing:"
    Write-Host "  1. Launch: build\Release\RvrseMonitorApp.exe" -ForegroundColor Yellow
    Write-Host "  2. Verify process list loads" -ForegroundColor Yellow
    Write-Host "  3. Double-click a process to view modules" -ForegroundColor Yellow
    Write-Host "  4. Click 'Handles...' to view handles" -ForegroundColor Yellow
    Write-Host "  5. Check CPU/Memory graphs update" -ForegroundColor Yellow
    Write-Host ""
    $uiTest = Read-Host "Did the UI test pass? (y/n)"

    if ($uiTest -eq "y") {
        Write-Success "UI smoke test passed"
    } else {
        Write-Failure "UI smoke test failed - investigate before pushing"
    }
} else {
    Write-Warning "Skipping UI test (use -SkipUI:$false to enable)"
}

# 10. Final Summary
Write-Section "Validation Summary"
Write-Host ""

if ($script:FailCount -eq 0) {
    Write-Success "All critical checks passed! ✨"
    Write-Host ""
    Write-Host "Ready to push:" -ForegroundColor Green
    Write-Host "  git push origin main" -ForegroundColor Cyan
    Write-Host ""

    if ($script:WarnCount -gt 0) {
        Write-Warning "$($script:WarnCount) warning(s) detected. Review before pushing."
    }

    exit 0
} else {
    Write-Failure "$($script:FailCount) critical issue(s) found!"
    Write-Host ""
    Write-Host "DO NOT PUSH until all issues are resolved." -ForegroundColor Red
    Write-Host ""

    if ($script:WarnCount -gt 0) {
        Write-Warning "$($script:WarnCount) warning(s) also detected."
    }

    exit 1
}
