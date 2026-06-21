# Audit Iteration 001

Date: 2026-06-21

## Starting State

- Branch: `release/aps-windows-1.0.0-completion-clean`
- Base: `origin/main` at `b832f6e9b76a8c08038ba9dfdaf76f64691405e5`
- Canonical reference: `upstream/main` at `cc253f3d137a27f0eeb471bed62bbdb939e3b6d1`

## Package Validation

| Command | Result |
|---|---|
| `python3 <windows-package>/tools/package_preflight.py` | Pass |
| `python3 <product-package>/tools/package_preflight.py` | Pass |
| `python3 <windows-package>/tools/verify_canonical_reference.py` | Pass |
| `python3 <windows-package>/tools/verify_control_package.py` | Pass |
| `python3 <windows-package>/tools/verify_protected_surface.py --policy <windows-package>/controls/protected-surface-policy.json --base-ref origin/main --mode product` | Pass |
| `python3 <windows-package>/tools/verify_protected_surface.py --policy <windows-package>/controls/protected-surface-policy.json --base-ref origin/main --mode product --write-baseline /tmp/aps-windows-protected-surface-baseline.json --verify-baseline /tmp/aps-windows-protected-surface-baseline.json` | Pass |

## Root Causes Found

| Finding | Root cause | Change made |
|---|---|---|
| Remaining APS modules returned incomplete results | Windows branch implemented only the original first conformance subset | Implemented realm, topology, axiom, planning, and emission modules |
| State bit ordering did not match canonical b0 ordering | Local conversion treated the low bit as b0 | Changed `Bit9State::from_int` and `to_int` to canonical ordering |
| Diagnostic rule family still referenced a branch-local temporary family | Completion modules still emitted incomplete diagnostics | Added module rule IDs and diagnostics |
| Documentation overstated incomplete areas as plans | Docs reflected the pre-completion branch state | Updated Windows docs and conformance reports |

## Focused Reruns

Local C++20 compilation and test execution passed for five test executables with 60 total tests.

Native command attempts failed locally because `pwsh`, `powershell`, and `msbuild` are not installed on this host.

## Remaining Risk

- Native MSBuild/PowerShell verification is blocked by local toolchain absence.
- Local tests cover all 512 realm identities and all 8,192 state/codeword pairs; full upstream JSON/vector corpus execution is not wired.
- Packaging, signing, install, SBOM, and release evidence are not complete.
- Repository-local protected baseline evidence was not added because the package baseline records protected governance path names that are not appropriate for new product artifacts.
- Current acceptance remains `NOT_SHIPPABLE`.
