[CmdletBinding()]
param(
    [string]$OutputDir = (Join-Path $PSScriptRoot 'results'),
    [switch]$BuildOnly
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$source = Join-Path $root 'tools/d3d12_caps.cpp'
$binary = Join-Path $root 'build/d3d12_caps.exe'
New-Item -ItemType Directory -Force -Path (Split-Path $binary -Parent),$OutputDir | Out-Null
if (-not (Test-Path -LiteralPath $source -PathType Leaf)) {
    throw "Source missing: $source"
}

function Find-VsDevCmd {
    $programFilesX86 = [Environment]::GetFolderPath('ProgramFilesX86')
    $locations = @(
        (Join-Path $programFilesX86 'Microsoft Visual Studio/Installer/vswhere.exe'),
        (Join-Path $env:ProgramFiles 'Microsoft Visual Studio/Installer/vswhere.exe')
    ) | Where-Object { $_ -and (Test-Path -LiteralPath $_ -PathType Leaf) }
    foreach ($vswhere in $locations) {
        $installation = (& $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath |
            Select-Object -First 1)
        if ($installation) {
            $devCmd = Join-Path $installation 'Common7/Tools/VsDevCmd.bat'
            if (Test-Path -LiteralPath $devCmd -PathType Leaf) { return $devCmd }
        }
    }
    throw 'Visual Studio C++ tools not found. Install the Desktop development with C++ workload and Windows SDK.'
}

$devCmd = Find-VsDevCmd
# Keep Visual Studio environment and compilation within a single cmd.exe invocation.
# This intentionally builds under build/ (gitignored), not into the game folder.
$compile = ('call "{0}" -arch=x64 -host_arch=x64 >nul && cl /nologo /std:c++17 /EHsc /utf-8 /O2 /MD /Fe:"{1}" "{2}" /link dxgi.lib d3d12.lib' -f $devCmd,$binary,$source)
Write-Host 'Building Direct3D 12 capability probe with Visual Studio MSVC...'
& cmd.exe /d /c $compile
if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $binary)) {
    throw 'MSVC compilation failed. Check Visual Studio C++ tools and Windows SDK.'
}
if ($BuildOnly) {
    Write-Host "Built: $binary"
    return
}

# Running a local native probe does not read or modify any game files.
$jsonLines = @(& $binary)
if ($LASTEXITCODE -ne 0) { throw "D3D12 probe failed with exit code $LASTEXITCODE" }
$json = $jsonLines -join [Environment]::NewLine
$parsed = $json | ConvertFrom-Json -ErrorAction Stop
$report = Join-Path $OutputDir 'd3d12-caps.json'
$json | Set-Content -LiteralPath $report -Encoding utf8
foreach ($gpu in @($parsed.adapters)) {
    Write-Host "GPU: $($gpu.name)"
    Write-Host "  D3D12 device:         $($gpu.d3d12_device)"
    Write-Host "  Feature Level 12_0:   $($gpu.feature_level_12_0)"
    Write-Host "  Shader Model:         $($gpu.shader_model)"
    Write-Host "  Mesh Shader Tier:     $($gpu.mesh_shader_tier)"
    Write-Host "  Wave lanes:           $($gpu.wave_lane_min) - $($gpu.wave_lane_max)"

}
Write-Host "Results (local only): $report"
