# Windows Implementation Reference Templates

## Purpose

This directory is retained as APS implementation reference material for the Windows edition.

The active target for this repository is Windows desktop. The common and desktop files describe the deliverables the Windows edition uses for module mapping, verification, diagnostics, materialization, packaging, release evidence, and acceptance status.

The mobile and service files are retained only to make target-class boundaries explicit. They are not active release targets for this repository.

## Windows Boundary

The Windows edition must satisfy the APS semantic contract files under `specs/`, the module contracts under `specs/interfaces/`, and the verification requirements under `specs/verification/`.

These reference files describe how the Windows repository organizes proof of implementation. They do not change APS behavior.

## Contents

| File | Windows edition role |
|---|---|
| `common-downstream-handoff-requirements.md` | Shared Windows implementation deliverable checklist |
| `desktop-implementation-handoff-template.md` | Active Windows desktop target checklist |
| `mobile-implementation-handoff-template.md` | Not applicable to this Windows repository |
| `service-implementation-handoff-template.md` | Not applicable to this Windows repository |

## Usage

For Windows work:

1. Read the APS semantic contract files in `specs/`.
2. Use `common-downstream-handoff-requirements.md` for the required Windows evidence set.
3. Use `desktop-implementation-handoff-template.md` for Windows desktop-specific release gates.
4. Treat mobile and service files as out-of-scope markers for this repository.
