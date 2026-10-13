# Prepare the project-local build tools. No administrator rights or global PATH changes.
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$localTools = Join-Path $projectRoot '.tools'
$packRoot = Join-Path $localTools 'toolchain'
New-Item -ItemType Directory -Force $localTools | Out-Null

if (-not (Test-Path -LiteralPath (Join-Path $packRoot 'bin/cmake.exe'))) {
    $archivePath = Join-Path $localTools 'toolchain.zip'
    $packUrl = 'https://github.com/RetroPortingToolKit/RetroPorting-Toolchains/releases/download/v1.0.14/cmake-clang-v1-windows-x64.zip'
    $expectedHash = '28da9742385e7ff875b3d9311e8ed89dbdc84f27b6ecba2bc0d0acc11f6d2b4d'
    if (-not (Test-Path -LiteralPath $archivePath)) {
        Invoke-WebRequest $packUrl -OutFile $archivePath
    }
    if ((Get-FileHash -LiteralPath $archivePath -Algorithm SHA256).Hash.ToLowerInvariant() -ne $expectedHash) {
        throw 'Toolchain archive hash mismatch. Download the pinned archive again.'
    }
    Expand-Archive -LiteralPath $archivePath -DestinationPath $packRoot -Force
}

$env:CARGO_HOME = Join-Path $localTools 'cargo'
$env:RUSTUP_HOME = Join-Path $localTools 'rustup'
$rustupExe = Join-Path $env:CARGO_HOME 'bin/rustup.exe'
if (-not (Test-Path -LiteralPath $rustupExe)) {
    $installerPath = Join-Path $localTools 'rustup-init.exe'
    Invoke-WebRequest 'https://win.rustup.rs/x86_64' -OutFile $installerPath
    & $installerPath -y --no-modify-path --profile minimal `
        --default-host x86_64-pc-windows-gnullvm --default-toolchain 1.97.1
    if ($LASTEXITCODE -ne 0) { throw 'Local Rust installation failed.' }
} else {
    & $rustupExe toolchain install 1.97.1-x86_64-pc-windows-gnullvm --profile minimal
    if ($LASTEXITCODE -ne 0) { throw 'Pinned Rust installation failed.' }
}
Write-Output 'Local tools ready. Run: ./tools/build-windows.ps1 -Regenerate'
