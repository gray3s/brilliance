# AIH v1 Test Classes

Created: 2026-07-13

AIH v1 organizes hallucination tests into three canonical classes ordered from
most severe to least severe:

```text
class_1 = rule-bound state/game-action hallucination
class_2 = provenance/workflow-history hallucination
class_3 = source-bound knowledge/education-ladder hallucination
```

This is the hallucination-test order used in reports and aggregate runs.

`AIhistory` is reference/evidence material for the overall AIH effort and may
feed Class 2 prototypes. It is not the name of Class 2.

See [NAMING_CONVENTION.md](NAMING_CONVENTION.md) for class, runner, test ID,
result-file, and stack-ID rules.

Project goals and boundary choices are tracked in
[PROJECT_GOALS.md](PROJECT_GOALS.md).

## CHRR Direction

AIH is working toward a CHRR-style metric that measures resistance to common
sources of combined human and AI hallucination. The target is not only
catastrophic failure; it is whether ordinary hallucination pressure is caught
and rejected before it becomes project state.

## Current Runnable v1 Probes

- Class 1: `AIchess/v1/run_aih_chess_probe_v1.py`
- Class 2: `AIhistory/v1/run_aihistory_provenance_probe_v1.py`
- Class 3: `k-phd/v1/run_kphd_wikipedia_probe_v1.py`

## Aggregate Same-Stack Probe

Use:

```bash
./run_v1_three_family_stack_probe.py --model qwen2.5-coder:3b
```

This runs all three canonical v1 classes against the same local Ollama model
stack and writes an aggregate JSON record under:

```text
runs/
```
