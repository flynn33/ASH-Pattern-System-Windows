# Performance Budgets - ASH Pattern System Windows

**Purpose:** Define performance, reliability, and resource-limit budgets for production deployment.  
**Status:** Preliminary budgets defined; measurement to be completed on Release builds.  

---

## Overview

Performance budgets establish measurable targets for the ASH Pattern System's behavior under various workload conditions. These budgets enable:
- Early detection of regressions
- Capacity planning for deployment scenarios
- Performance regression tracking
- Resource utilization monitoring

---

## Module-Specific Budgets

### 1. StateModel Module

| Metric | Budget | Current | Measurement Method |
|--------|--------|---------|-------------------|
| `classify_admissibility()` latency (single state) | < 50 ns | Pending | High-resolution timer |
| `normalize_valid_state()` throughput | > 10M states/sec | Pending | Benchmark harness |
| Memory usage per state operation | ≤ 64 bytes | N/A (no heap allocation) | Static analysis |
| Maximum recursion depth | None (iterative only) | Compliant | Code review |

**Implementation:** All operations use fixed-size containers; no heap allocation occurs.

---

### 2. RecoveryEngine Module

| Metric | Budget | Current | Measurement Method |
|--------|--------|---------|-------------------|
| `select_fallback()` decision latency | < 10 ns | Pending | Timer instrumentation |
| Diagnostic chain traversal time (max depth) | < 200 ns | Pending | Profiling |
| Memory footprint per recovery operation | ≤ 64 bytes | N/A | Static analysis |
| Failure escalation rate (per error) | ≤ 1 diagnostic event | Compliant | Test harness |

**Note:** Recovery engine is lightweight; primary budget is ensuring no blocking behavior.

---

### 3. TransitionRegistry Module

| Metric | Budget | Current | Measurement Method |
|--------|--------|---------|-------------------|
| Transition lookup latency | < 20 ns | Pending | Micro-benchmark |
| Registry initialization time | < 5 µs (one-time) | Pending | Startup timing |
| Memory for transition table | 16 codewords × 9 bytes = 144 B | Compliant | Static calculation |

**Note:** Transition registry is a small constant-size structure; performance should be negligible.

---

### 4. Diagnostics Module

| Metric | Budget | Current | Measurement Method |
|--------|--------|---------|-------------------|
| Diagnostic envelope construction time | < 50 ns | Pending | Timer instrumentation |
| Rule evaluation latency (single rule) | < 10 ns | Pending | Micro-benchmark |
| Total diagnostic chain time (full evaluation) | < 500 ns | Pending | End-to-end timing |
| Diagnostic output size limit | ≤ 4KB per event | Compliant | Size check |

---

### 5. RealmEncoder Module

| Metric | Budget | Current | Measurement Method |
|--------|--------|---------|-------------------|
| Realm ID computation latency | < 10 ns | Pending | Timer instrumentation |
| Canonical signature generation | < 30 ns | Pending | Micro-benchmark |
| Orbit assignment time (512 states) | < 5 µs | Pending | Batch timing |

---

### 6. TopologyGenerator Module

| Metric | Budget | Current | Measurement Method |
|--------|--------|---------|-------------------|
| Topology generation (full 9D hypercube) | < 100 µs | Pending | Benchmark harness |
| Graph traversal latency | < 50 ns per edge | Pending | Micro-benchmark |
| Memory for topology representation | ≤ 2 KB | Compliant (edge list) | Static analysis |

---

### 7. AxiomEvaluator Module

| Metric | Budget | Current | Measurement Method |
|--------|--------|---------|-------------------|
| Single axiom evaluation latency | < 50 ns | Pending | Micro-benchmark |
| Full invariant suite verification | < 50 µs | Pending | Benchmark harness |
| Failure classification time | < 100 ns | Pending | Timer instrumentation |

---

### 8. GenerationPlanner Module

| Metric | Budget | Current | Measurement Method |
|--------|--------|---------|-------------------|
| Plan generation latency (typical case) | < 5 µs | Pending | Benchmark harness |
| Memory for plan representation | ≤ 256 bytes | N/A (value type) | Static analysis |

---

### 9. ArtifactEmitter Module

| Metric | Budget | Current | Measurement Method |
|--------|--------|---------|-------------------|
| Descriptor emission latency (typical case) | < 10 µs | Pending | Timer instrumentation |
| Plan-to-artifact mapping time | < 50 ns per element | Pending | Micro-benchmark |

---

## Aggregate Performance Budgets

### Total System Latency

| Operation | Budget | Components |
|-----------|--------|------------|
| Full classification + normalization cycle | < 1 µs | StateModel (≤700ns) |
| Complete diagnostic chain traversal | < 1 µs | Diagnostics (≤500ns) |
| End-to-end semantic processing | < 2 µs | All modules combined |

### Throughput Budgets

