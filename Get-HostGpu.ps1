[CmdletBinding()]
param(
    [string]$CpuName,
    [string]$GpuName,
    [string]$DriverVersion,
    [string]$OutputDir = (Join-Path $PSScriptRoot 'results')
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

if (-not $CpuName) {
    $cpu = Get-CimInstance -ClassName Win32_Processor -ErrorAction Stop |
        Select-Object -First 1
    $CpuName = [string]$cpu.Name
}
if (-not $GpuName) {
    $adapters = @(Get-CimInstance -ClassName Win32_VideoController -ErrorAction Stop)
    if ($adapters.Count -eq 0) { throw 'No video adapter was reported by Windows.' }
    $gpu = @($adapters | Where-Object { $_.Name -notmatch 'Microsoft Basic Display|Remote Display' }) |
        Select-Object -First 1
    if (-not $gpu) { $gpu = $adapters[0] }
    $GpuName = [string]$gpu.Name
    if (-not $DriverVersion) { $DriverVersion = [string]$gpu.DriverVersion }
}

# Architecture identification, NOT a substitute for querying D3D12 feature support.
$architecture = 'unknown'
$identification = 'GPU name alone does not establish architecture.'
if ($GpuName -match 'Vega') {
    $architecture = 'amd-vega'
    $identification = 'Vega is explicitly present in the GPU name.'
} elseif (
    $GpuName -match 'AMD Radeon(\(TM\))? Graphics' -and
    $CpuName -match '\bRyzen\s+5\s+5500U\b'
) {
    $architecture = 'amd-vega'
    $identification = 'Ryzen 5 5500U integrated GPU is Radeon Vega 7.'
}

$mesh = 'unverified'
$details = 'The actual Shader Model and Mesh Shader Tier require a Direct3D 12 device feature query.'
if ($architecture -eq 'amd-vega') {
    $mesh = 'unsupported-on-this-architecture'
    $details = 'Vega predates the RDNA2 hardware mesh-shader generation. Use a Direct3D12 feature query for authoritative device capabilities.'
}

$result = [ordered]@{
    schema_version = 1
    assessed_utc = (Get-Date).ToUniversalTime().ToString('o')
    cpu = $CpuName
    gpu = $GpuName
    driver = $DriverVersion
    gpu_architecture = $architecture
    identification = $identification
    d3d12_shader_model = 'not-probed'
    mesh_shaders = $mesh
    explanation = $details
    note = 'This inventory estimates hardware family from model names; verify capabilities using Direct3D feature queries.'
}
New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null
$path = Join-Path $OutputDir 'host-gpu-assessment.json'
$result | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $path -Encoding utf8
Write-Host ("CPU:                 " + $result.cpu)
Write-Host ("GPU:                 " + $result.gpu)
Write-Host ("Driver:              " + $result.driver)
Write-Host ("Identified GPU:      " + $result.gpu_architecture)
Write-Host ("Mesh shaders:        " + $result.mesh_shaders)
Write-Host ("Shader Model 6.6:    " + $result.d3d12_shader_model)
Write-Host ("Local-only report:   " + $path)
Write-Host $result.explanation
