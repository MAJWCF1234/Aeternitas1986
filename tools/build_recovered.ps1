# Build recovered game EXE (MinGW GCC). Output: ..\aeternitas64_recovered.exe
# Requires: gcc on PATH. Compare to original with:
#   py tools\dual_first_diff.py --corpus recovery_artifacts\golden_playthrough --label global --full-normalize

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot
Set-Location $Root

$Sources = @(
    "aeternitas64_ascii.c",
    "aeternitas_item_catalog.c",
    "aeternitas_world_generated.c",
    "aeternitas_char_creation.c",
    "aeternitas_mod_guide.c",
    "aeternitas_char_description.c",
    "aeternitas_mods.c",
    "aeternitas_mod_bootstrap.c"
)

$Out = Join-Path $Root "aeternitas64_recovered.exe"
$Args = @("-std=c99", "-Wall", "-Wextra", "-O2", "-o", $Out) + $Sources

Write-Host "gcc $($Args -join ' ')"
& gcc @Args
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
Write-Host "OK: $Out"
