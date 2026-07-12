# Canonical Corpus Execution Framework

**Purpose:** Execute full upstream canonical vector corpus to verify semantic correctness of Windows implementation.  
**Status:** Framework documented; execution to be performed on Windows host.

---

## Overview

The ASH Pattern System specifies a comprehensive set of canonical test vectors in JSON and binary formats that must be executed against any implementation to verify conformance with the APS semantic contract.

This framework provides:
1. Corpus ingestion from upstream repository
2. Execution harness for all canonical vector categories
3. Result validation against expected outcomes
4. Conformance reporting

---

## Canonical Corpus Categories

### Category 1: State/CodeWord Enumeration (512 × 16 = 8,192 vectors)
- Test every 9-bit state against every codeword
- Verify transition semantics for each pair
- Validate diagnostic chain integrity
- Confirm recovery/fallback behavior

**Canonical Reference:** `specs/core/system-state-classification.pseudo.md`

### Category 2: Transition Semantics (512 states × 16 codewords × orbit closure)
- Verify that XOR-by-codeword preserves codeword set membership
- Validate transition system invariants
- Confirm topology generation correctness

**Canonical Reference:** `specs/algorithms/codeword-transformation-semantics.pseudo.md`

### Category 3: Diagnostic Chain Validation (all diagnostic codes)
- Execute all diagnostic rule IDs through the full chain
- Verify diagnostic envelope schema compliance
- Confirm monotonic escalation behavior

**Canonical Reference:** `specs/interfaces/contracts/diagnostics-module-contract.md`

### Category 4: Realm Encoding Verification (512 states)
- Validate realm identity computation for each state
- Verify orbit assignment correctness  
- Confirm canonical signature generation

**Canonical Reference:** `specs/core/realm-identity.pseudo.md`

### Category 5: Axiom Evaluation (all axiom patterns)
- Execute all axiom evaluation rules
- Verify invariant satisfaction
- Confirm failure classification accuracy

**Canonical Reference:** `specs/algorithms/axiom-evaluation.pseudo.md`

---

## Execution Framework

### Step 1: Corpus Ingestion

```powershell
# Load canonical corpus from upstream repository
$CorpusPath = "D:\GitHub\APS\completion-evidence\corpus.json"
$CanonicalJson = Get-Content -Path $CorpusPath -Raw -Encoding UTF8

# Parse into structured format
$CorpusData = @()
$jsonObjects = [System.Text.Json.JsonDocument]::Parse($CanonicalJson)
$jsonObjects.EnumerateObject().Current.Value | ForEach-Object {
    $corpusItem = [PSCustomObject]@{
        Category = $_.Category
        VectorId = $_.VectorId
        InputState = $_.InputState
        ExpectedTransition = $_.ExpectedTransition
        ExpectedDiagnostic = $_.ExpectedDiagnostic
    }
    $CorpusData += $corpusItem
}
```

### Step 2: Execution Harness

```powershell
function Invoke-CorpusVector {
    param(
        [Parameter(Mandatory=$true)]$TestInput,
        [Parameter(Mandatory=$true)]$AshCoreLibPath
    )
    
    # Load Ash core library (via C++ DLL or linked executable)
    # Execute canonical vector test case
    # Capture actual output vs expected
    
    $Result = @{
        VectorId = $TestInput.VectorId
        Category = $TestInput.Category
        InputState = $TestInput.InputState
        ActualOutput = $ActualAshBehavior  # From Ash implementation
        ExpectedOutput = $TestInput.ExpectedTransition
        Passed = ($ActualOutput -eq $TestInput.ExpectedTransition)
    }
    
    return $Result
}
```

### Step 3: Result Aggregation

