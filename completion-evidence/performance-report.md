<<<<<<< HEAD
# Performance Report

Date: 2026-06-21

## Local Observations

The semantic core algorithms are bounded over small canonical sets: 512 states, 16 codewords, 32 orbits, and bounded topology depth. Local tests exercise the exhaustive state/codeword transition space, but they do not collect timing, memory, cancellation, stress, or reliability metrics.

## Blocking Requirements

Before release:

- Define performance and memory budgets for supported workflows.
- Measure Release x64 and ARM64 builds on Windows.
- Verify topology resource-limit behavior under maximum supported depth.
- Verify cancellation behavior where workflows can be interrupted.
- Run reliability/failure-injection tests for persistence, packaging, install, upgrade, and recovery workflows after those surfaces exist.

## Judgment

`NOT_SHIPPABLE`

