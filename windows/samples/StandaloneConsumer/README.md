# Standalone native SDK consumer

Use the named versioned SDK release ZIP for your architecture. GitHub's automatic Source code archives are not the SDK. Do not import the repository wholesale into an application.

This sample owns its semantic objects and support session, links only `ash-core.lib`, and uses only public `ash/` headers. The sample needs C++20, Microsoft v143, Release `/MT`, and release iterator settings. Debug SDK assets are not distributed.

After extraction, build this project from a Visual Studio developer shell:

```powershell
MSBuild.exe .\StandaloneConsumer.sln /p:Configuration=Release /p:Platform=x64
.\bin\x64\StandaloneConsumer.exe .\diagnostic-exports
```

For a relocated SDK pass `/p:ASHSDKRoot` the absolute versioned root with a trailing directory separator, containing `include`, `lib`, and `msbuild`. Set `Platform=ARM64` only for the ARM64 asset. ARM64 compilation/linking is distinct from execution on a compatible host.

The application demonstrates structural normalization and XOR transitions, validated diagnostic ancestry, recovery/rejection, pure planning, descriptor emission, and one explicit bounded support diagnostic export. Output is written only to the supplied export directory. An emitted descriptor is not a generated filesystem project.
