<#
.SYNOPSIS
  Makes the players' zip, dist\MK-Arcade-Online-<version>-windows.zip: unzip it, put the ROM sets in its roms folder,
  double-click MKArcade.exe. It holds the app, the emulation core, the Qt runtime windeployqt put beside the app, the
  Visual C++ runtime (so nothing needs installing), an empty roms folder, the players' readme and the licenses.
  Nothing from the build folder but those, and never its qt.conf (that one points Qt at this PC's own folders).
  Run tools\build.cmd first.

.EXAMPLE
  powershell -File tools\package.ps1
#>
param(
    [string]$Config = 'Release',
    [switch]$KeepFolder  # keep dist\MK-Arcade-Online-<version> beside the zip (by default only the zip stays)
)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$app = Join-Path $root "build\$Config\app"
if (-not (Test-Path (Join-Path $app 'MKArcade.exe'))) { throw "no $app\MKArcade.exe: run tools\build.cmd first" }
$m = Select-String -Path (Join-Path $root 'CMakeLists.txt') -Pattern 'project\(\S+ VERSION ([0-9.]+)'
if (-not $m) { throw 'no project version in CMakeLists.txt' }
$version = $m.Matches[0].Groups[1].Value
$name = "MK-Arcade-Online-$version"
$dist = Join-Path $root 'dist'
$stage = Join-Path $dist $name
$zip = Join-Path $dist "$name-windows.zip"

# the folder this script fills (only ever its own: dist\MK-Arcade-Online-<version>)
if (Test-Path $stage) { Remove-Item -LiteralPath $stage -Recurse -Force }
New-Item -ItemType Directory -Force -Path $stage | Out-Null

# the app and the core, and the Qt runtime: its DLLs, the plugins the app loads and the QML modules it imports
Copy-Item (Join-Path $app 'MKArcade.exe'), (Join-Path $app 'mka_fbneo.dll') $stage
Get-ChildItem $app -Filter 'Qt6*.dll' | Copy-Item -Destination $stage
foreach ($dir in 'generic', 'iconengines', 'imageformats', 'networkinformation', 'platforms', 'tls', 'qml') {
    if (-not (Test-Path (Join-Path $app $dir))) { throw "no $dir beside the app: run tools\build.cmd (windeployqt) again" }
    Copy-Item -Recurse (Join-Path $app $dir) (Join-Path $stage $dir)
}

# the Visual C++ runtime the app and Qt load, beside them (as Microsoft allows), from the build tools' redist folder
$vcvars = 'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat'
# (cmd reads %...% before the call runs: !...! with delayed expansion is read after it)
$redist = (cmd /v:on /c "call `"$vcvars`" >nul 2>&1 && echo !VCToolsRedistDir!" | Select-Object -Last 1).Trim()
if (-not $redist -or $redist.StartsWith('!')) { throw "the VS build tools' redist folder was not found ($vcvars)" }
$crt = Get-ChildItem (Join-Path $redist 'x64') -Directory -Filter 'Microsoft.VC*.CRT' | Select-Object -First 1
if (-not $crt) { throw "no Microsoft.VC*.CRT folder under $redist\x64" }
foreach ($dll in 'msvcp140.dll', 'msvcp140_1.dll', 'msvcp140_2.dll', 'vcruntime140.dll', 'vcruntime140_1.dll') {
    Copy-Item (Join-Path $crt.FullName $dll) $stage
}

# where the player's ROM sets go, what to read first, and the licenses
New-Item -ItemType Directory -Force -Path (Join-Path $stage 'roms') | Out-Null
$readme = (Get-Content -Raw (Join-Path $PSScriptRoot 'package\README.txt')).Replace('@VERSION@', $version)
[IO.File]::WriteAllText((Join-Path $stage 'README.txt'), $readme.Replace("`r`n", "`n").Replace("`n", "`r`n"))
Set-Content -Path (Join-Path $stage 'roms\PUT YOUR ROMS HERE.txt') -Encoding ascii -Value @(
    'Put your own ROM sets here: mk.zip, mk2.zip, mk3.zip, umk3.zip (MAME or FBNeo sets).',
    'Or choose another folder in the app: SETTINGS, LIBRARY.')
Copy-Item (Join-Path $PSScriptRoot 'package\THIRD-PARTY.txt') $stage
$licenses = Join-Path $stage 'licenses'
New-Item -ItemType Directory -Force -Path $licenses | Out-Null
Copy-Item (Join-Path $PSScriptRoot 'package\licenses\LGPL-3.0.txt') $licenses
Copy-Item (Join-Path $root 'external\FBNeo\src\license.txt') (Join-Path $licenses 'FBNeo-license.txt')
Copy-Item (Join-Path $root 'external\SDL3\LICENSE.txt') (Join-Path $licenses 'SDL3-LICENSE.txt')
Copy-Item (Join-Path $root 'external\GekkoNet\LICENSE') (Join-Path $licenses 'GekkoNet-LICENSE.txt')

# the zip, with the folder inside it (Windows' own tar writes a standard zip)
if (Test-Path $zip) { Remove-Item -LiteralPath $zip -Force }
& "$env:SystemRoot\System32\tar.exe" -a -c -f $zip -C $dist $name
if ($LASTEXITCODE) { throw 'zipping failed' }
$files = (Get-ChildItem $stage -Recurse -File).Count
'Made {0}: {1} files, {2:N1} MB (unzipped {3:N1} MB)' -f $zip, $files, ((Get-Item $zip).Length / 1MB),
    ((Get-ChildItem $stage -Recurse -File | Measure-Object Length -Sum).Sum / 1MB)
if (-not $KeepFolder) { Remove-Item -LiteralPath $stage -Recurse -Force }
