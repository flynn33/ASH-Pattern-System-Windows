# SDK distribution decision

The authorized product is a native static-library SDK. Version `1.0.0-rc.1` uses separate x64 and ARM64 Release ZIPs, each with one versioned root. The artifact is `ash-core.lib`, matching target `ash-core`.

The exact payload includes curated headers, generated identity/compile guards, the matching Release `/MT` library, relocatable MSBuild props/targets, a standalone sample, license, consumer docs, product-owned SBOM inventory, manifest and internal checksums. Its allowlist is `../packaging/sdk/sdk-public-files.json`. `/Z7` embeds debug information in objects; no external producer PDB is required.

Packaging binds actual build metadata and hashes to the exact clean source commit. It rejects dirty/stale source, architecture/configuration mismatches, failed mandatory conformance, unverified dependencies/composition, unsafe overlapping paths and incomplete inputs. Unique staging is promoted after strict archive verification. Metadata is finalized before ZIP creation. Internal checksums exclude themselves; external SHA256SUMS.txt covers final assets outside the archives.

Installation uses explicit verified extraction and SDK project references. Upgrade uses a separate directory followed by rebuild/relink. Repair uses fresh extraction. Removal deletes only the selected SDK after removing references. Disposable acceptance workspaces test these operations without global registration.

This SDK prerelease is unsigned. Hash verification does not imply Authenticode signing. GUI applications, MSIX/MSI, certificates, persistence/migration services and hosted replacement build infrastructure are outside this SDK work order.

Publication requires final clean-source gates, both archives, extracted consumers, independent review, actual wiki update, an immutable annotated tag and downloaded assets. ARM64 runtime/physical gaps can be named candidate limitations after cross-build/link checks; core failures still block publication.

The required public asset names are `ash-pattern-system-windows-sdk-1.0.0-rc.1-x64-release.zip`, `ash-pattern-system-windows-sdk-1.0.0-rc.1-arm64-release.zip`, `qualification.json`, `release-notes.md`, `release-index.json` and `SHA256SUMS.txt`. This document defines the asset set and does not establish qualification or publication. Consult the exact released `qualification.json` for its bound source and asset bytes. The adopted canonical APS rc.3 pin and separate policy/profile pins are recorded in [canonical deference](../CANONICAL-DEFERENCE.md).
