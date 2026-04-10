# Build and Run Instructions

## Prerequisites

- **Visual Studio 2022** (17.8 or later) with the **Desktop development with C++** workload, or **Build Tools for Visual Studio 2022** with the equivalent workload. Both are Microsoft-produced.
- **Windows 10 (1809)** or later, x64 or ARM64.
- **PowerShell 5.1** or later (ships with Windows) or **PowerShell 7**.

Optional:
- **Windows SDK 10.0.22621.0** or later (installed automatically with the Visual Studio C++ workload).

No other tools are required. The branch uses only MSVC `cl.exe`, MSBuild, and PowerShell — all Microsoft-produced.

## Build

From a Developer PowerShell prompt or any PowerShell prompt with the Visual Studio tooling on PATH:

```powershell
cd windows\build
.\build.ps1
```

The `build.ps1` script:

1. Locates `msbuild.exe` via `vswhere.exe` (which ships with Visual Studio).
2. Restores any build artifacts from a previous run.
3. Invokes MSBuild on `ash-windows.sln` with `/p:Configuration=Debug /p:Platform=x64` by default.
4. On success, prints the paths to `ash_core.lib` and the four test executables.

### Build options

```powershell
.\build.ps1 -Configuration Release
.\build.ps1 -Platform ARM64
.\build.ps1 -Test              # build and then run the full test suite
```

## Run the tests

After a successful build:

```powershell
cd windows\tests
.\run_all.ps1
```

The `run_all.ps1` script runs all four slice-module test executables in order, prints `[PASS]` or `[FAIL]` per test case, and exits with the total failure count. A zero exit code means all tests passed.

The test executables can also be run individually from `windows\build\bin\x64-Debug\`:

```powershell
.\ash-test-statemodel.exe
.\ash-test-transitionregistry.exe
.\ash-test-diagnostics.exe
.\ash-test-recoveryengine.exe
```

Each test executable exits with the number of failed test cases (0 if all pass).

## Troubleshooting

**`msbuild.exe` not found.** Ensure Visual Studio 2022 or Build Tools for Visual Studio 2022 is installed with the C++ workload. The `build.ps1` script uses `vswhere.exe` at `${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe` to locate MSBuild. If that path does not exist, reinstall the Visual Studio Installer.

**`cl.exe` cannot find standard headers.** The build is configured to use the Visual Studio toolchain's own include paths via the `.vcxproj` files, not the environment. If MSBuild reports missing headers, verify the Windows SDK is installed (Visual Studio Installer → Individual components → Windows 11 SDK).

**Test failures.** All slice-module tests are deterministic and should pass on any supported Windows host. A failing test usually means a build artifact from a previous configuration is stale — run `.\build.ps1 -Configuration Debug -Clean` and retry.

## What is *not* required

The branch does not depend on:

- `vcpkg`, `Conan`, or any package manager
- Any third-party C++ library (Boost, abseil, fmt, Google Test, Catch2, etc.)
- CMake (MSBuild is the build system)
- Any network connection at build or test time
- Any Visual Studio IDE plugin beyond the default C++ workload

The branch explicitly follows the instruction package's rule: **Windows-native tools and libraries may be used only if they are produced and supported by Microsoft.**