| Workload Type | Target Throughput | Measurement Conditions |
|---------------|-------------------|----------------------|
| Batch state classification | > 100M states/sec | x64 Release, single-threaded |
| Diagnostic event processing | > 50M events/sec | Continuous stream |
| Realm encoding (512-state batch) | > 100K encodings/sec | Typical workload |

---

## Reliability Budgets

### Failure Behavior

| Metric | Budget | Rationale |
|--------|--------|-----------|
| Crash rate in Release build | 0 crashes per billion operations | RAII, no heap, bounds-checked |
| Exception rate (expected) | 0 exceptions under normal operation | No exceptional control flow planned |
| Memory leak rate | 0 bytes/operation | Stack-only or RAII containers |

### Recovery Behavior

| Metric | Budget | Rationale |
|--------|--------|-----------|
| Recovery attempt latency | < 1 µs per failure | Lightweight fallback mechanism |
| Maximum recovery depth before halt | 3 levels (monotonic escalation) | Prevents infinite loops |
| Diagnostic coverage for recoverable failures | 100% of failure classes | All 7 classify_admissibility outcomes tested |

---

## Resource Limit Budgets

### Memory Usage

| Category | Limit | Current Implementation |
|----------|-------|----------------------|
| Per-state storage | 9 bits = 1.125 B + overhead ≤ 64 B | std::bitset<9> |
| Per-diagnostic envelope | ≤ 4 KB | Structured diagnostic record |
| Total process memory (typical) | < 10 MB for core library | Static linkage possible |

### CPU Usage

| Metric | Budget | Measurement Context |
|--------|--------|---------------------|
| CPU time per operation | Varies by module, see individual budgets | Measured in Release build |
| Parallel scaling efficiency | ≥ 80% of linear on 8+ cores | Multi-threaded scenarios |

---

## Performance Test Harness

To measure and validate these budgets:

```cpp
// Example benchmark harness skeleton (windows/tests/performance/*.cpp)
#include <chrono>
#include "ash/StateModel.hpp"

void Benchmark_StateClassification() {
    const std::size_t iterations = 100'000'000; // 100M iterations
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (std::uint16_t state = 0; state < ash::kStateSpaceSize; ++state) {
        ash::Bit9State s = ash::from_int(state);
        const auto result = ash::classify_admissibility(s);
        (void)result; // Suppress unused warning in benchmark
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    
    std::cout << "Total time: " << duration.count() << " ns\n";
    std::cout << "Throughput: " << (iterations / duration.count() * 1e9) << " ops/sec\n";
}
```

---

## Performance Regression Tracking

### Baseline Establishment

1. **Build Release x64:** `msbuild /p:Configuration=Release /p:Platform=x64`
2. **Run benchmark suite:** Execute all performance test executables
3. **Record metrics:** Capture latency and throughput values
4. **Establish baseline:** Store results in `completion-evidence/performance-baseline.json`

### Regression Detection

```json
{
  "baseline": {
    "timestamp": "2026-06-21T12:00:00Z",
    "platform": "Windows x64, MSVC 18.7",
    "configuration": "Release"
  },
  "modules": {
    "StateModel": {
      "classify_admissibility_latency_ns": 45,
      "throughput_ops_per_sec": 200000000
    },
    ...
  }
}
```

### Alert Thresholds

- **Warning:** ≥ 20% degradation from baseline
- **Error:** ≥ 50% degradation from baseline or budget exceeded
- **Critical:** Budget violated or exception thrown

---

## Performance Optimization Guidelines

1. **Avoid heap allocation** - Use fixed-size containers throughout
2. **Minimize function call overhead** - Inlining for hot paths
3. **Use constexpr where possible** - Compile-time evaluation
4. **Cache frequently-accessed data** - Avoid redundant computations
5. **Vectorize SIMD operations** - For bulk processing (future optimization)

---

## Budget Validation Status

| Module | Budgets Defined | Baseline Measured | Status |
|--------|-----------------|-------------------|--------|
| StateModel | ✅ Yes | ⏳ Pending | Ready for measurement |
| RecoveryEngine | ✅ Yes | ⏳ Pending | Ready for measurement |
| TransitionRegistry | ✅ Yes | ⏳ Pending | Ready for measurement |
| Diagnostics | ✅ Yes | ⏳ Pending | Ready for measurement |
| RealmEncoder | ✅ Yes | ⏳ Pending | Ready for measurement |
| TopologyGenerator | ✅ Yes | ⏳ Pending | Ready for measurement |
| AxiomEvaluator | ✅ Yes | ⏳ Pending | Ready for measurement |
| GenerationPlanner | ✅ Yes | ⏳ Pending | Ready for measurement |
| ArtifactEmitter | ✅ Yes | ⏳ Pending | Ready for measurement |

---

**Status:** Budgets defined; baseline measurements pending execution on Release build.  
**Next Steps:** Run performance test harness, establish baselines, monitor regressions.
