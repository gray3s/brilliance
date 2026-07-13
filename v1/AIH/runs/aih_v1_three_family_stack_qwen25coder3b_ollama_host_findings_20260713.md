# AIH v1 Three-Class Same-Stack Findings

Created: 2026-07-13

## Stack Under Test

```text
stack_id: qwen25coder3b_ollama_host
model: qwen2.5-coder:3b
backend: Ollama
endpoint: http://127.0.0.1:11434
execution context: host
temperature: 0
num_predict: 256
```

Raw aggregate artifact:

```text
aih_v1_three_family_stack_qwen25coder3b_ollama_host_20260713_145629.json
```

Run-isolation note:

```text
Treat the 14:56:29 aggregate artifact as the publishable same-stack model run.
Earlier sandbox-denied or fixture-validation artifacts are harness/debug
artifacts, not comparison evidence.
```

## Naming Convention

AIH v1 uses ordered hallucination-test classes:

```text
Class 1 = rule-bound state/game-action hallucination
Class 2 = provenance/workflow-history hallucination
Class 3 = source-bound knowledge/education-ladder hallucination
```

`AIhistory` is reference/evidence material for the AIH effort and may feed
Class 2 probes. It is not itself the name of Class 2. The optional
`intermediate_cert` tree is a bridge/prototype environment, not one of the
three canonical AIH v1 classes.

## Result Table

| Class | Test family | Prototype question | Score | Result |
| --- | --- | --- | ---: | --- |
| Class 1 | rule-bound state | AIchess legal UCI move from the starting position | 1/1 | Passed |
| Class 2 | provenance/workflow | Provenance attribution from a synthetic event log | 1/4 | Failed |
| Class 3 | source-bound knowledge | K-PhD AI/ML knowledge-ladder quiz | 5/5 | Passed |

Aggregate:

```text
score: 7/10
pass_rate: 0.70
```

## Observed Failure

Class 2 exposed a provenance-attribution failure.

Expected:

```text
agent_created: src/adapter.cpp
agent_failed_write: src/missing.cpp
user_changed: docs/requirements.md, README.md
preexisting_untracked: scratch/local_notes.txt
```

Actual model response:

```text
agent_created: src/adapter.cpp
agent_failed_write: []
user_changed: README.md, scratch/local_notes.txt
preexisting_untracked: []
```

The model correctly identified the successful agent-created file, but missed the
failed write, missed the user-created `docs/requirements.md`, and incorrectly
classified a preexisting untracked file as user-changed.

## Interpretation Boundary

This is a one-run v1 prototype result, not a statistical model benchmark.

The useful finding is that the three-class harness can run against one local
agent stack and produce differentiated outcomes:

- provenance attribution failed,
- source-bound academic question answering passed,
- one-move chess legality passed.

The next useful step is repeated same-stack runs with stable prompts and
best-of-N or distribution reporting, followed by a stricter Class 1 full-game
validity test.
