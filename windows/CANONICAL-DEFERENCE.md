# Canonical authority and native realization

The adopted immutable semantic authority is [APS at 50d7e5a](https://github.com/flynn33/ASH-Pattern-System/tree/50d7e5ab88932a4a967ffc2c83bd18fae45b73c8), canonical version `1.0.0-rc.3`. This repository owns native realization, Windows build choices, C++ ownership and SDK product version `1.0.0-rc.1`. Copied specifications under the product root are historical references and do not override the external authority.

The machine-readable bindings are in [sdk-authority-bindings.json](packaging/sdk-authority-bindings.json). Policy and realization-profile sources have separate roles:

| Authority | Version | Immutable source commit |
|---|---|---|
| Canonical APS | `1.0.0-rc.3` | `50d7e5ab88932a4a967ffc2c83bd18fae45b73c8` |
| Engineering policy | `0.7.0` | `8dd6e75a06336491b249fac3577db75458db8c13` |
| Realization-profile catalog | `0.7.2` | `82049af6b10d4dc21d24fb9c67f1b93155e4b7e3` |

Producer and consumer canonical contracts use `rfd.aps.canonical.1.0.0-rc.3`, SHA256 `27108f8bc77d7585e0b2877a3f44052ac4e80b0bfcefadbe589d65a8a5ccbd40`. The Windows consumer contract retains `rfd.aps.windows.b1c05f1`, SHA256 `8f27f926bc0cf8a8da0e7424c4e99093614279cd72f0dab9aba246ae10bf63f1`; its name identifies the original native baseline, not the current SDK commit. Updating the profile catalog does not upgrade the engineering policy pin.

`packaging/canonical-resource-lock.json` records exact raw Git blob identities and SHA256 for the selected normative and conformance inventory. Explicit acquisition stages complete resources outside the product before promotion. `run-canonical-conformance.py` validates offline resource integrity; it does not execute native behavior. `generate-canonical-fixtures.py` translates independently pinned expectations into external test inputs. `ash-test-canonicalconformance.exe` compares native results against them.

Core acceptance covers every invariant in the pinned specification, all five categories, applicable module contracts and diagnostic completeness. The actual specification contains 28 invariant headings; an earlier count of 27 is not a reason to omit one. Failed core checks produce NON-CONFORMANT. Parsing JSON or matching selected inputs is insufficient.

The earlier authority [APS at 7bb68bf](https://github.com/flynn33/ASH-Pattern-System/tree/7bb68bf3377b18c74d4b0ace6a561371e446bbee), version `1.0.0-rc.1`, had two literal contradictions: its containment-breach vector expected `FAILED` despite the immediate `SAFE_HALT` requirement, and fourteen diagnostic vectors omitted required meaningful notes. The historical `1.0.0-rc.2` correction at `ea2c6494b0d40bd6975bd210efe33a0cf540812f` supplied complete records and distinguished prevented attempts from confirmed breach. Original rc.1 inputs, failing evidence and actual rc.2 repair/integration evidence remain unchanged. Persisted rc.1 diagnostic IDs require a complete semantic migration and separate old provenance.

Current comparisons bind schema, corpus and expectations together to rc.3. It preserves rc.2 semantic diagnostic identities, adds machine rejection of already-invalid blank explanations and multiline summaries, and states exact normative precedence for older conflicting state, recovery, realm, topology, fallback, diagnostic, axiom and generation prose. Unaffected requirements remain binding. Read the immutable [normative reconciliation](https://github.com/flynn33/ASH-Pattern-System/blob/50d7e5ab88932a4a967ffc2c83bd18fae45b73c8/PUBLIC-SPECIFICATION-API.md#100-rc3-normative-reconciliation) and [canonical migration guide](https://github.com/flynn33/ASH-Pattern-System/blob/50d7e5ab88932a4a967ffc2c83bd18fae45b73c8/MIGRATION-GUIDE.md). Rebind product-version fields and whole-artifact hashes with the exact current resources; this adoption is not final SDK qualification.

Support export schema `1.0` is separate from canonical diagnostic and typed safety transports. Diagnostic identity includes the nine semantic fields, including notes/details; schema, occurrence metadata, parent/root IDs, sequence and local safety entry/adoption times are outside that digest. Typed containment/halt projections retain complete causal evidence. Independent imported observer histories are retained and exported separately rather than spliced into one chain. Diagnostic parents use strong ownership, chain roots weak references, and malformed/cyclic graphs are rejected.

Debug-only source contains the actual rich support collector. Release compiles minimal essential collection; a runtime flag cannot activate excluded code. These profile choices cannot alter semantic decisions. The final clean matrix, artifact and independent review gates must pass before publication. The exact released `qualification.json` establishes acceptance for its source and asset bytes.

Windows-specific choices include v143, C++20, Release `/MT`, no `/GL`, `/Z7`, x64/ARM64 artifacts and caller-directed local export. No runtime authority-repository or policy-validator dependency is introduced.
