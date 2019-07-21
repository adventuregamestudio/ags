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

Function GetJsonPath
{
    return Join-Path (GetRootDir) "version.json"
}

Function GetSourceDir
{
    return Join-Path -Path $PSScriptRoot -ChildPath "Source" 
}

Function GetEditorPath
{
    return Join-Path (GetSourceDir) "AGSEditor.exe"
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
        Throw "Version mismatch: Editor build needs to be version {0}" -f $_.version
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

$ErrorActionPreference = "Stop"

Exit (Start-Process -FilePath $IsccPath `
    -ArgumentList (,"ags.iss" + (GetBuildArgs)) `
    -WorkingDirectory $PSScriptRoot `
    -Wait `
    -NoNewWindow `
    -PassThru).ExitCode
