param(
    [switch]$Run,
    [switch]$Test,
    [ValidateSet("Debug", "Release", "RelWithDebInfo")]
    [string]$Config = "Release",
    [string]$BuildDir = "build-studio"
)
$ErrorActionPreference = "Stop"
$SourceDir = $PSScriptRoot
if (![System.IO.Path]::IsPathRooted($BuildDir)) { $BuildDir = Join-Path $SourceDir $BuildDir }
cmake -S $SourceDir -B $BuildDir -G Ninja "-DCMAKE_BUILD_TYPE=$Config"
if ($LASTEXITCODE -ne 0) { throw "CMake configuration failed." }
cmake --build $BuildDir --config $Config
if ($LASTEXITCODE -ne 0) { throw "Build failed. Close the executable if Windows has it locked." }
if ($Test) {
    ctest --test-dir $BuildDir --output-on-failure -C $Config
    if ($LASTEXITCODE -ne 0) { throw "Tests failed." }
}
if ($Run) { & (Join-Path $BuildDir "xboard.exe") }
