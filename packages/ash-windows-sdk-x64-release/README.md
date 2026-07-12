# ASH Pattern System Windows SDK

- Platform: x64
- Configuration: Release
- Source bin: `D:\GitHub\APS\windows\build\bin\x64-Release`

## Contents

- `include/ash/` â€” public C++ headers
- `lib/ash-core.lib` â€” static library
- `lib/*.pdb` â€” debug symbols when present

## Integrate

1. Add `include` to additional include directories.
2. Link `lib\ash-core.lib`.
3. Toolchain: MSVC C++20, matching architecture (x64).

## Build artifacts source

Produced by:

```powershell
cd windows\build
.\build.ps1 -Configuration Release -Platform x64
```

Signing: this package is **unsigned** unless a production cert is applied externally.
