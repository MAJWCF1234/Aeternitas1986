# Recovery Baseline Note

The file below is the original pre-corruption master binary and should be treated as the golden baseline during recovery work (same file on disk; clone path may use **O:** or **D:**):

- `O:\light\Aeternitas1986\aeternitas64modern original.exe`
- `D:\light\Aeternitas1986\aeternitas64modern original.exe`

## Handling Rules

- Do not modify, rename, or overwrite this file.
- Use this binary as the source-of-truth reference for behavioral and data comparisons.
- Any recovered/rebuilt outputs should be compared against this baseline before acceptance.

## Verification (to fill/refresh as needed)

- SHA256: `e5313f9ea7b60ed79ac570805558fc39953651891b9c804c5258729c9bd40fa6`
- Date confirmed: 2026-05-10
- Confirmed by: `certutil -hashfile` on `O:\light\Aeternitas1986\aeternitas64modern original.exe` (same filename as golden baseline above; drive letter may differ)