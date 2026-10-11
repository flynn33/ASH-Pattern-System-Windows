# GitHub Governance Agents

## Windows repository role

This document describes the actual attribution sentinel in the Windows native
implementation and static SDK repository. The workflow inventory under
`.github/workflows/` currently contains
[no-ai-attribution.yml](../.github/workflows/no-ai-attribution.yml); its checker is
[no_attribution_check.py](../.github/scripts/no_attribution_check.py).
Native source, MSBuild projects, tests and SDK packaging belong in this product.

The immutable external semantic authority is
[APS 1.0.0-rc.3 at 50d7e5a](https://github.com/flynn33/ASH-Pattern-System/tree/50d7e5ab88932a4a967ffc2c83bd18fae45b73c8).
Engineering policy remains `0.7.0` at
`8dd6e75a06336491b249fac3577db75458db8c13`; the separate realization-profile catalog
is `0.7.2` at `82049af6b10d4dc21d24fb9c67f1b93155e4b7e3`.
See [repository governance](repository-governance.md) and the
[exact bindings](../windows/packaging/sdk-authority-bindings.json).

Upstream [canonical governance](https://github.com/flynn33/ASH-Pattern-System/blob/50d7e5ab88932a4a967ffc2c83bd18fae45b73c8/governance/github-agents-governance.md)
documents the external specification repository's checks. Its alignment,
semantic-integrity, math and downstream-conformance workflows are not local
Windows workflows. This product's sentinel neither owns canonical truth nor
implements those external checks.

## Attribution sentinel

The workflow runs on branch and tag pushes and pull requests. It uses read-only
contents permission, pinned checkout/Python setup actions, full Git history and
checkout without persisted credentials, then invokes the checker with the event
name/path and Git ref/SHA.

The checker selects the actual event commits: pull-request base-to-head range,
ordinary push before-to-after range, explicit pushed commits for a new branch,
or the verified target of a tag push. Tag deletion, invalid/unavailable objects
and ref/head mismatches reject. Annotated tag metadata is checked separately.

Checks cover prohibited attribution markers in the ref name, changed paths,
commit authors/committers/message bodies, annotated tag metadata and readable
changed-file content. Rename/copy records include both paths. The two explicit
grandfathered paths are the existing workflow and historical coding handoff;
that path-name exemption does not disable content checks. Binary content is
skipped by this text check.

The checker returns 1 for detected violations and 2 for invalid evidence or an
unexpected checking failure. It has no report-only activation switch. Owner
identity and attribution policy remain binding; neither this document nor a
passing check grants write permission or claims human approval. The owner's
[contribution policy](../CONTRIBUTING.md) remains unchanged.

## Mathematics, safety and delivery acceptance

The pinned semantic authority and its explicit precedence control the locked
mathematics, operational assessment, fallback, containment, terminal finality and
required diagnostic records. A sentinel output cannot redefine these contracts.
Its successful attribution result supplies no native or semantic conformance
result and no package/consumer acceptance result.

The Windows [qualification entry point](../windows/packaging/verify-sdk-release.ps1)
collects actual native, canonical, archive and consumer evidence for the
[acceptance judgment](../windows/conformance/acceptance-judgment.md).
The exact released `qualification.json` binds clean source, selected authorities,
libraries, SDK assets and independent delivery conclusions. Mandatory failures
block publication. Local script checks and external canonical governance must
be reported with their actual scope instead of being described as fictional
Windows CI workflows.
