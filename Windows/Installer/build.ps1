param (
   [Parameter(Mandatory=$true)]
   [ValidateScript({Test-Path $_ -PathType Leaf})]
   [string]
   $IsccPath
)

Function GetParentDir ([string]$directory, [int]$n)
{
    if ($n -gt 0)
    {
        return GetParentDir (Split-Path $directory) ($n - 1)
    }

    return $directory
}

Function GetRootDir
{
    return GetParentDir $PSScriptRoot 2
}

Function GetChangesPath
{
    return Join-Path (GetRootDir) "Changes.txt"
}

Function GetJsonPath
{
    return Join-Path (GetRootDir) "version.json"
}

Function GetSourceDir
{
    return Join-Path -Path $PSScriptRoot -ChildPath "Source" 
}

Function GetChecksumsPath
{
    return Join-Path -Path $PSScriptRoot -ChildPath "checksums.sha256"
}

Function GetEditorPath
{
    return Join-Path (Join-Path (GetSourceDir) "Editor") "AGSEditor.exe"
}

Function GetEditorVersion
{
    return (Get-Item (GetEditorPath)).VersionInfo.FileVersion
}

Filter CheckJson
{
    $_ | Format-List | Out-String | % { $_.Trim() } | Out-Host

    $_.PSObject.Properties |
        % { if (!$_.Value) { Write-Warning ("Property {0} is empty" -f $_.Name) } }

    if ($_.version -ne (GetEditorVersion))
    {
        Write-Error ("Version mismatch: Editor build needs to be version {0}" -f $_.version)
    }

    $_
}

Filter RenameProperties
{
    $_ |
        Select-Object -Property `
            @{Name = 'AgsFriendlyVersion'; Expression = {$_.versionFriendly}},
            @{Name = 'AgsFullVersion'; Expression = {$_.version}},
            @{Name = 'AgsSpVersion'; Expression = {$_.versionSp}},
            @{Name = 'AgsAppId'; Expression = {$_.appID}}
}

Filter PropertiesAsStrings
{
    $_ |
        % { $_.PSObject.Properties | % { "/D{0}={1}" -f $_.Name, $_.Value } }
}

Function GetBuildArgs
{
    return Get-Content (GetJsonPath) | ConvertFrom-Json | CheckJson |
        RenameProperties | PropertiesAsStrings
}

Function VerifyFile([string]$hash, [string]$path)
{
    Get-FileHash (Resolve-Path (Join-Path $PSScriptRoot $path)) -Algorithm SHA256 |
        % { if ($_.Hash -ne $hash) { Write-Error ("{0}: FAILED" -f $path) }
            else { Write-Host ("{0}: OK" -f $path) } }
}

$ErrorActionPreference = "Stop"

New-Item -ItemType Directory -Path (Join-Path (GetSourceDir) "Docs") -Force |
    % { Copy-Item (GetChangesPath) $_.FullName }

Get-Content (GetChecksumsPath) |
    % { Select-Object -InputObject ($_ -Split "  ", 2, "SimpleMatch") |
        % { VerifyFile $_[0] $_[1] } }

Exit (Start-Process -FilePath $IsccPath `
    -ArgumentList (,"ags.iss" + (GetBuildArgs)) `
    -WorkingDirectory $PSScriptRoot `
    -Wait `
    -NoNewWindow `
    -PassThru).ExitCode
