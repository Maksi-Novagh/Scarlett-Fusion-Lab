[CmdletBinding()]
param(
    [string]$Root = (Split-Path -Parent $PSScriptRoot)
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$rootPath = (Resolve-Path -LiteralPath $Root -ErrorAction Stop).ProviderPath
$files = @(
    Get-ChildItem -LiteralPath $rootPath -File -Filter '*.ps1' -ErrorAction Stop
)
$testsPath = Join-Path $rootPath 'tests'
if (Test-Path -LiteralPath $testsPath -PathType Container) {
    $files += @(Get-ChildItem -LiteralPath $testsPath -File -Filter '*.ps1' -ErrorAction Stop)
}
$files = @($files | Sort-Object FullName -Unique)
if ($files.Count -eq 0) { throw "No project PowerShell scripts found under $rootPath" }

$problems = [System.Collections.Generic.List[string]]::new()
foreach ($file in $files) {
    $tokens = $null
    $parseErrors = $null
    [System.Management.Automation.Language.Parser]::ParseFile(
        $file.FullName, [ref]$tokens, [ref]$parseErrors
    ) | Out-Null
    if ($parseErrors) {
        foreach ($issue in $parseErrors) {
            $relative = [IO.Path]::GetRelativePath($rootPath, $file.FullName)
            $problems.Add(('{0}:{1}:{2}: {3}' -f
                $relative,
                $issue.Extent.StartLineNumber,
                $issue.Extent.StartColumnNumber,
                $issue.Message
            ))
        }
    }
}
if ($problems.Count -gt 0) {
    foreach ($problem in $problems) {
        Write-Error -Message $problem -ErrorAction Continue
    }
    throw "PowerShell syntax validation failed: $($problems.Count) parsing errors across $($files.Count) scripts."
}
Write-Host "PowerShell syntax validation passed: $($files.Count) project scripts parsed."
Write-Host 'No project scripts were executed by this parser check.'
