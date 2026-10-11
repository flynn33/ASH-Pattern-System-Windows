# Compatibility and limits

The contract is Microsoft v143 19.44 or later, C++20, architecture-matched Release /MT, iterator debugging zero, no /GL and embedded /Z7 debug information. Exact compiler/linker/SDK versions are recorded in qualification evidence; arbitrary C++ ABI compatibility or earlier linkers are not established.

The intended OS floor is Windows 10 1809 and Windows 11. A single Windows 11 x64 execution does not qualify that range. ARM64 cross-build/link is not runtime execution. Physical-device and untested OS coverage remain owner-phase limitations.

Public records gained context/identity fields, structured diagnostic details/occurrence metadata and typed safety projections; diagnostic root ownership became weak. Rebuild/relink all consumers and adapt chain-root access with `lock()`. There is no compatibility claim with historical unverified archives. Owning safety projections retain chain lifetime, while mutable semantic records still require caller serialization.

Unsigned prerelease status and debug path metadata are disclosed. Core conformance failures block publication and cannot become hardware caveats.

SDK product `1.0.0-rc.1` remains distinct from the adopted canonical APS `1.0.0-rc.3` binding. Engineering policy remains `0.7.0`; the adopted realization-profile catalog is `0.7.2`, with the native `rfd.aps.windows.b1c05f1` baseline profile retained. Release acceptance must be established by the exact released `qualification.json`, bound to its clean source and asset bytes. See [Conformance and Release Status](Conformance-and-Release-Status).
