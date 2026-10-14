param(
    [ValidateSet('all','c','java','tri','log','legacy','maven')][string]$Task='all',
    [switch]$Native,[switch]$Benchmark,[switch]$Sanitize,[switch]$Large
)
$ErrorActionPreference='Stop'
switch($Task) {
    'all' { & "$PSScriptRoot\scripts\build-library.ps1"; & "$PSScriptRoot\scripts\build-java.ps1" -Native }
    'c' { & "$PSScriptRoot\scripts\build-library.ps1" }
    'java' { & "$PSScriptRoot\scripts\build-java.ps1" -Native:$Native -Benchmark:$Benchmark -Large:$Large }
    'tri' { & "$PSScriptRoot\scripts\build-tri.ps1" -Million:$Benchmark -Sanitize:$Sanitize }
    'log' { & "$PSScriptRoot\scripts\validate-log.ps1" -Tests:(!$Benchmark) -Sanitize:$Sanitize }
    'legacy' { & "$PSScriptRoot\scripts\build-legacy.ps1" -Bench:$Benchmark }
    'maven' { & "$PSScriptRoot\scripts\build-maven.ps1" }
}
