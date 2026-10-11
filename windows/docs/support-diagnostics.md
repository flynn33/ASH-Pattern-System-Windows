# Support diagnostics

`ash::support::Session` is an opt-in, caller-owned native support collector in
`ash-core`. Canonical `ash::Diagnostics` still owns envelope and chain validation.
Support collection does not change a transition, recovery choice, realm, ordering,
or plan. Callers capture the observed canonical result and return that result even
when support capture or export fails.
Validate canonical envelopes/chains with `ash::Diagnostics` before copying bounded
rule and diagnostic identities into an event. The support collector does not retain
the graph or establish the semantic truth of caller-supplied text. Keep unavailable
numeric error codes null rather than substituting a failed input value.

Debug selects the private Development backend. It accepts lifecycle, operation,
decision, fault, and health records, including bounded state and duration evidence,
expected/observed results, canonical rules/cases, recovery attempts, typed errors,
underlying errors, synthetic fixture references, findings, hypotheses, and unknowns.
Release selects a separate private Release source file. The actual state/duration,
all-context and error-message/cause collection code is in the Debug-only compiled
implementation; shared capture dispatches to the selected collector rather than
using runtime retention flags around rich payload construction. Release accepts faults and
health, retaining short identities/messages, canonical references, terminal
outcomes, unknown evidence, and error domain/code. Release excludes state,
operation duration, error messages/causes, and detailed decision context. There is
no runtime activation switch for the Development backend. Use `capture_lazy` to
avoid constructing context for categories excluded by the selected backend.

The public contract is in `ash/SupportDiagnostics.hpp`. A session owns its store,
backend, lock, and identity. Capture, snapshot creation, health retrieval, and close
may run concurrently. `close()` rejects subsequent capture; it does not invalidate
existing snapshots or cancel an export already using one. The caller must join
all operations using a session before destroying it. A snapshot owns immutable
copied data and remains usable after the session dies. Multiple callers may export
one snapshot concurrently. These guarantees concern support sessions; they do not
make mutable semantic objects such as `RecoveryEngine` thread safe.

Canonical typed containment/halt projections are a separate API in
`ash/RecoveryEngine.hpp`. `containment_record()` and `safe_halt_record()` preserve
their original/action diagnostics and complete causal chain; `ash::Diagnostics`
serializes the corresponding canonical transports. Their UTC engine entry/adoption
times are outside semantic identity. Export retained independent histories with
`Diagnostics::serialize_chain()` separately; the bounded support snapshot is not
a substitute for those full canonical safety records.

Capture returns `CaptureStatus` and an accepted sequence number. Invalid enum
fields and state values outside 0 through 511 are rejected. Allocation/collector
failure is returned as `ResourceFailure` and counted. Snapshot creation and the
caller-supplied lazy builder may throw, including `std::bad_alloc`; callers retain
their original canonical failure when reporting that exception. Export returns
`ExportError` and a native error number for native I/O failures. Export errors do
not recursively capture another event. The caller can record a bounded health
event if it has a separate available session.

The store retains at most 4,096 events and 4 MiB of bounded content, with at most
16 KiB per event. It measures the larger of the escaped JSON and Markdown event
views plus framing/index allowance before retention. Routine records can occupy
at most 3,840 events and 3.75 MiB, reserving 256 events and 256 KiB for essentials.
Essential faults/health evict routine records first, then the oldest essential
record when all retained records are essential. Routine traffic cannot evict
essential records. Health reports capacity drops, evictions, invalid input,
profile exclusions, omitted fields, redactions, truncation, cancellation, and
collector failures. Field text is limited to 256 UTF-8 bytes, context to 32 entries,
errors to eight, and underlying errors to eight per error. Large event detail is
trimmed to the event bound and disclosed. Bounds apply independently to every
session and snapshot; callers also own and limit how many snapshots they keep.

Only the fixed `Field` enum is accepted; there are no unrestricted payload maps,
request bodies, environment dumps, or attachments. `Text::classification` is the
primary privacy boundary: Personal and Secret values are replaced before storage.
Callers must explicitly classify opaque credentials and personal input at the
boundary that owns them. Public text is for reviewed non-sensitive or synthetic
evidence, not arbitrary exception/request strings. Additional conservative
recognizers omit whole fields containing password/authorization/token/cookie/key
markers, known credential prefixes, query credential markers, and user/home path
segments. These recognizers cannot identify every unlabeled opaque secret.
Invalid UTF-8 is replaced; control and bidirectional override characters become
visible code-point labels. Valid Unicode remains UTF-8. Export repeats this
privacy check on copied identity and all retained event values, then escapes JSON
and Markdown. Untrusted strings cannot introduce headings, active HTML, or links.

