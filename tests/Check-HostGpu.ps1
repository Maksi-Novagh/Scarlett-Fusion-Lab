$ErrorActionPreference = 'Stop'
$folder = Join-Path $env:TEMP ('sfl-gpu-test-' + [guid]::NewGuid().ToString('N'))
try {
    & (Join-Path (Split-Path $PSScriptRoot -Parent) 'Get-HostGpu.ps1') -CpuName 'AMD Ryzen 5 5500U with Radeon Graphics' -GpuName 'AMD Radeon(TM) Graphics' -DriverVersion 'test' -OutputDir $folder
    $r = Get-Content -LiteralPath (Join-Path $folder 'host-gpu-assessment.json') -Raw | ConvertFrom-Json
    if ($r.gpu_architecture -ne 'amd-vega') { throw 'Known Vega GPU was not identified.' }
    if ($r.mesh_shaders -ne 'unsupported-on-this-architecture') { throw 'Vega mesh status was wrong.' }
    if ($r.d3d12_shader_model -ne 'not-probed') { throw 'Must not claim a measured Shader Model.' }
    & (Join-Path (Split-Path $PSScriptRoot -Parent) 'Get-HostGpu.ps1') -CpuName 'Unknown CPU' -GpuName 'Unlisted GPU' -DriverVersion 'test' -OutputDir $folder
    $r = Get-Content -LiteralPath (Join-Path $folder 'host-gpu-assessment.json') -Raw | ConvertFrom-Json
    if ($r.mesh_shaders -ne 'unverified') { throw 'Unknown hardware was classified without evidence.' }
    Write-Host 'Host GPU assessment smoke test passed.'
} finally {
    Remove-Item -LiteralPath $folder -Recurse -Force -ErrorAction SilentlyContinue
}
