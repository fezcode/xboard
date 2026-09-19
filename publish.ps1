# Native equivalent of Atelier's self-contained publish.ps1.
param()
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'tools/packaging.ps1')
$root = $PSScriptRoot
$version = Get-XboardVersion $root
& (Join-Path $root 'build.ps1') -Config Release -BuildDir build-release -Test
$buildDir = Join-Path $root 'build-release'
$cache = Get-Content -LiteralPath (Join-Path $buildDir 'CMakeCache.txt') -Raw
if ($cache -notmatch '(?m)^CMAKE_C_COMPILER:FILEPATH=(.+)') { throw 'Cannot locate the configured compiler.' }
$runtimeDir = Split-Path $Matches[1].Trim() -Parent
$objdump = Join-Path $runtimeDir 'objdump.exe'
if (!(Test-Path -LiteralPath $objdump)) { throw "MinGW objdump is required: $objdump" }
$prefix = Split-Path $runtimeDir -Parent
$msysRoot = Split-Path $prefix -Parent
$prefixName = Split-Path $prefix -Leaf
$packageDb = Join-Path $msysRoot 'var/lib/pacman/local'
if (!(Test-Path -LiteralPath $packageDb)) { throw "MSYS2 package metadata is required for notices: $packageDb" }
$publishDir = Reset-XboardOutput $root 'Publish'
Copy-Item -LiteralPath (Join-Path $buildDir 'xboard.exe') -Destination $publishDir
Copy-Item -LiteralPath (Join-Path $root 'README.md') -Destination $publishDir
Copy-Item -LiteralPath (Join-Path $root 'docs') -Destination $publishDir -Recurse
New-Item -ItemType Directory -Path (Join-Path $publishDir 'assets') | Out-Null
Copy-Item -LiteralPath (Join-Path $root 'assets/xboard.png') -Destination (Join-Path $publishDir 'assets')
Copy-Item -LiteralPath (Join-Path $root 'assets/banner.svg') -Destination (Join-Path $publishDir 'assets')
$queue = [System.Collections.Generic.Queue[string]]::new()
$queue.Enqueue((Join-Path $publishDir 'xboard.exe'))
$seen = @{}
$bundled = @{}
while ($queue.Count) {
    $binary = $queue.Dequeue()
    $imports = & $objdump -p $binary
    if ($LASTEXITCODE -ne 0) { throw "Cannot inspect imports: $binary" }
    foreach ($line in $imports) {
        if ($line -notmatch 'DLL Name:\s*(\S+)') { continue }
        $name = $Matches[1]
        if ($seen.ContainsKey($name)) { continue }
        $seen[$name] = $true
        if ($name -match '^(api-ms-|ext-ms-)') { continue }
        $dependency = Join-Path $runtimeDir $name
        if (Test-Path -LiteralPath $dependency) {
            Copy-Item -LiteralPath $dependency -Destination $publishDir
            $bundled[$name] = $dependency
            $queue.Enqueue($dependency)
        } elseif (!(Test-Path -LiteralPath (Join-Path "$env:SystemRoot/System32" $name))) { throw "Unresolved dependency $name imported by $binary" }
    }
}
# Attribute shipped DLLs to installed packages and copy the exact upstream notices.
$owners = @{}
$licenseRoot = Join-Path $publishDir 'licenses'
New-Item -ItemType Directory -Path $licenseRoot | Out-Null
foreach ($package in Get-ChildItem -LiteralPath $packageDb -Directory) {
    $fileList = Join-Path $package.FullName 'files'
    if (!(Test-Path -LiteralPath $fileList)) { continue }
    $entries = @(Get-Content -LiteralPath $fileList)
    $ownedDlls = @($bundled.Keys | Where-Object { $entries -contains "$prefixName/bin/$_" })
    if (!$ownedDlls.Count) { continue }
    $licenseFiles = @($entries | Where-Object { $_ -like "$prefixName/share/licenses/*" -and !($_.EndsWith('/')) })
    if (!$licenseFiles.Count) { throw "No license notices found for $($package.Name). Add them before distributing." }
    $destination = Join-Path $licenseRoot $package.Name
    New-Item -ItemType Directory -Path $destination | Out-Null
    foreach ($entry in $licenseFiles) {
        $relative = $entry.Substring("$prefixName/share/licenses/".Length)
        $target = Join-Path $destination $relative
        New-Item -ItemType Directory -Path (Split-Path $target -Parent) -Force | Out-Null
        Copy-Item -LiteralPath (Join-Path $msysRoot $entry) -Destination $target
    }
    Copy-Item -LiteralPath (Join-Path $package.FullName 'desc') -Destination (Join-Path $destination 'PACKAGE-METADATA.txt')
    foreach ($dll in $ownedDlls) { $owners[$dll] = $package.Name }
}
$inventory = @()
foreach ($dll in ($bundled.Keys | Sort-Object)) {
    if (!$owners.ContainsKey($dll)) { throw "Missing package/license attribution for $dll" }
    $inventory += [PSCustomObject]@{ DLL=$dll; Package=$owners[$dll]; SHA256=(Get-FileHash -LiteralPath (Join-Path $publishDir $dll)).Hash }
}
$inventory | Export-Csv -LiteralPath (Join-Path $publishDir 'DEPENDENCIES.csv') -NoTypeInformation -Encoding UTF8
@(
    'Third-party runtime components', '',
    'This distribution includes unmodified dynamically linked DLLs from MSYS2.',
    'DEPENDENCIES.csv identifies each DLL, package version and SHA-256 hash.',
    'Exact upstream license notices and installed package metadata are in licenses/.',
    'Package metadata includes upstream URLs. Packaging recipes and source references:',
    'https://github.com/msys2/MINGW-packages', '',
    'These notices concern third-party components; they do not grant a license for xboard.',
    'Windows system libraries and fonts are not redistributed.'
) | Set-Content -LiteralPath (Join-Path $publishDir 'THIRD-PARTY-NOTICES.txt') -Encoding UTF8
# Hidden startup/render test without the development runtime on PATH.
$smokeDir = Reset-XboardOutput $root 'build-release/smoke'
$previousPath = $env:PATH
$previousAudio = $env:SDL_AUDIODRIVER
try {
    $env:PATH = "$env:SystemRoot/System32;$env:SystemRoot"
    $env:SDL_AUDIODRIVER = 'dummy'
    # Keep a real process handle: Windows PowerShell 5.1 Start-Process without
    # -Wait can lose ExitCode after a short-lived GUI-subsystem executable exits.
    $process = New-Object System.Diagnostics.Process
    $process.StartInfo.FileName = Join-Path $publishDir 'xboard.exe'
    $process.StartInfo.Arguments = '--screenshot'
    $process.StartInfo.WorkingDirectory = $smokeDir
    $process.StartInfo.UseShellExecute = $false
    $process.StartInfo.CreateNoWindow = $true
    $process.StartInfo.RedirectStandardOutput = $true
    $process.StartInfo.RedirectStandardError = $true
    $null = $process.Start()
    $stdoutTask = $process.StandardOutput.ReadToEndAsync()
    $stderrTask = $process.StandardError.ReadToEndAsync()
    if (!$process.WaitForExit(30000)) { $process.Kill(); $process.WaitForExit(); throw 'Staged app startup timed out.' }
    $stdoutTask.Result | Set-Content -LiteralPath (Join-Path $smokeDir 'startup.stdout.log')
    $stderrTask.Result | Set-Content -LiteralPath (Join-Path $smokeDir 'startup.stderr.log')
    if ($process.ExitCode -ne 0) { throw "Staged app failed (exit $($process.ExitCode)). See $smokeDir" }
    $screenshots = @(Get-ChildItem -LiteralPath $smokeDir -Filter '*.bmp' | Where-Object Length -gt 0)
    if ($screenshots.Count -ne 6) { throw 'Staged app did not render all six screenshots.' }
} finally {
    if ($null -ne $process) { $process.Dispose() }
    $env:PATH = $previousPath
    $env:SDL_AUDIODRIVER = $previousAudio
}
$exeVersion = (Get-Item -LiteralPath (Join-Path $publishDir 'xboard.exe')).VersionInfo.ProductVersion
if ($exeVersion -ne $version) { throw "Executable version $exeVersion does not match $version" }
Write-Host "Published xboard $version with $($bundled.Count) runtime DLLs; clean-PATH smoke test passed." -ForegroundColor Green