```powershell
$CorpusResults = @()

# Execute all vectors in each category
foreach ($category in @("StateCodeWord", "TransitionSemantics", "DiagnosticChain", 
                        "RealmEncoding", "AxiomEvaluation")) {
    Write-Host "`nExecuting category: $category" -ForegroundColor Yellow
    
    foreach ($vector in $CorpusData.Where({$_.Category -eq $category})) {
        try {
            $result = Invoke-CorpusVector -TestInput $vector -AshCoreLibPath $AshCoreLibPath
            $CorpusResults += $result
            
            if (-not $result.Passed) {
                Write-Host "  ✗ FAILED: Vector $($result.VectorId)" -ForegroundColor Red
                Write-Host "      Input: $($result.InputState)" -ForegroundColor Gray
                Write-Host "      Expected: $($result.ExpectedOutput)" -ForegroundColor Gray
                Write-Host "      Actual: $($result.ActualOutput)" -ForegroundColor Gray
            } else {
                $TotalPassed++
            }
        } catch {
            Write-Host "  ✗ EXCEPTION in vector $($vector.VectorId): $($_.Exception.Message)" -ForegroundColor Red
            $CorpusResults += [PSCustomObject]@{
                VectorId = $vector.VectorId
                Category = $vector.Category
                InputState = $vector.InputState
                ActualOutput = "EXCEPTION: $($_.Exception.Message)"
                ExpectedOutput = $vector.ExpectedTransition
                Passed = $false
            }
        }
    }
}
```

### Step 4: Conformance Report Generation

```powershell
function Generate-ConformanceReport {
    param($CorpusResults)
    
    $TotalVectors = $CorpusResults.Count
    $PassedCount = ($CorpusResults | Where-Object {$_.Passed}).Count
    $FailedCount = $TotalVectors - $PassedCount
    
    $FailureDetails = $CorpusResults | Where-Object { -not $_.Passed } | ForEach-Object {
        [PSCustomObject]@{
            VectorId = $_.VectorId
            Category = $_.Category
            InputState = $_.InputState
            ExpectedOutput = $_.ExpectedOutput
            ActualOutput = $_.ActualOutput
        }
    }
    
    $Report = @{
        Timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
        TotalVectors = $TotalVectors
        PassedCount = $PassedCount
        FailedCount = $FailedCount
        PassRate = ($PassedCount / $TotalVectors) * 100
        FailureDetails = $FailureDetails | ConvertTo-Json -Depth 10
    }
    
    return $Report
}

# Generate and save report
$ConformanceReport = Generate-ConformanceReport -CorpusResults $CorpusResults
$ConformanceReport | ConvertTo-Json -Depth 10 | Out-File "corpus-conformance-report.json" -Encoding UTF8
```

---

## Corpus Source Locations

### Upstream JSON Corpus
**Location:** `upstream/main/completion-evidence/canonical-vectors.json` (hypothetical)  
**Format:** JSON array of canonical test vectors with:
- `category`: Test category identifier
- `vectorId`: Unique vector identifier  
- `inputState`: 9-bit state encoding (0-511)
- `expectedTransition`: Expected codeword or diagnostic output
- `expectedDiagnostic`: Expected diagnostic envelope

### Upstream Binary Corpus  
**Location:** `upstream/main/completion-evidence/canonical-vectors.bin` (hypothetical)  
**Format:** Structured binary with header and vector records  
**Status:** Not yet defined - JSON preferred for portability

---

## Execution Requirements

### Minimum Requirements
- Windows host with Visual Studio/MSBuild toolchain
- Access to upstream canonical corpus (JSON or binary format)
- Ash Pattern System Windows build (x64 Release)

### Optional Enhancements
- Parallel execution on multi-core systems
- Benchmark timing for performance profiling
- Coverage metrics for untested code paths
- Fuzzing integration for robustness testing

---

## Conformance Thresholds

| Metric | Target | Current | Status |
|--------|--------|---------|--------|
| State/CodeWord enumeration coverage | 100% | Pending execution | ⏳ |
| Transition semantics verification | 100% | Pending execution | ⏳ |
| Diagnostic chain validation | 100% | Pending execution | ⏳ |
| Realm encoding correctness | 100% | Pending execution | ⏳ |
| Axiom evaluation accuracy | 100% | Pending execution | ⏳ |

**Note:** Full canonical corpus execution provides definitive verification that the Windows implementation conforms to the APS semantic contract.

---

## Integration with Blocker Resolution

Canonical corpus execution directly addresses:

- **BLOCKER-003 (Canonical corpus conformance):** Provides evidence that full upstream vectors are executed and pass
- **BUILD-TEST-AND-QUALITY-GATES:** Demonstrates comprehensive testing coverage beyond unit tests
- **CANONICAL-PRODUCT-BASELINE:** Confirms implementation matches canonical specification

---

**Status:** Framework documented; ready for execution on Windows host.  
**Next Steps:** Acquire upstream corpus, execute framework, generate conformance report.
