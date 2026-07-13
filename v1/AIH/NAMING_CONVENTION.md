# AIH v1 Naming Convention

Created: 2026-07-13

## Purpose

Make AIH class, family, runner, and result names traceable and predictable.

## Hallucination Test Class Order

AIH v1 uses three canonical hallucination-test classes ordered from most severe
to least severe. A `class` is the ordered hallucination-test category. A test
family, probe, or stack configuration is subordinate to the class.

```text
class_1 = rule-bound state/game-action hallucination
class_2 = provenance/workflow-history hallucination
class_3 = source-bound knowledge/education-ladder hallucination
```

Meanings:

- `class_1_rule_bound_state`: state-fidelity and legal-action tests under an
  objective rule system.
- `class_2_provenance_workflow`: provenance, continuity, attribution, and
  project-flow failures.
- `class_3_source_bound_knowledge`: bounded academic knowledge-ladder tests
  with explicit source, time, I/O, grading, and assistance controls.

Current prototype examples:

- Class 1 prototype: AIchess legal-move/state-fidelity test.
- Class 2 prototype: synthetic provenance attribution event log.
- Class 3 prototype: K-PhD / AI-ML source-bound quiz.

`AIhistory` is reference/evidence material that justifies the hallucination
examination effort. It may supply source material for Class 2 tests, but it is
not itself the class name.

The optional `intermediate_cert` tree is a bridge/prototype test family. It is
useful for AI/ML certificate-style tests, but it is not one of the three
canonical AIH v1 classes.

Reports should keep this order unless a specific report states otherwise:

```text
Class 1 -> Class 2 -> Class 3
rule-bound state -> provenance/workflow -> source-bound knowledge
```

Public summaries may use concise labels such as `Class 1 AIH test`, `Class 2
AIH test`, or `Class 1 stack`. The repository implementation should carry the
specific probe, stack, and resource-profile details for readers who want them.

## Runner Names

Runner names use:

```text
run_<family>_probe_v1.py
```

Examples:

```text
run_aih_chess_probe_v1.py
run_class2_provenance_probe_v1.py
run_kphd_wikipedia_probe_v1.py
```

Aggregate runners use:

```text
run_v1_three_family_stack_probe.py
```

## Test IDs

Test IDs use:

```text
aih_<family>_<scope>_probe_v1_<yyyymmdd>
```

Example:

```text
aih_class2_provenance_probe_v1_20260713
```

## Result Files

Single-test result files use:

```text
<test_id>_<yyyymmdd_hhmmss>.json
```

Aggregate stack result files use:

```text
aih_v1_three_family_stack_<stack_id>_<yyyymmdd_hhmmss>.json
```

The `stack_id` must identify the agent stack used for all tests in the
aggregate run.

Example:

```text
aih_v1_three_family_stack_qwen25coder3b_ollama_host_20260713_150102.json
```

## Stack Naming

Short stack IDs should be lowercase, ASCII, and ordered from model to backend
to execution context:

```text
<model>_<backend>_<context>
```

Current local example:

```text
qwen25coder3b_ollama_host
```

Longer HybridAI stack codes may also be used when a formal lookup table is
available. In that case, record both the short stack ID and the expanded stack
profile.

## Run Isolation Rule

Do not run multiple AIH simulations at once when generating evidence intended
for comparison or publication.

Fixture/harness validation, model runs, and aggregate runs should be serialized
unless a test is explicitly designed as a concurrency/load test. Parallel runs
can contaminate timing, memory pressure, endpoint state, cache behavior, and
resource contention. If concurrency is the variable under test, label it as a
concurrency/load test and record the hardware/runtime envelope.

For ordinary AIH v1 evidence, publish only serialized model-run artifacts.

## CHRR Direction

AIH is working toward a CHRR-style metric: common hallucination rejection or
resistance rate.

The goal is to estimate how resistant a human/AI pair is to common causes of
combined human and AI hallucination, not only whether it avoids rare dramatic
rail-excursions.
Candidate inputs include bad assumptions, stale state, weak evidence, ambiguous
scope, unverified claims, invalid tool output, unsupported attribution, and
fluent answers that should have been rejected.

Each AIH class should define observable rejection events, accepted-failure
events, and evidence records that can support a CHRR-like metric.
