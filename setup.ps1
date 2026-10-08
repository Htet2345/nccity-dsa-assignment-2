# Project-local portable GCC/GDB setup for Windows x64.
$ErrorActionPreference = 'Stop'
$toolsDir = Join-Path $PSScriptRoot '.tools'
$compiler = Join-Path $toolsDir 'w64devkit\bin\gcc.exe'
if (Test-Path -LiteralPath $compiler) {
    Write-Host 'Portable GCC is already available.'
    exit 0
}
New-Item -ItemType Directory -Path $toolsDir -Force | Out-Null
$archive = Join-Path $toolsDir 'w64devkit-x64-2.10.0.7z.exe'
$url = 'https://github.com/skeeto/w64devkit/releases/download/v2.10.0/w64devkit-x64-2.10.0.7z.exe'
$expectedHash = '18d0a4c71a166f8401ab6305781bec5882b40b5e06ba9807c61cb5f3b3c6325e'
if (-not (Test-Path -LiteralPath $archive)) {
    Write-Host 'Downloading portable GCC/GDB...'
    $ProgressPreference = 'SilentlyContinue'
    Invoke-WebRequest -Uri $url -OutFile $archive -UseBasicParsing
}
if ((Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash -ne $expectedHash) {
    throw "SHA256 check failed. Remove $archive and rerun setup."
}
Write-Host 'Extracting the verified compiler archive...'
$extract = Start-Process -FilePath $archive -ArgumentList @('-y', ('-o"' + $toolsDir + '"')) -WindowStyle Hidden -Wait -PassThru
if ($extract.ExitCode -ne 0 -or -not (Test-Path -LiteralPath $compiler)) {
    throw 'Compiler extraction failed.'
}
Write-Host 'Setup complete. Run .\build.cmd or press Ctrl+F5 in VS Code.'
