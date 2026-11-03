function Get-Tspr3CentralAuthorization {
    $username=$env:CENTRAL_TOKEN_USERNAME;$password=$env:CENTRAL_TOKEN_PASSWORD
    if(!$username -or !$password) { throw 'Central credentials are unavailable.' }
    $bytes=[Text.Encoding]::UTF8.GetBytes("$username`:$password")
    try { return @{Authorization='Bearer '+[Convert]::ToBase64String($bytes)} } finally { [Array]::Clear($bytes,0,$bytes.Length) }
}
function Get-Tspr3CentralStatus([Parameter(Mandatory)][string]$DeploymentId) {
    $headers=Get-Tspr3CentralAuthorization
    try { return Invoke-RestMethod -Method Post -Headers $headers -Uri "https://central.sonatype.com/api/v1/publisher/status?id=$DeploymentId" } finally { $headers.Clear() }
}
function Publish-Tspr3CentralDeployment([Parameter(Mandatory)][string]$DeploymentId,[int]$TimeoutSeconds=1800) {
    $current=Get-Tspr3CentralStatus $DeploymentId
    if($current.deploymentState -eq 'VALIDATED') {
        $headers=Get-Tspr3CentralAuthorization
        try { [void](Invoke-WebRequest -UseBasicParsing -Method Post -Headers $headers -Uri "https://central.sonatype.com/api/v1/publisher/deployment/$DeploymentId") } finally { $headers.Clear() }
    } elseif($current.deploymentState -notin @('PUBLISHING','PUBLISHED')) { throw "Deployment cannot be published from state $($current.deploymentState)." }
    $deadline=(Get-Date).AddSeconds($TimeoutSeconds)
    do {
        $current=Get-Tspr3CentralStatus $DeploymentId
        if($current.deploymentState -eq 'PUBLISHED') { return $current }
        if($current.deploymentState -eq 'FAILED') { throw ('Central publication failed: '+($current.errors | ConvertTo-Json -Compress)) }
        Start-Sleep -Seconds 5
    } while((Get-Date) -lt $deadline)
    throw "Central publication timed out in state $($current.deploymentState); inspect the same deployment before retrying."
}
