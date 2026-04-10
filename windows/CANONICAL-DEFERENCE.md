# Canonical Deference

This file records the branch's explicit deference to the canonical ASH Pattern System main repository.

## Statement of deference

The `windows-cpp` branch is a downstream implementation branch of the ASH Pattern System. It is not the authority on any ASH semantic question. When any file on this branch — source code, tests, documentation, conformance deliverables, or design notes — conflicts with the canonical specifications on `main`, the canonical specification wins. No exception.

The canonical agnostic main repository is the source of truth. The `windows-cpp` branch is one Windows C++ interpretation of that source of truth, scoped to the desktop target class defined in `handoff-templates/desktop-implementation-handoff-template.md`.

## What this branch may do

- Implement the nine canonical semantic modules for Windows desktop C++
- Contain Windows-specific source code, MSBuild project files, PowerShell scripts, tests, and implementation documentation
- Record branch-local decisions (integer encoding, realm identity string format, test framework) that are not prescribed by the canonical spec, provided those decisions do not bind the canonical spec and are documented in the deviation log
- Preserve the canonical meaning of every ASH module and interface

## What this branch may not do

- Modify the canonical files at the repository root (`README.md`, `specs/`, `docs/`, `governance/`, `handoff-templates/`)
- Redefine, override, extend, or restrict canonical ASH semantics
- Reintroduce the superseded 8+1 canonical model (decomposing 9-bit state into 8-bit core plus derived 9th bit)
- Weaken contracts, diagnostics, registries, invariants, or conformance requirements
- Back-propagate platform or implementation decisions into canonical docs on main
- Claim that this branch is the source of truth for any ASH semantic question

## Authority boundary in source and test files

Every C++ header file in `windows/src/include/ash/` carries a contract comment block quoting the canonical contract or specification it implements. Every test file verifies properties of the canonical specification as applied to this branch's implementation, not properties of the branch's private conventions.

Where a branch-local decision exists (for example, the integer encoding from `F2^9` to `uint16_t`, or the realm identity string format), the deviation is documented in `windows/conformance/deviation-log.md` with:

1. A statement that the canonical spec does not prescribe the decision
2. The branch-local choice
3. The resolution path — how the branch would accept a future canonical resolution that replaces the branch-local choice

Branch-local decisions are not presented as canonical. They are presented as implementation conventions that apply only on this branch.

## Resolution rule for conflicts

If a future change on this branch would conflict with the canonical spec on main, the correct response is:

1. Stop the change
2. Escalate to the canonical spec (if the spec is ambiguous, raise the ambiguity on main, not here)
3. Do not silently adapt the branch in a way that diverges from main

This rule exists because the canonical repository is the only place where ASH semantics are negotiated. The `windows-cpp` branch is where one target class's implementation of those semantics lives.

## Reference

- Canonical governance rule: `governance/repository-governance.md` on main (§ Main-repository closeout)
- Governing downstream template: `handoff-templates/desktop-implementation-handoff-template.md` on main
- Canonical specifications: `specs/core/`, `specs/algorithms/`, `specs/interfaces/`, `specs/verification/` on main
