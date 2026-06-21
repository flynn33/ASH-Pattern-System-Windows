# Test and Quality Baseline

Date: 2026-06-21

## Local Toolchain

| Tool | Result |
|---|---|
| `clang++` | Available at `/usr/bin/clang++` |
| `g++` | Available at `/usr/bin/g++` |
| `pwsh` | Not found |
| `powershell` | Not found |
| `msbuild` | Not found |

## Local Verification Commands

The strongest practical local check on this host is C++20 compilation with warnings as errors, followed by all local test executables.

| Test executable | Test count |
|---|---:|
| `/tmp/ash-test-statemodel` | 10 |
| `/tmp/ash-test-transitionregistry` | 13 |
| `/tmp/ash-test-diagnostics` | 16 |
| `/tmp/ash-test-recoveryengine` | 11 |
| `/tmp/ash-test-completionmodules` | 10 |

## Quality Status

- Local C++20 tests passed after implementation.
- Native Windows build and test script are not verified on this host.
- Static analysis beyond compiler warnings was not available locally.
- Local semantic coverage includes all 512 realm identities and all 8,192 state/codeword transformation pairs. Full upstream JSON/vector corpus execution is not complete.

## Native Command Attempts

| Command | Result |
|---|---|
| `pwsh -File windows/build/build.ps1` | Failed locally: `zsh:1: command not found: pwsh` |
| `msbuild windows/build/ash-windows.sln` | Failed locally: `zsh:1: command not found: msbuild` |
| `powershell -File windows/tests/run_all.ps1` | Failed locally: `zsh:1: command not found: powershell` |
