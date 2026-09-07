Set-StrictMode -Version Latest
function Get-XboardVersion {
    param([string]$Root)
    $header = Get-Content -LiteralPath (Join-Path $Root 'src/app_state.h') -Raw
    if ($header -notmatch '(?m)^#define XBOARD_VERSION "(\d+\.\d+\.\d+)"') { throw 'Invalid XBOARD_VERSION.' }
    $version = $Matches[1]
    $manifest = Get-Content -LiteralPath (Join-Path $Root 'forge.toml') -Raw
    if ($manifest -notmatch '(?ms)^\[app\]\s*(.*?)(?=^\[|\z)') { throw 'Missing Forge [app] section.' }
    $app = $Matches[1]
    if ($app -notmatch '(?m)^version\s*=\s*"([^"]+)"' -or $Matches[1] -ne $version) { throw 'forge.toml version must match XBOARD_VERSION.' }
    return $version
}
function Reset-XboardOutput {
    param([string]$Root, [ValidateSet('Publish', 'build-release/smoke')][string]$Name)
    $rootPath = [IO.Path]::GetFullPath($Root).TrimEnd('\', '/')
    $path = [IO.Path]::GetFullPath((Join-Path $rootPath $Name))
    if (!$path.StartsWith($rootPath + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) { throw "Output escapes project: $path" }
    $ancestor = $path
    while ($ancestor -and $ancestor -ne $rootPath) {
        if (Test-Path -LiteralPath $ancestor) {
            if ((Get-Item -LiteralPath $ancestor -Force).Attributes -band [IO.FileAttributes]::ReparsePoint) { throw "Linked output: $ancestor" }
        }
        $ancestor = Split-Path $ancestor -Parent
    }
    if (Test-Path -LiteralPath $path) {
        $links = @(Get-ChildItem -LiteralPath $path -Recurse -Force | Where-Object { $_.Attributes -band [IO.FileAttributes]::ReparsePoint })
        if ($links.Count) { throw "Output contains links: $path" }
        Remove-Item -LiteralPath $path -Recurse -Force
    }
    New-Item -ItemType Directory -Path $path -Force | Out-Null
    return $path
}
function Invoke-XboardForge {
    param([string]$Forge, [string]$Root, [string[]]$ForgeArgs, [string]$LogName)
    $logDir = Join-Path $Root 'build-release/logs'
    New-Item -ItemType Directory -Path $logDir -Force | Out-Null
    $outFile = Join-Path $logDir "$LogName.stdout.log"
    $errFile = Join-Path $logDir "$LogName.stderr.log"
    $arguments = @($ForgeArgs | ForEach-Object {
        if ($_ -match '["\r\n]') { throw 'Unsupported quote or newline in Forge argument.' }
        '"' + $_ + '"'
    })
    $process = Start-Process -FilePath $Forge -ArgumentList $arguments -WorkingDirectory $Root `
        -WindowStyle Hidden -Wait -PassThru -RedirectStandardOutput $outFile -RedirectStandardError $errFile
    Get-Content -LiteralPath $outFile | ForEach-Object { Write-Host $_ }
    Get-Content -LiteralPath $errFile | ForEach-Object { Write-Host $_ -ForegroundColor Yellow }
    if ($process.ExitCode -ne 0) { throw "Forge $LogName failed (exit $($process.ExitCode)). See $logDir" }
    return $outFile
}
