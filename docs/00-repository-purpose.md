# Repository Purpose

## What this repository is

This repository is the **canonical agnostic specification baseline** for the ASH Pattern System.

Its job is to define the engine in a way that survives translation across:

- languages
- operating systems
- runtime environments
- developer toolchains
- packaging models
- future implementation repositories

## What this repository must accomplish

This repository must provide enough semantic clarity that a coding agent can build a correct implementation without needing a preexisting language-bound repository as the authority.

That means the repository must make the following explicit:

- what an ASH state is
- how the control dimension is derived
- what makes a state valid
- what transitions are allowed
- how topology is expanded
- how axioms are evaluated
- how a generation plan is produced before side effects occur
- what downstream implementations must preserve

## What this repository is not for

This repository is not a place to settle language-specific engineering decisions.

Examples of concerns that do not belong here as canonical truth:

- whether implementation uses structs, classes, enums, or traits
- whether validation is compile-time, runtime, or hybrid
- whether artifacts are emitted through a CLI, GUI, service, or IDE extension
- whether storage is local, remote, or in-memory
- whether emitted projects use one directory layout or another

## Source-of-truth rule

If a future implementation disagrees with this repository, this repository wins.

## Success condition

A reader should be able to understand the ASH Pattern System as a coherent system **without seeing implementation language code**.
