<# 
Barony Windows x64 Setup (PowerShell)

- Uses this repository checkout + vcpkg
- Builds & installs open-source dependencies via vcpkg (x64-windows)
- Validates proprietary SDK folder layout (FMOD / Steamworks / EOS)
- Can auto-build TheoraPlayer from source when enabled
- Provides interactive wizard prompts for feature toggles and install paths (unless -NonInteractive is used)
- Generates a Visual Studio 2022-compatible solution via CMake, builds Release, and runs INSTALL target
- Copies runtime DLLs next to the built .exe (and optionally BARONY_DATADIR)
- Writes a full installer transcript to install.log in the repository root

Run in PowerShell (Windows):
  PS> Set-ExecutionPolicy -Scope Process Bypass
  PS> .\setup_barony_vs2022_x64.ps1

Non-interactive mode is available via: -NonInteractive

You must manually provide licensed SDKs:
  deps\fmod
  deps\steamworks
  deps\eos
#>

[CmdletBinding()]
param(
  # Skips wizard prompts and fails fast on missing inputs/dependencies.
  [switch]$NonInteractive,
  # Retained for backwards compatibility; persistence is enabled by default.
  [switch]$PersistUserEnvironment,
  # Disable persistence of environment variables to user profile.
  [switch]$NoPersistUserEnvironment
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$Script:IsInteractive = (-not $NonInteractive) -and [Environment]::UserInteractive
if ($PersistUserEnvironment.IsPresent -and $NoPersistUserEnvironment.IsPresent) {
  throw "Use either -PersistUserEnvironment or -NoPersistUserEnvironment, not both."
}
$Script:PersistEnvVars = -not $NoPersistUserEnvironment.IsPresent
$Script:PersistSensitiveEnvVars = $false
$Script:OverwriteRuntimeFiles = $true
$Script:InstallLogPath = Join-Path $PSScriptRoot "install.log"
$Script:TranscriptActive = $false
# Original User-scope env values captured before any persisted mutation.
# Used to generate a one-click rollback script for developers.
$Script:PersistedEnvOriginalValues = [ordered]@{}
# Lazily created restore script path (restore-env-YYYYMMDD-HHMMSS.ps1).
$Script:EnvRestoreScriptPath = ""
$Script:EnvRestoreScriptAnnounced = $false

# Unified console logger so wizard output, warnings, and failures are easy to scan.
function Write-Log {
  param(
    [Parameter(Mandatory=$true)][string]$Message,
    [ValidateSet("STEP","INFO","WARN","ERROR","DEBUG")][string]$Level = "INFO"
  )

  $prefix = "[setup:$Level]"
  switch ($Level) {
    "STEP"  { Write-Host "$prefix $Message" -ForegroundColor Cyan }
    "INFO"  { Write-Host "$prefix $Message" -ForegroundColor Gray }
    "WARN"  { Write-Host "$prefix $Message" -ForegroundColor Yellow }
    "ERROR" { Write-Host "$prefix $Message" -ForegroundColor Red }
    "DEBUG" { Write-Host "$prefix $Message" -ForegroundColor DarkGray }
  }
}

# Starts a PowerShell transcript so each installer run is fully captured.
# If install.log is locked, falls back to a timestamped install-*.log path.
function Start-InstallLogging {
  param([Parameter(Mandatory=$true)][string]$LogPath)

  # If a previous run in this same session left a transcript active, stop it first.
  try { Stop-Transcript | Out-Null } catch {}

  $dir = Split-Path -Path $LogPath -Parent
  if ([string]::IsNullOrWhiteSpace($dir)) {
    $dir = $PSScriptRoot
  }
  $timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
  $fallbackPath = Join-Path $dir ("install-{0}-pid{1}.log" -f $timestamp, $PID)
  $pathsToTry = @($LogPath, $fallbackPath)
  $errors = [System.Collections.Generic.List[string]]::new()

  foreach ($path in $pathsToTry) {
    try {
      Start-Transcript -Path $path -Force | Out-Null
      $Script:TranscriptActive = $true
      $Script:InstallLogPath = $path
      if ($path -ne $LogPath) {
        Write-Log -Level WARN -Message "Default install.log was unavailable; logging to fallback file instead: $path"
      }
      Write-Log -Level INFO -Message "Installer output is being logged to: $path"
      return
    } catch {
      [void]$errors.Add(("{0}: {1}" -f $path, $_.Exception.Message))
    }
  }

  throw ("Failed to start installer transcript. Tried paths:`n - {0}" -f ($errors -join "`n - "))
}

# Stops transcript if we started it. Safe to call from finally blocks.
function Stop-InstallLogging {
  if (-not $Script:TranscriptActive) { return }
  try {
    Stop-Transcript | Out-Null
  } catch {
    Write-Warning "Failed to stop installer transcript cleanly: $($_.Exception.Message)"
  } finally {
    $Script:TranscriptActive = $false
  }
}

# Escapes values so generated restore scripts can safely single-quote strings.
function Convert-ToPsSingleQuotedLiteral {
  param(
    [AllowNull()][string]$Value
  )

  if ($null -eq $Value) { return '$null' }
  $escaped = $Value -replace "'", "''"
  return ("'{0}'" -f $escaped)
}

# Creates and caches a timestamped restore script path for this run.
function Get-EnvRestoreScriptPath {
  if (-not [string]::IsNullOrWhiteSpace($Script:EnvRestoreScriptPath)) {
    return $Script:EnvRestoreScriptPath
  }

  $timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
  $Script:EnvRestoreScriptPath = Join-Path $PSScriptRoot ("restore-env-{0}.ps1" -f $timestamp)
  return $Script:EnvRestoreScriptPath
}

# Rewrites the restore script with all captured User-scope pre-change values.
function Update-EnvRestoreScript {
  if ($Script:PersistedEnvOriginalValues.Count -eq 0) {
    return
  }

  # Rewriting keeps the restore script idempotent and reflects latest captured key set.
  $restorePath = Get-EnvRestoreScriptPath
  $lines = [System.Collections.Generic.List[string]]::new()

  [void]$lines.Add("# Auto-generated by setup_barony_vs2022_x64.ps1")
  [void]$lines.Add("# Restores original User-scope environment variables captured before installer changes.")
  [void]$lines.Add("# WARNING: This file may contain sensitive values (tokens/secrets).")
  [void]$lines.Add("# Do NOT commit or share this file.")
  [void]$lines.Add("`$ErrorActionPreference = 'Stop'")
  [void]$lines.Add("")

  foreach ($entry in $Script:PersistedEnvOriginalValues.GetEnumerator()) {
    $name = [string]$entry.Key
    $nameLiteral = Convert-ToPsSingleQuotedLiteral -Value $name
    $hadValue = [bool]$entry.Value.HadValue
    if ($hadValue) {
      $valueLiteral = Convert-ToPsSingleQuotedLiteral -Value ([string]$entry.Value.Value)
      [void]$lines.Add(("[Environment]::SetEnvironmentVariable({0}, {1}, 'User')" -f $nameLiteral, $valueLiteral))
      [void]$lines.Add(("Write-Host 'Restored {0}'" -f ($name -replace "'", "''")))
    } else {
      [void]$lines.Add(("[Environment]::SetEnvironmentVariable({0}, `$null, 'User')" -f $nameLiteral))
      [void]$lines.Add(("Write-Host 'Unset {0}'" -f ($name -replace "'", "''")))
    }
  }

  [void]$lines.Add("")
  [void]$lines.Add("Write-Host 'Environment restore complete. Restart shells to pick up restored values.'")

  Set-Content -Path $restorePath -Value $lines -Encoding UTF8

  if (-not $Script:EnvRestoreScriptAnnounced) {
    Write-Log -Level INFO -Message "Created environment restore script: $restorePath"
    if ($Script:PersistSensitiveEnvVars) {
      Write-Log -Level WARN -Message "Restore script contains persisted sensitive environment values. Keep it private and do not commit it."
    } else {
      Write-Log -Level WARN -Message "Restore script may contain environment values. Keep it private and do not commit it."
    }
    $Script:EnvRestoreScriptAnnounced = $true
  }
}

# Wrapper for external commands with consistent debug logging + exit-code checks.
function Invoke-NativeCommand {
  param(
    [Parameter(Mandatory=$true)][string]$Description,
    [Parameter(Mandatory=$true)][scriptblock]$Command
  )

  Write-Log -Level DEBUG -Message "Executing: $Description"
  & $Command
  $code = $LASTEXITCODE
  if ($null -ne $code -and $code -ne 0) {
    throw "$Description failed with exit code $code"
  }
}

# --------------------------
# USER SETTINGS (edit here)
# --------------------------
# Enable/disable features
$EnableFmod       = $true
$EnableSteamworks = $true
$EnableEos        = $false

# Optional toggles
$EnableCurl         = $true
$EnableOpus         = $true
# PlayFab is intentionally not automated here (SDK + token provisioning is account-specific)
$EnablePlayFab      = $false
$EnableTheoraPlayer = $true
$TheoraPlayerRepoUrl = "https://github.com/AprilAndFriends/theoraplayer.git"
$TheoraPlayerRepoRef = ""

# Optional: set to your Barony assets/data folder (Steam install, etc)
# Example: $BaronyDataDir = "D:\SteamLibrary\steamapps\common\Barony"
$BaronyDataDir = ""
if (-not $BaronyDataDir -and $env:BARONY_DATADIR) {
  $BaronyDataDir = $env:BARONY_DATADIR
}
# If BARONY_DATADIR is not set, prompt to locate a local install using common Steam paths.
$PromptToLocateBaronyDataDir = $true

# --------------------------
# INTERNAL PATHS
# --------------------------
$Root     = $PSScriptRoot
$DepsDir  = Join-Path $Root "deps"
$SrcDir   = $Root
$BuildDir = Join-Path $Root "build-vs2022-x64"

$VcpkgDir        = Join-Path $DepsDir "vcpkg"
$VcpkgTriplet    = "x64-windows"
$VcpkgInstalled  = Join-Path $VcpkgDir "installed\$VcpkgTriplet"
$VcpkgToolchain  = Join-Path $VcpkgDir "scripts\buildsystems\vcpkg.cmake"

# Proprietary SDK locations (you must supply these)
$FmodDir        = Join-Path $DepsDir "fmod"
$SteamworksRoot = Join-Path $DepsDir "steamworks"
$EosRoot        = Join-Path $DepsDir "eos"
$TheoraPlayerRoot = Join-Path $DepsDir "theoraplayer"

function Assert-Command([string]$Name, [string]$Hint) {
  if (-not (Get-Command $Name -ErrorAction SilentlyContinue)) {
    throw "Missing required command '$Name'. $Hint"
  }
}

# Locates a VS 2022-compatible install because this workflow uses that generator.
function Get-VsInstallPath {
  $vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
  if (-not (Test-Path $vswhere)) {
    throw "vswhere.exe not found. Install Visual Studio with C++ build tools."
  }
  # CMake in this workflow uses the VS 2022 generator ("Visual Studio 17 2022").
  # VS 2026 users should install the VS 2022-compatible C++ toolset as well.
  $path = & $vswhere -latest -products * -version "[17.0,18.0)" -requires Microsoft.Component.MSBuild -property installationPath
  if (-not $path) {
    throw "Visual Studio 2022-compatible build tools were not found. Install VS 2022 Build Tools (or VS 2026 with VS 2022-compatible C++ toolset)."
  }
  return $path.Trim()
}

# Imports the VS developer environment variables into the current PowerShell process.
function Import-VsDevCmdEnv {
  param(
    [Parameter(Mandatory=$true)][string]$VsDevCmdBat
  )
  if (-not (Test-Path $VsDevCmdBat)) { throw "VsDevCmd.bat not found at: $VsDevCmdBat" }

  # Run VsDevCmd.bat inside cmd.exe and dump the resulting environment with `set`
  $cmd = "`"$VsDevCmdBat`" -arch=amd64 -host_arch=amd64 >nul && set"
  $output = & cmd.exe /c $cmd

  foreach ($line in $output) {
    $idx = $line.IndexOf('=')
    if ($idx -gt 0) {
      $name  = $line.Substring(0, $idx)
      $value = $line.Substring($idx + 1)
      Set-Item -Path ("Env:{0}" -f $name) -Value $value
    }
  }
}

# Converts Windows paths to CMake-friendly slash style for command-line args.
function Convert-ToCMakePath([string]$Path) {
  return ($Path -replace '\\', '/')
}

# Robust remove helper for temp dirs, even when long paths/readonly files are present.
function Remove-DirectoryIfExists([string]$Path) {
  if (-not (Test-Path $Path)) { return }
  & cmd.exe /c "rmdir /s /q `"$Path`"" | Out-Null
  if (Test-Path $Path) {
    Remove-Item -Recurse -Force $Path
  }
}

# Interactive yes/no helper with stable defaults and non-interactive fallbacks.
function Prompt-YesNo {
  param(
    [Parameter(Mandatory=$true)][string]$Prompt,
    [bool]$DefaultYes = $true
  )

  if (-not $Script:IsInteractive) {
    return $DefaultYes
  }

  $suffix = if ($DefaultYes) { "[Y/n]" } else { "[y/N]" }
  while ($true) {
    $answer = Read-Host "$Prompt $suffix"
    if ([string]::IsNullOrWhiteSpace($answer)) {
      return $DefaultYes
    }
    switch -Regex ($answer.Trim()) {
      '^(y|yes)$' { return $true }
      '^(n|no)$'  { return $false }
      default { Write-Host "Please answer y or n." }
    }
  }
}

# Prompt helper for required text fields.
function Prompt-NonEmptyString {
  param(
    [Parameter(Mandatory=$true)][string]$Prompt,
    [string]$DefaultValue = ""
  )

  if (-not $Script:IsInteractive) {
    return $DefaultValue
  }

  while ($true) {
    $value = Read-Host $Prompt
    if ([string]::IsNullOrWhiteSpace($value)) {
      if (-not [string]::IsNullOrWhiteSpace($DefaultValue)) {
        return $DefaultValue
      }
      Write-Log -Level WARN -Message "A value is required."
      continue
    }
    return $value.Trim()
  }
}

# Prompt helper that normalizes path values and re-prompts invalid input.
function Prompt-Path {
  param(
    [Parameter(Mandatory=$true)][string]$Prompt,
    [string]$DefaultValue = ""
  )

  if (-not $Script:IsInteractive) {
    return $DefaultValue
  }

  while ($true) {
    $inputPath = Read-Host $Prompt
    if ([string]::IsNullOrWhiteSpace($inputPath)) {
      if (-not [string]::IsNullOrWhiteSpace($DefaultValue)) {
        return $DefaultValue
      }
      return ""
    }
    try {
      return [System.IO.Path]::GetFullPath($inputPath.Trim())
    } catch {
      Write-Log -Level WARN -Message "Invalid path format. Try again."
    }
  }
}

# Sets process env vars immediately and optionally persists to User scope.
# When persisting, snapshots the original User value once so rollback stays accurate.
function Set-EnvVarValue {
  param(
    [Parameter(Mandatory=$true)][string]$Name,
    [Parameter(Mandatory=$true)][string]$Value,
    [bool]$Persist = $false,
    [bool]$Sensitive = $false
  )

  if ([string]::IsNullOrWhiteSpace($Value)) {
    return
  }

  Set-Item -Path ("Env:{0}" -f $Name) -Value $Value
  if (-not $Persist) {
    Write-Log -Level DEBUG -Message "Set session env var: $Name"
    return
  }

  $currentUserValue = [Environment]::GetEnvironmentVariable($Name, "User")
  if ($null -ne $currentUserValue -and $currentUserValue -ceq $Value) {
    # No-op writes are skipped so restore script only tracks actual mutations.
    Write-Log -Level DEBUG -Message "User env var already matches desired value: $Name"
    return
  }

  if (-not $Script:PersistedEnvOriginalValues.Contains($Name)) {
    $Script:PersistedEnvOriginalValues[$Name] = @{
      HadValue = ($null -ne $currentUserValue)
      Value = $currentUserValue
    }
  }

  [Environment]::SetEnvironmentVariable($Name, $Value, "User")
  Update-EnvRestoreScript
  if ($Sensitive) {
    Write-Log -Level INFO -Message "Persisted user env var: $Name (sensitive value hidden)"
  } else {
    Write-Log -Level INFO -Message "Persisted user env var: $Name"
  }
}

# Reads the most relevant visible env value: process first, then user.
function Get-CurrentEnvValue {
  param(
    [Parameter(Mandatory=$true)][string]$Name
  )

  $processValue = [Environment]::GetEnvironmentVariable($Name, "Process")
  if (-not [string]::IsNullOrWhiteSpace($processValue)) {
    return $processValue
  }

  $userValue = [Environment]::GetEnvironmentVariable($Name, "User")
  if (-not [string]::IsNullOrWhiteSpace($userValue)) {
    return $userValue
  }

  return ""
}

# Appends current env value context to wizard prompts.
function Format-EnvPrompt {
  param(
    [Parameter(Mandatory=$true)][string]$Prompt,
    [Parameter(Mandatory=$true)][string]$EnvName
  )

  $current = Get-CurrentEnvValue -Name $EnvName
  if ([string]::IsNullOrWhiteSpace($current)) {
    $current = "<unset>"
  }
  return ("{0} (current {1}={2})" -f $Prompt, $EnvName, $current)
}

# Interactive feature wizard: captures toggles and behavior flags, not SDK paths.
function Prompt-FeatureConfiguration {
  if (-not $Script:IsInteractive) {
    Write-Log -Level INFO -Message "Running in non-interactive mode; using script defaults for feature toggles."
    return
  }

  Write-Log -Level STEP -Message "Feature Configuration Wizard"
  Write-Host "Press Enter to accept defaults."

  $script:EnableFmod = Prompt-YesNo `
    -Prompt (Format-EnvPrompt -Prompt "Enable FMOD integration?" -EnvName "FMOD_DIR") `
    -DefaultYes $EnableFmod
  $script:EnableSteamworks = Prompt-YesNo `
    -Prompt (Format-EnvPrompt -Prompt "Enable Steamworks integration?" -EnvName "STEAMWORKS_ENABLED") `
    -DefaultYes $EnableSteamworks
  $script:EnableEos = Prompt-YesNo `
    -Prompt (Format-EnvPrompt -Prompt "Enable EOS integration?" -EnvName "EOS_ENABLED") `
    -DefaultYes $EnableEos
  $script:EnableCurl = Prompt-YesNo `
    -Prompt (Format-EnvPrompt -Prompt "Enable CURL + OpenSSL integration?" -EnvName "CURL_ENABLED") `
    -DefaultYes $EnableCurl
  $script:EnableOpus = Prompt-YesNo `
    -Prompt (Format-EnvPrompt -Prompt "Enable Opus integration?" -EnvName "OPUS_ENABLED") `
    -DefaultYes $EnableOpus
  $script:EnableTheoraPlayer = Prompt-YesNo `
    -Prompt (Format-EnvPrompt -Prompt "Enable TheoraPlayer integration?" -EnvName "THEORAPLAYER_ENABLED") `
    -DefaultYes $EnableTheoraPlayer
  if ($EnablePlayFab) {
    Write-Log -Level WARN -Message "PlayFab automation is not implemented by this script; forcing PlayFab OFF."
  }
  $script:EnablePlayFab = $false

  if ($EnableTheoraPlayer) {
    $useCustomRepo = Prompt-YesNo -Prompt "Use custom TheoraPlayer repo/ref?" -DefaultYes $false
    if ($useCustomRepo) {
      $script:TheoraPlayerRepoUrl = Prompt-NonEmptyString -Prompt "TheoraPlayer repo URL" -DefaultValue $TheoraPlayerRepoUrl
      $script:TheoraPlayerRepoRef = Read-Host "Optional TheoraPlayer git ref (blank for default)"
    }
  }

  # BARONY_DATADIR handling happens in Resolve-BaronyDataDir; show context here only.
  $currentDataDir = Get-CurrentEnvValue -Name "BARONY_DATADIR"
  if ([string]::IsNullOrWhiteSpace($currentDataDir)) {
    Write-Log -Level INFO -Message "Current BARONY_DATADIR: <unset>"
  } else {
    Write-Log -Level INFO -Message "Current BARONY_DATADIR: $currentDataDir"
  }

  $script:OverwriteRuntimeFiles = Prompt-YesNo -Prompt "Overwrite existing files when copying runtime outputs?" -DefaultYes $Script:OverwriteRuntimeFiles

  $persistDefault = if ($Script:PersistEnvVars) { "enabled" } else { "disabled" }
  $script:PersistEnvVars = Prompt-YesNo `
    -Prompt ("Persist standard environment variables for future shells? (default currently {0})" -f $persistDefault) `
    -DefaultYes $Script:PersistEnvVars
  if ($EnableEos) {
    $script:PersistSensitiveEnvVars = Prompt-YesNo -Prompt "Persist EOS token environment variables (sensitive)?" -DefaultYes $false
  }
}

# Adds normalized paths into case-insensitive sets used for candidate discovery.
function Add-UniquePath {
  param(
    [Parameter(Mandatory=$true)][AllowEmptyCollection()][System.Collections.Generic.HashSet[string]]$Set,
    [string]$Path
  )

  if ([string]::IsNullOrWhiteSpace($Path)) { return }
  try {
    $normalized = [System.IO.Path]::GetFullPath(($Path -replace '/', '\')).TrimEnd('\')
    [void]$Set.Add($normalized)
  } catch {
    # Ignore malformed candidate paths.
  }
}

# Guards against accidentally treating this source checkout as BARONY_DATADIR.
function Test-IsRepositoryPath {
  param([string]$Path)

  if ([string]::IsNullOrWhiteSpace($Path)) { return $false }

  try {
    $candidate = [System.IO.Path]::GetFullPath(($Path -replace '/', '\')).TrimEnd('\')
    $repoRoot  = [System.IO.Path]::GetFullPath(($Root -replace '/', '\')).TrimEnd('\')
    return $candidate.Equals($repoRoot, [System.StringComparison]::OrdinalIgnoreCase)
  } catch {
    return $false
  }
}

# Heuristic for a plausible Barony runtime folder.
function Test-BaronyDataDirCandidate {
  param([string]$Path)

  if ([string]::IsNullOrWhiteSpace($Path)) { return $false }
  if (-not (Test-Path -Path $Path -PathType Container)) { return $false }
  if (Test-IsRepositoryPath -Path $Path) { return $false }

  $hasExe  = Test-Path (Join-Path $Path "barony.exe")
  $hasLang = (Test-Path (Join-Path $Path "lang\en.txt")) -or (Test-Path (Join-Path $Path "lang"))
  $hasData = (Test-Path (Join-Path $Path "data")) -or (Test-Path (Join-Path $Path "books"))

  return (($hasExe -and $hasLang) -or ($hasExe -and $hasData))
}

# Lower rank means higher preference when multiple install candidates exist.
function Get-BaronyDataDirCandidateRank {
  param([string]$Path)

  if ([string]::IsNullOrWhiteSpace($Path)) { return 9999 }
  $normalized = ($Path -replace '/', '\').ToLowerInvariant()

  if ($normalized -match '\\steamapps\\common\\barony$') { return 0 }
  if ($normalized -match '\\steamapps\\common\\barony')  { return 1 }
  if ($normalized -match '\\steamapps\\common\\')        { return 2 }
  if ($normalized -match '\\steam')                      { return 3 }

  return 100
}

# Collects Steam roots from common install paths + Steam registry/library config.
function Get-SteamLibraryRoots {
  $roots = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::OrdinalIgnoreCase)

  $programFilesX86 = ${env:ProgramFiles(x86)}
  $programFiles = $env:ProgramFiles
  $programW6432 = $env:ProgramW6432

  if (-not [string]::IsNullOrWhiteSpace($programFilesX86)) {
    Add-UniquePath -Set $roots -Path (Join-Path $programFilesX86 "Steam")
  }
  if (-not [string]::IsNullOrWhiteSpace($programFiles)) {
    Add-UniquePath -Set $roots -Path (Join-Path $programFiles "Steam")
  }
  if (-not [string]::IsNullOrWhiteSpace($programW6432)) {
    Add-UniquePath -Set $roots -Path (Join-Path $programW6432 "Steam")
  }

  try {
    $steamPath = (Get-ItemProperty -Path "HKCU:\Software\Valve\Steam" -Name SteamPath -ErrorAction Stop).SteamPath
    Add-UniquePath -Set $roots -Path $steamPath
  } catch {
    # Registry key is optional; continue with default paths.
  }

  # Parse known Steam roots for configured library folders.
  foreach ($steamRoot in @($roots)) {
    $libraryVdf = Join-Path $steamRoot "steamapps\libraryfolders.vdf"
    if (-not (Test-Path $libraryVdf)) { continue }

    foreach ($line in (Get-Content $libraryVdf -ErrorAction SilentlyContinue)) {
      $match = [regex]::Match($line, '"path"\s*"([^"]+)"')
      if (-not $match.Success) { continue }

      $libraryPath = $match.Groups[1].Value -replace '\\\\', '\'
      Add-UniquePath -Set $roots -Path $libraryPath
    }
  }

  return @($roots)
}

# Returns ranked candidate install dirs likely to contain a usable BARONY_DATADIR.
function Get-BaronyDataDirCandidates {
  $candidates = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::OrdinalIgnoreCase)

  foreach ($libraryRoot in (Get-SteamLibraryRoots)) {
    $commonDir = Join-Path $libraryRoot "steamapps\common"
    Add-UniquePath -Set $candidates -Path (Join-Path $commonDir "Barony")

    if (Test-Path $commonDir) {
      foreach ($dir in (Get-ChildItem -Path $commonDir -Directory -ErrorAction SilentlyContinue | Where-Object { $_.Name -like "Barony*" })) {
        Add-UniquePath -Set $candidates -Path $dir.FullName
      }
    }
  }

  foreach ($drive in (Get-PSDrive -PSProvider FileSystem -ErrorAction SilentlyContinue)) {
    Add-UniquePath -Set $candidates -Path (Join-Path $drive.Root "SteamLibrary\steamapps\common\Barony")
    Add-UniquePath -Set $candidates -Path (Join-Path $drive.Root "Games\SteamLibrary\steamapps\common\Barony")
    Add-UniquePath -Set $candidates -Path (Join-Path $drive.Root "Steam\steamapps\common\Barony")
  }

  $matches = [System.Collections.Generic.List[string]]::new()
  foreach ($candidate in $candidates) {
    if (Test-IsRepositoryPath -Path $candidate) {
      Write-Log -Level DEBUG -Message "Skipping repository path candidate for BARONY_DATADIR: $candidate"
      continue
    }
    if (Test-BaronyDataDirCandidate -Path $candidate) {
      [void]$matches.Add($candidate)
    }
  }

  return @(
    $matches |
      Sort-Object -Property `
        @{ Expression = { Get-BaronyDataDirCandidateRank -Path $_ }; Ascending = $true }, `
        @{ Expression = { $_ }; Ascending = $true } `
      -Unique
  )
}

# Resolves BARONY_DATADIR with explicit wizard flow:
# 1) If currently set, ask whether to change it.
# 2) If unset or changing, ask whether to auto-detect.
# 3) If auto-detect is enabled, show found candidates and confirm each.
# 4) If not accepted or not detected, prompt for manual path.
function Resolve-BaronyDataDir {
  param(
    [string]$ConfiguredPath,
    [bool]$PromptToLocate = $true
  )

  # Manual path prompt is intentionally centralized so all fallback paths behave the same.
  $promptManual = {
    if (-not $Script:IsInteractive) { return "" }
    $manual = Prompt-Path -Prompt "Enter BARONY_DATADIR manually (blank to skip)"
    if ($manual -and (Test-IsRepositoryPath -Path $manual)) {
      Write-Log -Level WARN -Message "Manual BARONY_DATADIR points to repository root. Use your Steam install directory instead."
      return ""
    }
    if ($manual -and (Test-Path -Path $manual -PathType Container)) {
      return $manual
    }
    if ($manual) {
      Write-Log -Level WARN -Message "Manual BARONY_DATADIR path does not exist: $manual"
    }
    return ""
  }

  # Start from user-provided/configured value (script setting or existing env var).
  $selectedPath = ""
  if (-not [string]::IsNullOrWhiteSpace($ConfiguredPath)) {
    try {
      $selectedPath = [System.IO.Path]::GetFullPath($ConfiguredPath).TrimEnd('\')
    } catch {
      $selectedPath = $ConfiguredPath.Trim()
    }
  }

  # Step 1: If BARONY_DATADIR is set, ask whether to change it.
  if (-not [string]::IsNullOrWhiteSpace($selectedPath) -and $Script:IsInteractive) {
    $changeDefaultYes = $false
    if ((Test-IsRepositoryPath -Path $selectedPath) -or (-not (Test-Path -Path $selectedPath -PathType Container))) {
      $changeDefaultYes = $true
    }

    $changeConfigured = Prompt-YesNo `
      -Prompt ("BARONY_DATADIR is currently set to '{0}'. Change it?" -f $selectedPath) `
      -DefaultYes $changeDefaultYes

    if (-not $changeConfigured) {
      if (Test-IsRepositoryPath -Path $selectedPath) {
        Write-Log -Level WARN -Message "BARONY_DATADIR points to repository root; keeping it because you chose not to change it."
      } elseif (-not (Test-Path -Path $selectedPath -PathType Container)) {
        Write-Log -Level WARN -Message "BARONY_DATADIR does not exist; keeping it because you chose not to change it: $selectedPath"
      }
      return $selectedPath
    }

    $selectedPath = ""
  } elseif (-not [string]::IsNullOrWhiteSpace($selectedPath)) {
    # Non-interactive mode cannot prompt; prefer a valid configured value.
    if (Test-IsRepositoryPath -Path $selectedPath) {
      Write-Log -Level WARN -Message "Configured BARONY_DATADIR points to repository root and will be ignored."
      $selectedPath = ""
    } elseif (-not (Test-Path -Path $selectedPath -PathType Container)) {
      Write-Log -Level WARN -Message "Configured BARONY_DATADIR does not exist and will be ignored: $selectedPath"
      $selectedPath = ""
    } else {
      return $selectedPath
    }
  }

  # Step 2: If unset/changing, ask whether to auto-detect.
  $shouldSearch = $PromptToLocate
  if ($Script:IsInteractive) {
    $shouldSearch = Prompt-YesNo -Prompt "Would you like the installer to find BARONY_DATADIR automatically?" -DefaultYes $PromptToLocate
  }

  # Step 3: If auto-detect was selected, search and confirm candidates.
  if ($shouldSearch) {
    # Normalize to a real array to avoid strict-mode property errors on single/null results.
    $matches = @(
      Get-BaronyDataDirCandidates |
        Where-Object { -not [string]::IsNullOrWhiteSpace([string]$_) }
    )
    if ($matches.Length -eq 0) {
      Write-Log -Level WARN -Message "Could not auto-locate a Barony install from common Steam paths."
    } elseif (-not $Script:IsInteractive) {
      # Non-interactive runs cannot confirm; choose highest-ranked candidate.
      return $matches[0]
    } else {
      foreach ($candidate in $matches) {
        Write-Log -Level INFO -Message "Auto-detected BARONY_DATADIR candidate: $candidate"
        $acceptCandidate = Prompt-YesNo -Prompt ("Use this BARONY_DATADIR? '{0}'" -f $candidate) -DefaultYes $true
        if ($acceptCandidate) {
          return $candidate
        }
      }
      Write-Log -Level WARN -Message "No auto-detected BARONY_DATADIR candidate was accepted."
    }
  } else {
    Write-Log -Level INFO -Message "Skipping BARONY_DATADIR auto-detection by user choice."
  }

  # Step 4: If auto-detect was skipped/failed/rejected, prompt manual entry.
  $manualPath = & $promptManual
  if ($manualPath) { return $manualPath }

  return ""
}

# Re-check loop for SDKs that must be manually downloaded/placed by the developer.
function Ensure-ManualDependency {
  param(
    [Parameter(Mandatory=$true)][string]$Name,
    [Parameter(Mandatory=$true)][scriptblock]$CheckAction,
    [string[]]$ExpectedPaths = @(),
    [string]$DownloadUrl = ""
  )

  while ($true) {
    try {
      & $CheckAction
      Write-Log -Level INFO -Message "$Name dependency check passed."
      return
    } catch {
      $message = $_.Exception.Message
      Write-Log -Level WARN -Message "$Name dependency check failed: $message"
      if ($ExpectedPaths.Count -gt 0) {
        Write-Host ("Expected paths for {0}:" -f $Name)
        foreach ($p in $ExpectedPaths) {
          Write-Host "  - $p"
        }
      }
      if ($DownloadUrl) {
        Write-Host "Download: $DownloadUrl"
      }

      if (-not $Script:IsInteractive) {
        throw "Missing required dependency '$Name' in non-interactive mode."
      }

      $retry = Prompt-YesNo -Prompt "Place the missing $Name files and retry now?" -DefaultYes $true
      if (-not $retry) {
        throw "Aborted while waiting for $Name dependency files."
      }
    }
  }
}

# Ensures local vcpkg checkout exists for deterministic dependency installation.
function Ensure-Vcpkg {
  if (-not (Test-Path (Join-Path $VcpkgDir "vcpkg.exe"))) {
    Write-Log -Level STEP -Message "Bootstrapping vcpkg in $VcpkgDir"
    Invoke-NativeCommand -Description "git clone vcpkg" -Command { & git clone --depth 1 https://github.com/microsoft/vcpkg $VcpkgDir }
    Invoke-NativeCommand -Description "bootstrap-vcpkg.bat" -Command { & cmd.exe /c "`"$VcpkgDir\bootstrap-vcpkg.bat`" -disableMetrics" }
  } else {
    Write-Log -Level INFO -Message "vcpkg already present: $VcpkgDir"
  }
}

# Installs open-source dependencies required by enabled features.
function Vcpkg-Install {
  $pkgs = @(
    "sdl2", "sdl2-image", "sdl2-net", "sdl2-ttf",
    "physfs", "rapidjson", "libpng", "zlib", "dirent", "glew"
  )
  if ($EnableSteamworks) { $pkgs += "nativefiledialog-extended" }
  if ($EnableCurl) { $pkgs += @("openssl", "curl[openssl]") }
  if ($EnableOpus) { $pkgs += "opus" }
  if ($EnableTheoraPlayer) { $pkgs += @("libogg", "libvorbis", "libtheora") }
  if ($EnablePlayFab) { throw "PlayFab support is not automated by this script. Keep `$EnablePlayFab = `$false unless you have a separate PlayFab SDK/token setup." }

  Write-Log -Level STEP -Message "Installing open-source dependencies via vcpkg ($VcpkgTriplet)"
  Write-Log -Level INFO -Message "vcpkg install list: $($pkgs -join ' ')"

  Invoke-NativeCommand -Description "vcpkg install" -Command { & "$VcpkgDir\vcpkg.exe" install @pkgs --triplet $VcpkgTriplet }

  if (-not (Test-Path (Join-Path $VcpkgInstalled "include"))) { throw "vcpkg include/ missing at $VcpkgInstalled" }
  if (-not (Test-Path (Join-Path $VcpkgInstalled "lib")))     { throw "vcpkg lib/ missing at $VcpkgInstalled" }
  Write-Log -Level INFO -Message "vcpkg dependency installation complete."
}

# Validates FMOD folder layout and exports FMOD_DIR for configure-time discovery.
function Check-Fmod {
  Write-Log -Level DEBUG -Message "Checking FMOD layout in $FmodDir"
  $hdr = Join-Path $FmodDir "api\core\inc\fmod.hpp"
  if (-not (Test-Path $hdr)) {
    throw "FMOD headers not found at $hdr. Extract the FMOD Core API to $FmodDir"
  }

  # Patch common Windows folder mismatch (FMOD often has x64; Barony expects x86_64)
  $libX64   = Join-Path $FmodDir "api\core\lib\x64"
  $libX8664 = Join-Path $FmodDir "api\core\lib\x86_64"
  if ((Test-Path $libX64) -and (-not (Test-Path $libX8664))) {
    Write-Log -Level INFO -Message "Mirroring FMOD lib path api\core\lib\x64 -> api\core\lib\x86_64"
    New-Item -ItemType Directory -Force -Path $libX8664 | Out-Null
    & robocopy $libX64 $libX8664 *.* /E /NFL /NDL /NJH /NJS | Out-Null
  }

  $lib = Join-Path $libX8664 "fmod_vc.lib"
  if (-not (Test-Path $lib)) { throw "FMOD import library not found: $lib" }

  $env:FMOD_DIR = $FmodDir
}

# Validates Steamworks headers/libs and exports STEAMWORKS_ROOT.
function Check-Steamworks {
  Write-Log -Level DEBUG -Message "Checking Steamworks layout in $SteamworksRoot"
  $hdr = Join-Path $SteamworksRoot "sdk\public\steam\steam_api.h"
  $lib = Join-Path $SteamworksRoot "sdk\redistributable_bin\win64\steam_api64.lib"
  if (-not (Test-Path $hdr)) { throw "Steamworks headers not found: $hdr" }
  if (-not (Test-Path $lib)) { throw "Steamworks import library not found: $lib" }
  $env:STEAMWORKS_ROOT = $SteamworksRoot
}

# Ensures steam_appid.txt exists for local runtime validation outside Steam client.
function Ensure-SteamAppIdFile {
  $appidPath = Join-Path $SrcDir "steam_appid.txt"
  if (Test-Path $appidPath) {
    Write-Log -Level DEBUG -Message "Using existing steam_appid.txt from repository root."
    return $appidPath
  }

  $mainHeader = Join-Path $SrcDir "src\main.hpp"
  if (-not (Test-Path $mainHeader)) {
    Write-Warning "Could not locate $mainHeader to derive STEAM_APPID."
    return $null
  }

  $match = Select-String -Path $mainHeader -Pattern '^\s*#define\s+STEAM_APPID\s+(\d+)' | Select-Object -First 1
  if (-not $match) {
    Write-Warning "Could not derive STEAM_APPID from $mainHeader."
    return $null
  }

  $steamAppId = $match.Matches[0].Groups[1].Value
  Set-Content -Path $appidPath -Value $steamAppId -Encoding ASCII
  Write-Log -Level INFO -Message "Created steam_appid.txt with appid $steamAppId"
  return $appidPath
}

# Validates EOS SDK layout and exports EOS_ROOT.
function Check-Eos {
  Write-Log -Level DEBUG -Message "Checking EOS SDK layout in $EosRoot"
  $hdr = Join-Path $EosRoot "SDK\Include\eos_sdk.h"
  $lib = Join-Path $EosRoot "SDK\Lib\EOSSDK-Win64-Shipping.lib"
  if (-not (Test-Path $hdr)) { throw "EOS headers not found: $hdr" }
  if (-not (Test-Path $lib)) { throw "EOS import library not found: $lib" }
  $env:EOS_ROOT = $EosRoot
}

# Collects required EOS token vars when EOS is enabled.
function Ensure-EosTokens {
  Write-Log -Level STEP -Message "Collecting required EOS token environment variables."

  if (-not $Script:IsInteractive) {
    foreach ($k in "BUILD_ENV_PR","BUILD_ENV_SA","BUILD_ENV_DE","BUILD_ENV_CC","BUILD_ENV_CS","BUILD_ENV_GSE") {
      if (-not ${env:$k}) {
        throw "EOS is enabled in non-interactive mode, but missing environment variable '$k'."
      }
    }
    Write-Log -Level INFO -Message "EOS token variables already present in environment."
    return
  }

  # Barony's CMakeLists will error if EOS_ENABLED=1 and any of these are empty.
  if (-not $env:BUILD_ENV_PR) { $env:BUILD_ENV_PR = Read-Host "EOS BUILD_ENV_PR (Product ID)" }
  if (-not $env:BUILD_ENV_SA) { $env:BUILD_ENV_SA = Read-Host "EOS BUILD_ENV_SA (Sandbox ID)" }
  if (-not $env:BUILD_ENV_DE) { $env:BUILD_ENV_DE = Read-Host "EOS BUILD_ENV_DE (Deployment ID)" }
  if (-not $env:BUILD_ENV_CC) { $env:BUILD_ENV_CC = Read-Host "EOS BUILD_ENV_CC (Client ID)" }

  if (-not $env:BUILD_ENV_CS) {
    $sec = Read-Host "EOS BUILD_ENV_CS (Client Secret)" -AsSecureString
    $bstr = [Runtime.InteropServices.Marshal]::SecureStringToBSTR($sec)
    try { $env:BUILD_ENV_CS = [Runtime.InteropServices.Marshal]::PtrToStringBSTR($bstr) }
    finally { [Runtime.InteropServices.Marshal]::ZeroFreeBSTR($bstr) }
  }

  if (-not $env:BUILD_ENV_GSE) { $env:BUILD_ENV_GSE = Read-Host "EOS BUILD_ENV_GSE (Game Server Encryption Key)" }

  foreach ($k in "BUILD_ENV_PR","BUILD_ENV_SA","BUILD_ENV_DE","BUILD_ENV_CC","BUILD_ENV_CS","BUILD_ENV_GSE") {
    if (-not ${env:$k}) { throw "EOS token $k is empty." }
  }

  if ($Script:PersistSensitiveEnvVars) {
    foreach ($k in "BUILD_ENV_PR","BUILD_ENV_SA","BUILD_ENV_DE","BUILD_ENV_CC","BUILD_ENV_CS","BUILD_ENV_GSE") {
      Set-EnvVarValue -Name $k -Value ${env:$k} -Persist $true -Sensitive $true
    }
  }
}

# Validates TheoraPlayer include/lib paths expected by FindTheoraPlayer.cmake.
function Check-TheoraPlayer {
  # Required by cmake/Modules/FindTheoraPlayer.cmake
  $hdr = Join-Path $TheoraPlayerRoot "include\theoraplayer\theoraplayer.h"
  $lib = Join-Path $TheoraPlayerRoot "lib\theoraplayer.lib"
  if (-not (Test-Path $hdr)) {
    throw "TheoraPlayer headers not found: $hdr. Build/provide TheoraPlayer and place headers under $TheoraPlayerRoot\include\theoraplayer\."
  }
  if (-not (Test-Path $lib)) {
    throw "TheoraPlayer import library not found: $lib. Build/provide TheoraPlayer and place theoraplayer.lib under $TheoraPlayerRoot\lib\."
  }
  $env:THEORAPLAYER_DIR = $TheoraPlayerRoot
}

# Builds TheoraPlayer via shared wrapper and stages headers/lib/dll under deps\theoraplayer.
function Build-TheoraPlayerFromSource {
  Write-Log -Level STEP -Message "Building TheoraPlayer from source: $TheoraPlayerRepoUrl"

  $tmpRoot      = Join-Path $DepsDir "_tmp_theoraplayer"
  $cloneDir     = Join-Path $tmpRoot "src"
  $cmakeBuildDir= Join-Path $tmpRoot "build"
  $wrapperDir   = Join-Path $Root "scripts\theoraplayer"

  Remove-DirectoryIfExists $tmpRoot
  New-Item -ItemType Directory -Force -Path $cloneDir | Out-Null

  try {
    Invoke-NativeCommand -Description "git clone TheoraPlayer" -Command { & git clone --depth 1 $TheoraPlayerRepoUrl $cloneDir }

    if ($TheoraPlayerRepoRef) {
      Push-Location $cloneDir
      try {
        Invoke-NativeCommand -Description "git fetch TheoraPlayer ref $TheoraPlayerRepoRef" -Command { & git fetch --depth 1 origin $TheoraPlayerRepoRef }
        Invoke-NativeCommand -Description "git checkout fetched TheoraPlayer ref" -Command { & git checkout --force FETCH_HEAD }
      }
      finally {
        Pop-Location
      }
    }

    $theoraRootCMake = Convert-ToCMakePath $cloneDir
    $toolchainCMake  = Convert-ToCMakePath $VcpkgToolchain
    $wrapperCMake = Join-Path $wrapperDir "CMakeLists.txt"
    if (-not (Test-Path $wrapperCMake)) {
      throw "Shared TheoraPlayer wrapper not found: $wrapperCMake"
    }

    Invoke-NativeCommand -Description "cmake configure TheoraPlayer wrapper" -Command {
      & cmake -S $wrapperDir -B $cmakeBuildDir -G "Visual Studio 17 2022" -A x64 `
        -DCMAKE_POLICY_VERSION_MINIMUM=3.10 `
        -DTHEORAPLAYER_ROOT="$theoraRootCMake" `
        -DTHEORAPLAYER_DEP_BACKEND="vcpkg" `
        -DCMAKE_TOOLCHAIN_FILE="$toolchainCMake" `
        -DVCPKG_TARGET_TRIPLET="$VcpkgTriplet"
    }

    Invoke-NativeCommand -Description "cmake build TheoraPlayer wrapper" -Command { & cmake --build $cmakeBuildDir --config Release --target theoraplayer }

    $releaseDir = Join-Path $cmakeBuildDir "Release"
    $builtLib   = Join-Path $releaseDir "theoraplayer.lib"
    $builtDll   = Join-Path $releaseDir "theoraplayer.dll"
    if (-not (Test-Path $builtLib)) { throw "Expected build artifact missing: $builtLib" }
    if (-not (Test-Path $builtDll)) { throw "Expected build artifact missing: $builtDll" }

    $sourceIncludeDir = Join-Path $cloneDir "theoraplayer\include\theoraplayer"
    if (-not (Test-Path $sourceIncludeDir)) {
      throw "TheoraPlayer include directory not found: $sourceIncludeDir"
    }

    $targetIncludeDir = Join-Path $TheoraPlayerRoot "include\theoraplayer"
    $targetLibDir     = Join-Path $TheoraPlayerRoot "lib"
    $targetBinDir     = Join-Path $TheoraPlayerRoot "bin"
    New-Item -ItemType Directory -Force -Path $targetIncludeDir | Out-Null
    New-Item -ItemType Directory -Force -Path $targetLibDir | Out-Null
    New-Item -ItemType Directory -Force -Path $targetBinDir | Out-Null

    & robocopy $sourceIncludeDir $targetIncludeDir *.h /E /NFL /NDL /NJH /NJS | Out-Null
    if ($LASTEXITCODE -ge 8) {
      throw "Failed to copy TheoraPlayer headers."
    }

    Copy-Item -Force $builtLib (Join-Path $targetLibDir "theoraplayer.lib")
    Copy-Item -Force $builtDll (Join-Path $targetBinDir "theoraplayer.dll")
    Write-Log -Level INFO -Message "TheoraPlayer build installed to: $TheoraPlayerRoot"
  }
  finally {
    Remove-DirectoryIfExists $tmpRoot
  }
}

# Uses existing local TheoraPlayer if present; otherwise builds and validates it.
function Ensure-TheoraPlayer {
  $hdr = Join-Path $TheoraPlayerRoot "include\theoraplayer\theoraplayer.h"
  $lib = Join-Path $TheoraPlayerRoot "lib\theoraplayer.lib"
  if ((Test-Path $hdr) -and (Test-Path $lib)) {
    Write-Log -Level INFO -Message "Using existing TheoraPlayer installation at: $TheoraPlayerRoot"
  } else {
    Build-TheoraPlayerFromSource
  }
  Check-TheoraPlayer
}

# Safe copy wrapper used for runtime staging; handles self-copy and non-fatal conflicts.
function Copy-ItemSafe {
  param(
    [Parameter(Mandatory=$true)][string]$SourcePath,
    [Parameter(Mandatory=$true)][string]$DestinationPath,
    [string]$Description = "file copy"
  )

  if (-not (Test-Path -LiteralPath $SourcePath -PathType Leaf)) {
    Write-Log -Level WARN -Message "Skipping $Description because source file was not found: $SourcePath"
    return $false
  }

  $targetPath = $DestinationPath
  if (Test-Path -LiteralPath $DestinationPath -PathType Container) {
    $targetPath = Join-Path $DestinationPath (Split-Path -Leaf $SourcePath)
  }

  $normalizedSource = $SourcePath
  $normalizedTarget = $targetPath
  try { $normalizedSource = [System.IO.Path]::GetFullPath(($SourcePath -replace '/', '\')).TrimEnd('\') } catch {}
  try { $normalizedTarget = [System.IO.Path]::GetFullPath(($targetPath -replace '/', '\')).TrimEnd('\') } catch {}

  if ($normalizedSource.Equals($normalizedTarget, [System.StringComparison]::OrdinalIgnoreCase)) {
    # Avoid Copy-Item self-overwrite errors (common when datadir points at source tree).
    Write-Log -Level DEBUG -Message "Skipping $Description because source and destination are the same file: $normalizedSource"
    return $true
  }

  if ((Test-Path -LiteralPath $targetPath -PathType Leaf) -and (-not $Script:OverwriteRuntimeFiles)) {
    Write-Log -Level INFO -Message "Skipping existing file (overwrite disabled): $targetPath"
    return $false
  }

  $copyParams = @{
    Path = $SourcePath
    Destination = $DestinationPath
    ErrorAction = 'Stop'
  }
  if ($Script:OverwriteRuntimeFiles) {
    $copyParams['Force'] = $true
  }

  try {
    Copy-Item @copyParams
    return $true
  } catch {
    Write-Log -Level WARN -Message "Failed ${Description}: '$SourcePath' -> '$targetPath'. $($_.Exception.Message)"

    if ($Script:IsInteractive) {
      # In wizard mode allow one-off overwrite/skip decisions without aborting setup.
      $overwriteNow = $false
      if ((Test-Path -LiteralPath $targetPath -PathType Leaf) -and (-not $Script:OverwriteRuntimeFiles)) {
        $overwriteNow = Prompt-YesNo -Prompt "Overwrite '$targetPath' now?" -DefaultYes $true
      }
      if ($overwriteNow) {
        try {
          Copy-Item -Path $SourcePath -Destination $DestinationPath -Force -ErrorAction Stop
          return $true
        } catch {
          Write-Log -Level WARN -Message "Retry with overwrite also failed for '$targetPath': $($_.Exception.Message)"
        }
      }

      $skip = Prompt-YesNo -Prompt "Skip this file and continue setup?" -DefaultYes $true
      if ($skip) { return $false }
    }

    return $false
  }
}

# Stages runtime binaries into build outputs and optional BARONY_DATADIR target.
function Copy-RuntimeDlls {
  Write-Log -Level STEP -Message "Copying runtime DLLs next to built executables."

  $baronyExes = Get-ChildItem -Path $BuildDir -Recurse -Filter "barony.exe" -ErrorAction SilentlyContinue
  if (-not $baronyExes) {
    Write-Warning "Could not locate barony.exe under $BuildDir. Skipping runtime copy."
    return
  }

  # Always populate both standard VS config output dirs to avoid first-run DLL errors
  # when users switch configs after setup (e.g. building Debug in Visual Studio).
  $outDirMap = @{}
  foreach ($exe in $baronyExes) {
    $outDirMap[$exe.Directory.FullName] = $true
  }
  foreach ($cfg in @("Debug", "Release")) {
    $cfgOutDir = Join-Path $BuildDir $cfg
    New-Item -ItemType Directory -Force -Path $cfgOutDir | Out-Null
    $outDirMap[$cfgOutDir] = $true
  }
  $outDirs = $outDirMap.Keys | Sort-Object

  foreach ($outDir in $outDirs) {
    Write-Log -Level INFO -Message "Runtime copy output dir: $outDir"

    # vcpkg DLLs (SDL2, curl, etc.). Use debug DLL set for Debug outputs.
    $isDebugOutput = $outDir -match '(^|[\\/])Debug($|[\\/])'
    $vcpkgBin = if ($isDebugOutput) { Join-Path $VcpkgInstalled "debug\bin" } else { Join-Path $VcpkgInstalled "bin" }
    if (-not (Test-Path $vcpkgBin)) {
      # Fallback for incomplete debug package layouts.
      $vcpkgBin = Join-Path $VcpkgInstalled "bin"
    }
    if (Test-Path $vcpkgBin) {
      & robocopy $vcpkgBin $outDir *.dll /NFL /NDL /NJH /NJS | Out-Null
    }

    # FMOD DLLs
    if ($EnableFmod) {
      $fmodBin = Join-Path $FmodDir "api\core\lib\x86_64"
      if (Test-Path $fmodBin) {
        & robocopy $fmodBin $outDir *.dll /NFL /NDL /NJH /NJS | Out-Null
      }
    }

    # TheoraPlayer DLLs (if enabled and provided)
    if ($EnableTheoraPlayer) {
      $theoraBin = Join-Path $TheoraPlayerRoot "bin"
      if (Test-Path $theoraBin) {
        & robocopy $theoraBin $outDir theoraplayer*.dll /NFL /NDL /NJH /NJS | Out-Null
      }
    }

    # Steamworks DLLs + steam_appid.txt
    if ($EnableSteamworks) {
      $steamBin = Join-Path $SteamworksRoot "sdk\redistributable_bin\win64"
      if (Test-Path $steamBin) {
        & robocopy $steamBin $outDir *.dll /NFL /NDL /NJH /NJS | Out-Null
      }
      $appid = Ensure-SteamAppIdFile
      if (Test-Path $appid) {
        [void](Copy-ItemSafe -SourcePath $appid -DestinationPath $outDir -Description "steam_appid copy")
      }
    }

    # EOS DLL
    if ($EnableEos) {
      $eosDll = Get-ChildItem -Path $EosRoot -Recurse -Filter "EOSSDK-Win64-Shipping.dll" -ErrorAction SilentlyContinue | Select-Object -First 1
      if ($eosDll) {
        [void](Copy-ItemSafe -SourcePath $eosDll.FullName -DestinationPath $outDir -Description "EOS runtime dll copy")
      } else {
        Write-Log -Level WARN -Message "EOS enabled but EOSSDK-Win64-Shipping.dll not found under $EosRoot"
      }
    }
  }

  # Optionally also copy everything to BaronyDataDir
  if ($BaronyDataDir) {
    if (-not (Test-Path $BaronyDataDir)) {
      Write-Log -Level WARN -Message "BARONY_DATADIR '$BaronyDataDir' does not exist; skipping runtime copy there."
      return
    }

    $preferredExe = $baronyExes | Where-Object { $_.Directory.FullName -like "*\Release" } | Select-Object -First 1
    if (-not $preferredExe) {
      $preferredExe = $baronyExes | Select-Object -First 1
    }
    $preferredOutDir = $preferredExe.Directory.FullName

    Write-Log -Level INFO -Message "Also copying runtime files to BARONY_DATADIR: $BaronyDataDir"
    [void](Copy-ItemSafe -SourcePath $preferredExe.FullName -DestinationPath $BaronyDataDir -Description "barony.exe datadir copy")

    $editorExe = Get-ChildItem -Path $preferredOutDir -Filter "editor.exe" -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($editorExe) {
      [void](Copy-ItemSafe -SourcePath $editorExe.FullName -DestinationPath $BaronyDataDir -Description "editor.exe datadir copy")
    }

    & robocopy $preferredOutDir $BaronyDataDir *.dll /NFL /NDL /NJH /NJS | Out-Null

    $langEn = Join-Path $SrcDir "lang\en.txt"
    if (Test-Path $langEn) {
      $langDir = Join-Path $BaronyDataDir "lang"
      New-Item -ItemType Directory -Force -Path $langDir | Out-Null
      [void](Copy-ItemSafe -SourcePath $langEn -DestinationPath (Join-Path $langDir "en.txt") -Description "lang/en.txt datadir copy")
    }
  }

  Write-Log -Level INFO -Message "Runtime DLL copy complete."
}

function Show-ConfigurationSummary {
  Write-Log -Level STEP -Message "Configuration Summary"
  Write-Host ("  FMOD         : {0}" -f $EnableFmod)
  Write-Host ("  Steamworks   : {0}" -f $EnableSteamworks)
  Write-Host ("  EOS          : {0}" -f $EnableEos)
  Write-Host ("  CURL         : {0}" -f $EnableCurl)
  Write-Host ("  Opus         : {0}" -f $EnableOpus)
  Write-Host ("  TheoraPlayer : {0}" -f $EnableTheoraPlayer)
  Write-Host ("  PlayFab      : {0} (automation disabled)" -f $EnablePlayFab)
  Write-Host ("  BARONY_DATADIR: {0}" -f ($(if ($BaronyDataDir) { $BaronyDataDir } else { "<not set>" })))
  Write-Host ("  Overwrite runtime files: {0}" -f $Script:OverwriteRuntimeFiles)
  Write-Host ("  Persist env vars: {0}" -f $Script:PersistEnvVars)
  if ($EnableEos) {
    Write-Host ("  Persist EOS secrets: {0}" -f $Script:PersistSensitiveEnvVars)
  }
}

# Applies process/user env vars consumed by Barony CMake and runtime scripts.
function Configure-BaronyEnvironment {
  # Barony expects BARONY_WIN32_LIBRARIES to be a prefix containing include/ + lib/.
  Set-EnvVarValue -Name "BARONY_WIN32_LIBRARIES" -Value $VcpkgInstalled -Persist $Script:PersistEnvVars

  # Feature env vars consumed by CMakeLists.txt.
  Set-EnvVarValue -Name "STEAMWORKS_ENABLED" -Value ([int]$EnableSteamworks) -Persist $Script:PersistEnvVars
  Set-EnvVarValue -Name "EOS_ENABLED" -Value ([int]$EnableEos) -Persist $Script:PersistEnvVars
  Set-EnvVarValue -Name "CURL_ENABLED" -Value ([int]$EnableCurl) -Persist $Script:PersistEnvVars
  Set-EnvVarValue -Name "OPUS_ENABLED" -Value ([int]$EnableOpus) -Persist $Script:PersistEnvVars
  Set-EnvVarValue -Name "PLAYFAB_ENABLED" -Value ([int]$EnablePlayFab) -Persist $Script:PersistEnvVars
  Set-EnvVarValue -Name "THEORAPLAYER_ENABLED" -Value ([int]$EnableTheoraPlayer) -Persist $Script:PersistEnvVars

  if ($EnableOpus) {
    Set-EnvVarValue -Name "OPUS_DIR" -Value $VcpkgInstalled -Persist $Script:PersistEnvVars
  }

  if ($BaronyDataDir) {
    Set-EnvVarValue -Name "BARONY_DATADIR" -Value $BaronyDataDir -Persist $Script:PersistEnvVars
    Write-Log -Level INFO -Message "Using BARONY_DATADIR: $BaronyDataDir"
  }
}

# --------------------------
# MAIN
# --------------------------
# Main flow: wizard -> toolchain/deps -> SDK validation -> configure/build/install -> runtime staging.
Start-InstallLogging -LogPath $Script:InstallLogPath
try {
  Write-Host "=============================================================="
  Write-Host "  Barony setup (VS 2022-compatible, x64)"
  Write-Host "  Root: $Root"
  Write-Host ("  Mode: {0}" -f ($(if ($NonInteractive) { "Non-interactive" } else { "Interactive wizard" })))
  Write-Host "=============================================================="
  Write-Host ""

  Write-Log -Level STEP -Message "Step 1/8: Validating prerequisites."
  Assert-Command git   "Install Git and ensure git.exe is on PATH."
  Assert-Command cmake "Install CMake and ensure cmake.exe is on PATH."

  if (-not (Test-Path (Join-Path $SrcDir "CMakeLists.txt"))) {
    throw "CMakeLists.txt not found in '$SrcDir'. Run this script from the repository root."
  }
  New-Item -ItemType Directory -Force -Path $DepsDir | Out-Null

  Prompt-FeatureConfiguration
  $BaronyDataDir = Resolve-BaronyDataDir -ConfiguredPath $BaronyDataDir -PromptToLocate $PromptToLocateBaronyDataDir
  Show-ConfigurationSummary

  if ($Script:IsInteractive) {
    if (-not (Prompt-YesNo -Prompt "Continue with this configuration?" -DefaultYes $true)) {
      throw "Aborted by user before setup execution."
    }
  }

  Write-Log -Level STEP -Message "Step 2/8: Loading Visual Studio developer environment."
  $vsInstall = Get-VsInstallPath
  $vsDevCmd  = Join-Path $vsInstall "Common7\Tools\VsDevCmd.bat"
  Import-VsDevCmdEnv -VsDevCmdBat $vsDevCmd

  Write-Log -Level STEP -Message "Step 3/8: Preparing and installing vcpkg dependencies."
  # Avoid pulling an unrelated global vcpkg root/toolchain from another VS install.
  if (Test-Path Env:VCPKG_ROOT) {
    Remove-Item Env:VCPKG_ROOT -ErrorAction SilentlyContinue
  }
  Ensure-Vcpkg
  Vcpkg-Install

  Write-Log -Level STEP -Message "Step 4/8: Validating manually placed SDK dependencies."
  if ($EnableFmod) {
    Ensure-ManualDependency -Name "FMOD" -CheckAction { Check-Fmod } `
      -ExpectedPaths @(
        (Join-Path $FmodDir "api\core\inc\fmod.hpp"),
        (Join-Path $FmodDir "api\core\lib\x86_64\fmod_vc.lib")
      ) `
      -DownloadUrl "https://www.fmod.com/download"
  }
  if ($EnableSteamworks) {
    Ensure-ManualDependency -Name "Steamworks" -CheckAction { Check-Steamworks } `
      -ExpectedPaths @(
        (Join-Path $SteamworksRoot "sdk\public\steam\steam_api.h"),
        (Join-Path $SteamworksRoot "sdk\redistributable_bin\win64\steam_api64.lib")
      ) `
      -DownloadUrl "https://partner.steamgames.com/downloads/list"
  }
  if ($EnableEos) {
    Ensure-ManualDependency -Name "EOS SDK" -CheckAction { Check-Eos } `
      -ExpectedPaths @(
        (Join-Path $EosRoot "SDK\Include\eos_sdk.h"),
        (Join-Path $EosRoot "SDK\Lib\EOSSDK-Win64-Shipping.lib")
      ) `
      -DownloadUrl "https://onlineservices.epicgames.com/en-US/sdk"
  }

  if ($EnableTheoraPlayer) {
    Write-Log -Level INFO -Message "Ensuring TheoraPlayer dependency exists (auto-build if needed)."
    Ensure-TheoraPlayer
  }

  Write-Log -Level STEP -Message "Step 5/8: Configuring environment variables."
  Configure-BaronyEnvironment
  if ($EnableFmod) {
    Set-EnvVarValue -Name "FMOD_DIR" -Value $FmodDir -Persist $Script:PersistEnvVars
  }
  if ($EnableSteamworks) {
    Set-EnvVarValue -Name "STEAMWORKS_ROOT" -Value $SteamworksRoot -Persist $Script:PersistEnvVars
  }
  if ($EnableTheoraPlayer) {
    Set-EnvVarValue -Name "THEORAPLAYER_DIR" -Value $TheoraPlayerRoot -Persist $Script:PersistEnvVars
  }
  if ($EnableEos) {
    Set-EnvVarValue -Name "EOS_ROOT" -Value $EosRoot -Persist $Script:PersistEnvVars
    Ensure-EosTokens
  }

  Write-Log -Level STEP -Message "Step 6/8: Configuring CMake solution."
  New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null
  $fmodFlag = if ($EnableFmod) { "ON" } else { "OFF" }
  $InstallPrefix = if ($BaronyDataDir) { $BaronyDataDir } else { Join-Path $BuildDir "install-root" }
  if (-not $BaronyDataDir) {
    New-Item -ItemType Directory -Force -Path $InstallPrefix | Out-Null
  }
  Write-Log -Level INFO -Message "CMake install prefix: $InstallPrefix"

  Invoke-NativeCommand -Description "cmake configure Barony solution" -Command {
    & cmake -S $SrcDir -B $BuildDir -G "Visual Studio 17 2022" -A x64 `
      -DCMAKE_POLICY_VERSION_MINIMUM=3.10 `
      -DCMAKE_TOOLCHAIN_FILE="$VcpkgToolchain" -DVCPKG_TARGET_TRIPLET=$VcpkgTriplet `
      -DFMOD_ENABLED=$fmodFlag -DOPENAL_ENABLED=OFF `
      -DCMAKE_CXX_STANDARD=17 -DCMAKE_CXX_STANDARD_REQUIRED=ON `
      -DCMAKE_INSTALL_PREFIX="$InstallPrefix"
  }

  Write-Log -Level STEP -Message "Step 7/8: Building Release and INSTALL targets."
  Invoke-NativeCommand -Description "cmake build Release" -Command { & cmake --build $BuildDir --config Release --parallel }
  Invoke-NativeCommand -Description "cmake build INSTALL target" -Command { & cmake --build $BuildDir --config Release --target INSTALL --parallel }

  Write-Log -Level STEP -Message "Step 8/8: Copying runtime DLLs and finalizing output."
  Copy-RuntimeDlls

  Write-Host ""
  Write-Host "=============================================================="
  Write-Host "SUCCESS"
  Write-Host "  Source      : $SrcDir"
  Write-Host "  Build dir   : $BuildDir"
  Write-Host "  Solution    : $(Join-Path $BuildDir 'barony.sln')"
  Write-Host "  Install     : $InstallPrefix"
  Write-Host "  Install log : $Script:InstallLogPath"
  if (-not [string]::IsNullOrWhiteSpace($Script:EnvRestoreScriptPath) -and (Test-Path -Path $Script:EnvRestoreScriptPath)) {
    Write-Host "  Env restore : $Script:EnvRestoreScriptPath"
    Write-Host "  WARNING     : Env restore script may contain sensitive values; keep it private and do not commit it."
  }
  Write-Host "=============================================================="
} catch {
  Write-Log -Level ERROR -Message ("Setup failed: {0}" -f $_.Exception.Message)
  Write-Host ("Install log: {0}" -f $Script:InstallLogPath)
  if (-not [string]::IsNullOrWhiteSpace($Script:EnvRestoreScriptPath) -and (Test-Path -Path $Script:EnvRestoreScriptPath)) {
    Write-Host ("Env restore: {0}" -f $Script:EnvRestoreScriptPath)
    Write-Host "WARNING: Env restore script may contain sensitive values; keep it private and do not commit it."
  }
  throw
} finally {
  Stop-InstallLogging
}