Exports require a caller-supplied existing absolute destination directory, reject
traversal and reparse points, and reserve a unique `ash-support-<bundle-id>` child
directory. Ordinary absolute drive and UNC destinations support paths beyond
`MAX_PATH` without caller registry changes or an executable long-path manifest.
Caller-supplied device or extended namespace destinations are rejected.
Returned `ExportResult` paths may carry a Windows extended-length prefix
(`\\?\` for drive paths or `\\?\UNC\` for UNC paths); use those returned paths for
subsequent file access. Path support does not establish UNC runtime qualification;
release evidence must identify the destination types actually exercised.
Callers choose a destination with the desired access controls;
new directories inherit that destination's ACL. No privileged directory is
required, and no file outside the reserved child is overwritten. Each bundle has
`evidence.json`, `evidence.md`, and `COMPLETED.json`. Files are written with native
exclusive creation and write-through, flushed, then renamed; the completion
marker is published last. Cancellation is checked between preparation/writing
steps. Reported failure attempts to remove only the exporter's own known files and
empty directory. Cleanup access/allocation failure, a crash, or forced process
termination can leave an incomplete directory;
consumers accept a bundle only when its completion marker and both file sizes and
checksums verify. The marker uses explicitly labeled noncryptographic FNV-1a-64
checksums for accidental-corruption checks, not authenticity or adversarial
integrity. There is no promised crash-time flush or stack capture. Windows
debugger/crash facilities remain separate host tools, not automatic collection.

Both formats use support schema `1.0`, distinct from product and canonical schema
versions. Their envelope contains bundle/session/profile, generated UTC,
capture start/end, build/source/environment identity, event count, health, limits,
incident index, retained timeline, export redactions, and limitations. A missing
identity is JSON `null` with an explicit reason; source dirty metadata describes
tracked source at build time and has stated provenance limits. Timestamps are UTC;
sequence and monotonic nanoseconds order events within one session. Clock order
does not establish causality across processes. Event `context` contains typed
fields; `errors` contain domain, nullable integer code, message, and ordered causes. Incident
entries reference a fault event and the retained sequence range. Its context and
errors form the account, and correlation IDs select its lead-up from the complete
retained timeline. Markdown includes the same envelope, incident index, and every
retained event and error; it does not produce an inferred causal narrative.
Null state/duration values mean unavailable or excluded, with the event's reason.

This console route works when no UI exists:

```cpp
#include <ash/SupportDiagnostics.hpp>
#include <filesystem>
#include <iostream>

int main(int argc, char** argv) {
    if (argc != 2) return 2; // caller chooses an existing absolute local directory
    ash::support::Session session;
    ash::support::Event fault;
    fault.kind = ash::support::EventKind::Fault;
    fault.level = ash::support::Level::Error;
    fault.component = {"ConsumerApp"};
    fault.operation_id = {"synthetic-rejection"};
    fault.message = {"Observed canonical operation rejection"};
    fault.context = {
        {ash::support::Field::CanonicalRule, {"ASH-CODEWORD-004"}},
        {ash::support::Field::UnknownEvidence, {"No stack captured"}}
    };
    const auto captured = session.capture(fault);
    if (captured.status != ash::support::CaptureStatus::Captured) return 3;
    const auto result = ash::support::export_snapshot(session.snapshot(),
        std::filesystem::path(argv[1]));
    if (result.error != ash::support::ExportError::None) return 4;
    std::cout << result.completion_marker.string() << '\n';
    return 0;
}
```

The native `ash-test-supportdiagnostics` target exercises controlled canonical
failure, profile/lazy selection, pre-storage privacy, Unicode/escaping, invalid
fields, event and byte overflow/reserve, immutable lifetime, cancellation,
shutdown, access-denied I/O, concurrent capture/snapshot/export, and measured
capture/snapshot/export duration. Set `ASH_SUPPORT_EVIDENCE_ROOT` to an external
evidence directory to retain synthetic bundles. Run the development-only verifier
with `python windows/tests/verify-support-exports.py <evidence-root>
--require-profiles Development Release --report <external-report.json>` to parse
JSON, verify marker checksums, and compare every retained Markdown field/error,
identity, count, and omission total. Tests print timings for 2,000 reused synthetic
fault captures, one snapshot, and one flushed local export using
`steady_clock`. These single-host measurements include validation/serialization,
bounded storage pressure, and the selected compiler optimization, with no claim of
a production workload baseline or a percentage overhead target. Actual build logs,
timings, pair checks, and Release archive inspection belong to release evidence,
not the SDK payload. Windows deployment-floor and ARM64 runtime qualification
remain separate from these host-compatible tests.
