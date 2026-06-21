# Deviation Log — APS Windows

This log records known Windows-platform limitations relative to the final product-completion package. A deviation here is not a change to APS behavior.

## D-001 — Empty Fallback Policy Instance Store

**What:** `RecoveryEngine::select_fallback()` currently observes an empty in-memory fallback policy instance store and escalates with a diagnostic.

**Why:** APS does not declare a universal domain-safe fallback state. Concrete fallback candidates must come from validated operational context or declared policy instances. Until such an instance is supplied to the Windows product, selecting no replacement and escalating is the fail-closed behavior.

**Scope:** `windows/src/RecoveryEngine.cpp`, `windows/tests/RecoveryEngine.test.cpp`.

**Resolution:** Add a typed Windows fallback policy instance model and tests for declared known-good and last-verified-stable candidates. That work must preserve the rule that cross-orbit replacement is fallback, not a codeword transition.

## D-002 — No Production UI or Signed Installer in This Local Iteration

**What:** The current repository state provides a native C++ semantic core and tests, not a WinUI 3 product UI or signed MSIX installer.

**Why:** The local execution environment does not provide Visual Studio/MSBuild, Windows App SDK, signing credentials, or a clean Windows install target. Those gates are owner/environment controlled.

**Scope:** Packaging, accessibility UI verification, install/upgrade/uninstall validation, and production signing evidence.

**Resolution:** Complete these on a Windows build host with Visual Studio/MSBuild, Windows App SDK, production signing credentials, and clean supported Windows validation targets. Do not mark the release shippable until those gates pass.

## D-003 — Operational Context Model Is Incomplete

**What:** The semantic core distinguishes structural operations from recovery/fallback/halt diagnostics, but it does not yet expose a complete versioned `OperationalContext` schema in C++.

**Why:** The current work closes module completion and canonical encoding first. The package still requires a fuller typed operational context with stable-state sets, preferred targets, fallback policy instance references, containment policy, halt policy, and registry versions.

**Scope:** `StateModel`, `RecoveryEngine`, data/configuration, migration, and product workflow evidence.

**Resolution:** Add the typed context model, validation, persistence rules, negative tests, and migration evidence before any final release judgment.
