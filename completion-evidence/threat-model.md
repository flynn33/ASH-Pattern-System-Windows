# Threat Model

Date: 2026-06-21

## Current Scope

The audited implementation is a native C++ semantic core with in-memory module behavior. The current changes did not add network access, filesystem writes, credential handling, registry access, process execution, UI automation, or new dependencies.

## Trust Boundaries

| Boundary | Current status |
|---|---|
| External input parsing | Not complete; no product parser or import surface exists |
| Persistence | Not present |
| File writes | Not present in the semantic core |
| Network access | Not present |
| Credentials/secrets | Not present |
| Package/update channel | Not present |
| UI entry points | Not present |

## Blocking Requirements

Before release, complete:

- Product entry-point and data-flow threat model.
- Secret and sensitive-data handling review.
- Unsafe API review on the native Windows build output.
- Package/update trust review.
- Logging and diagnostics redaction review.
- Misuse/abuse case review for recovery/fallback/materialization workflows.

## Judgment

`NOT_SHIPPABLE`
