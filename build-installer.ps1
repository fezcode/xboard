# Adapted from Atelier: publish -> Forge validate -> build -> inspect.
param([string]$Forge = (Join-Path (Split-Path $PSScriptRoot -Parent) 'Forge/build/forge.exe'))
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'tools/packaging.ps1')
$root = $PSScriptRoot
$version = Get-XboardVersion $root
$Forge = [IO.Path]::GetFullPath($Forge)
if (!(Test-Path -LiteralPath $Forge)) { throw "Forge not found: $Forge. Run gobake build in the sibling Forge project." }
if (!(Test-Path -LiteralPath (Join-Path (Split-Path $Forge -Parent) 'uninstall.exe'))) { throw 'Forge standalone uninstall.exe is missing. Rebuild Forge before packaging.' }
Invoke-XboardForge $Forge $root @('--version') 'forge-version' | Out-Null
& (Join-Path $root 'publish.ps1')
Invoke-XboardForge $Forge $root @('validate') 'forge-validate' | Out-Null
$dist = [IO.Path]::GetFullPath((Join-Path $root 'dist'))
if (Test-Path -LiteralPath $dist) {
    if ((Get-Item -LiteralPath $dist -Force).Attributes -band [IO.FileAttributes]::ReparsePoint) { throw 'dist must not be a symlink or junction.' }
    foreach ($stale in Get-ChildItem -LiteralPath $dist -Filter 'xboard-Setup-*.exe' -File) {
        if ((Split-Path ([IO.Path]::GetFullPath($stale.FullName)) -Parent) -ne $dist -or ($stale.Attributes -band [IO.FileAttributes]::ReparsePoint)) { throw 'Unsafe stale installer path.' }
        Remove-Item -LiteralPath $stale.FullName -Force
    }
}
Invoke-XboardForge $Forge $root @('build') 'forge-build' | Out-Null
$installer = Join-Path $dist "xboard-Setup-$version.exe"
if (!(Test-Path -LiteralPath $installer) -or (Get-Item -LiteralPath $installer).Length -le 0) { throw "Forge produced no valid installer at $installer" }
$inspection = Invoke-XboardForge $Forge $root @('inspect', $installer) 'forge-inspect'
$listing = Get-Content -LiteralPath $inspection -Raw
foreach ($required in @('xboard.exe', 'uninstall.exe', 'theme/theme.css', 'SDL2.dll', 'SDL2_ttf.dll', 'THIRD-PARTY-NOTICES.txt')) {
    if (!$listing.Contains($required)) { throw "Installer inspection missing: $required" }
}
$hash = (Get-FileHash -LiteralPath $installer -Algorithm SHA256).Hash
"$hash  xboard-Setup-$version.exe" | Set-Content -LiteralPath "$installer.sha256" -Encoding ASCII
Write-Host "Installer: $installer ($((Get-Item -LiteralPath $installer).Length) bytes)" -ForegroundColor Green
Write-Host "SHA256: $hash"
