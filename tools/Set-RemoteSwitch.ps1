# Developer-only remote switch: edits the config gist of each app through the GitHub API.
# Usage : .\tools\Set-RemoteSwitch.ps1 -App Multi -State On -Message "..."
#         .\tools\Set-RemoteSwitch.ps1 -App Multi -Version 1.0.0 -Notice "..." -DownloadUrl "https://..."
# Apps  : Calc  = 1 (LostArkCalculator, C#)
#         Gem   = 2 (GemFinder, C#)
#         Multi = LostArk MultiTool (C++ / DirectX 11)
# Rules : a field you do not pass is kept as it is.
#         -State Off without -Message also clears MaintenanceMessage.
#         Pass "" to clear Notice or DownloadUrl.
# Token : read only from the GIST_TOKEN environment variable (classic token, "gist" scope only).
# NOTE  : this file is ASCII-only on purpose.
#         Windows PowerShell 5.1 reads BOM-less files as ANSI(CP949), which breaks non-ASCII text.
param(
    [Parameter(Mandatory = $true)][ValidateSet('Calc', 'Gem', 'Multi')] [string]$App,
    [ValidateSet('On', 'Off')] [string]$State,   # IsMaintenance (On = block launch)
    [string]$Message,                            # MaintenanceMessage (empty = app default message)
    [string]$Version,                            # LatestVersion (e.g. 1.0.5)
    [string]$Notice,                             # Notice (empty = no notice)
    [string]$DownloadUrl                         # DownloadUrl (http/https only)
)

$ErrorActionPreference = 'Stop'

# Each app has its own gist
$TARGETS = @{
    Calc  = @{ GistId = 'd075350e67a061d494e47703c717f3d7'; FileName = 'AppConfig.json' }
    Gem   = @{ GistId = '4a0d3aca8c027924c21e7aeff8066369'; FileName = 'GemFinderConfig.json' }
    Multi = @{ GistId = '80a6fde5d092ec7b39aa593cfad348c7'; FileName = 'LostArkMultiToolConfig.json' }
}
$GistId   = $TARGETS[$App].GistId
$FileName = $TARGETS[$App].FileName
$API_URL  = "https://api.github.com/gists/$GistId"

# At least one field must be given
$EDIT_KEYS = 'State', 'Message', 'Version', 'Notice', 'DownloadUrl'
if (-not ($EDIT_KEYS | Where-Object { $PSBoundParameters.ContainsKey($_) })) {
    throw 'Nothing to change. Pass at least one of -State, -Message, -Version, -Notice, -DownloadUrl'
}

# Token is read only from the environment (never write it in this file or in a repository)
$Token = $env:GIST_TOKEN
if (-not $Token) { $Token = [Environment]::GetEnvironmentVariable('GIST_TOKEN', 'User') }   # set by setx, VS not restarted yet
if (-not $Token) { throw 'GIST_TOKEN environment variable is missing. Register it once with: setx GIST_TOKEN "ghp_..."' }

$Headers = @{
    Authorization          = "Bearer $Token"
    Accept                 = 'application/vnd.github+json'
    'X-GitHub-Api-Version' = '2022-11-28'
}

# 1. Read the current config (only the JSON part, in case extra text was pasted around it)
$Gist = Invoke-RestMethod -Uri $API_URL -Headers $Headers
$GistFile = $Gist.files.$FileName
if (-not $GistFile) { throw "Gist $GistId has no file named '$FileName'. Check the file name on the gist page." }

$Raw = [string]$GistFile.content
$Start = $Raw.IndexOf('{')
$End = $Raw.LastIndexOf('}')
if ($Start -lt 0 -or $End -lt $Start) { throw "$FileName does not contain a JSON object. Fix it on the gist page first." }
if ($Start -gt 0 -or $End -lt $Raw.Length - 1) {
    if ($Raw.Substring(0, $Start).Trim() -or $Raw.Substring($End + 1).Trim()) {
        Write-Warning "$FileName had extra text outside the JSON. It will be removed."
    }
}
$Config = $Raw.Substring($Start, $End - $Start + 1) | ConvertFrom-Json

# 2. Change only the fields that were passed (keeps field order, adds the field if missing)
function Set-Field($Target, [string]$Name, $Value) {
    if ($Target.PSObject.Properties[$Name]) { $Target.$Name = $Value }
    else { $Target | Add-Member -NotePropertyName $Name -NotePropertyValue $Value }
}

if ($PSBoundParameters.ContainsKey('State')) {
    Set-Field $Config 'IsMaintenance' ($State -eq 'On')
    if ($State -eq 'Off' -and -not $PSBoundParameters.ContainsKey('Message')) {
        Set-Field $Config 'MaintenanceMessage' ''   # do not leave an old message for the next maintenance
    }
}
if ($PSBoundParameters.ContainsKey('Message')) { Set-Field $Config 'MaintenanceMessage' $Message }
if ($PSBoundParameters.ContainsKey('Notice'))  { Set-Field $Config 'Notice' $Notice }

if ($PSBoundParameters.ContainsKey('Version')) {
    $Parsed = $null
    if (-not [Version]::TryParse($Version.Trim(), [ref]$Parsed)) { throw "Invalid -Version '$Version'. Use a form like 1.0.5" }
    Set-Field $Config 'LatestVersion' $Version.Trim()
}

if ($PSBoundParameters.ContainsKey('DownloadUrl')) {
    $Url = $DownloadUrl.Trim()
    $Uri = $null
    $IsWebUrl = [Uri]::TryCreate($Url, [UriKind]::Absolute, [ref]$Uri) -and ($Uri.Scheme -eq 'https' -or $Uri.Scheme -eq 'http')
    if ($Url -and -not $IsWebUrl) { throw "Invalid -DownloadUrl '$DownloadUrl'. Use an http/https address or """"." }
    Set-Field $Config 'DownloadUrl' $Url
}

# 3. Save (UTF-8 bytes so Korean text is not broken)
$Content = $Config | ConvertTo-Json -Depth 5
$Body = @{ files = @{ $FileName = @{ content = $Content } } } | ConvertTo-Json -Depth 5
Invoke-RestMethod -Method Patch -Uri $API_URL -Headers $Headers `
    -ContentType 'application/json; charset=utf-8' -Body ([Text.Encoding]::UTF8.GetBytes($Body)) | Out-Null

Write-Host "[OK] $App -> $FileName (gist $GistId)"
Write-Host $Content