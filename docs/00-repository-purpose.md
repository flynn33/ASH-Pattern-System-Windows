# Repository Purpose

## What This Repository Is

This repository is the Windows edition of the ASH Pattern System. It contains a native C++20 semantic core, Windows build assets, platform conformance documentation, and release-readiness evidence for supported Windows desktop targets.

Its job is to turn the APS semantic contract into a Windows-native implementation that can be built, tested, packaged, and audited with Microsoft-supported tooling.

## What This Repository Must Accomplish

The Windows edition must make the following explicit:

- how each APS semantic module is represented in C++20;
- how 9-bit state, codewords, transitions, diagnostics, recovery, topology, axiom evaluation, planning, and emission are implemented;
- how deterministic behavior is verified by C++ tests;
- how Windows build outputs are produced with Visual Studio/MSBuild;
- where release blockers remain for packaging, signing, accessibility, installation, and owner approval;
- which platform decisions are Windows-specific and how they are tracked.

## What This Repository Is Not For

This repository is not a neutral design-only package. It is not the place to describe every possible language, operating system, runtime, or package model.

Examples of concerns outside this Windows edition:

- non-Windows build systems;
- non-C++ production implementations;
- mobile packaging;
- service deployment;
- Unix-specific runtime behavior;
- product claims that are not backed by Windows evidence.

## Platform Boundary

Windows-specific choices belong in this repository when they are needed to build, test, package, or validate the Windows edition.

Semantic behavior must remain consistent with the APS contract files bundled in this repository. If a platform decision conflicts with those semantics, the platform decision must be changed or recorded as an unresolved release blocker.

## Success Condition

A reader should be able to understand how the Windows edition is built, how its semantic modules are wired, how its tests map to APS behavior, and why the current product is or is not ready for release.
