<<<<<<< HEAD
# Migration Report

Date: 2026-06-21

## Current State

The Windows repository currently exposes a native semantic core and test executables. It does not define a persisted data model, versioned operational context schema, import/export format, migration matrix, downgrade path, corruption-recovery behavior, or deletion/reset workflow.

## Blocking Requirements

Before release, the Windows product needs:

- Versioned operational context schema.
- Fallback policy instance storage and validation.
- Import/export format decisions.
- Migration and downgrade tests.
- Corruption and missing-data recovery tests.
- Atomicity/crash-consistency evidence for any persisted state.
- Deletion/reset behavior.

## Judgment

`NOT_SHIPPABLE`

