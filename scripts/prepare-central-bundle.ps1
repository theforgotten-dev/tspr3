param([string]$Source = 'java\target\central-publishing\central-bundle.zip',
      [string]$Destination = 'dist\tspr3-0.1.1-central.zip')
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.IO.Compression.FileSystem
$sourcePath = (Resolve-Path -LiteralPath $Source).Path
$destinationPath = [IO.Path]::GetFullPath($Destination)
if ($sourcePath -eq $destinationPath) { throw 'Source and destination must differ.' }
if (Test-Path -LiteralPath $destinationPath) { throw 'Destination already exists; choose a new path.' }
[void][IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($destinationPath))
$inputZip = [IO.Compression.ZipFile]::OpenRead($sourcePath)
try {
    $outputZip = [IO.Compression.ZipFile]::Open($destinationPath, [IO.Compression.ZipArchiveMode]::Create)
    try {
        $names = @{}
        foreach ($entry in $inputZip.Entries) {
            $name = $entry.FullName.Replace('\','/')
            if ($name.StartsWith('/') -or $name.Contains(':') -or $name.Split('/') -contains '..' -or $names.ContainsKey($name)) {
                throw 'Unsafe or duplicate ZIP path.'
            }
            $names[$name] = $true
            $copy = $outputZip.CreateEntry($name)
            $copy.LastWriteTime = $entry.LastWriteTime
            $reader = $entry.Open()
            try {
                $writer = $copy.Open()
                try { $reader.CopyTo($writer) } finally { $writer.Dispose() }
            } finally { $reader.Dispose() }
        }
    } finally { $outputZip.Dispose() }
    $verified = [IO.Compression.ZipFile]::OpenRead($destinationPath)
    try {
        if ($verified.Entries.Count -ne $inputZip.Entries.Count) { throw 'ZIP entry count mismatch.' }
        $hash = [Security.Cryptography.SHA256]::Create()
        try {
            foreach ($entry in $inputZip.Entries) {
                $copy = $verified.GetEntry($entry.FullName.Replace('\','/'))
                $reader = $entry.Open()
                try { $before = [Convert]::ToBase64String($hash.ComputeHash($reader)) } finally { $reader.Dispose() }
                $reader = $copy.Open()
                try { $after = [Convert]::ToBase64String($hash.ComputeHash($reader)) } finally { $reader.Dispose() }
                if ($before -ne $after) { throw 'ZIP payload changed.' }
            }
        } finally { $hash.Dispose() }
        Write-Host "Verified $($verified.Entries.Count) entries; payloads unchanged: $destinationPath"
    } finally { $verified.Dispose() }
} finally { $inputZip.Dispose() }
