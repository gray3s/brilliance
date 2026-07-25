# AIChess v2 Agent Sort Checkpoint - 2026-07-25

Public link target:
https://github.com/gray3s/brilliance/blob/main/aih/aichess/v2/project-development-logs/aichess_v2_agent_sort_checkpoint_20260725.md

## Current Taxonomy

The temporary Class0/Class1 naming from the first pass has been replaced.

- `Class1`: general K-PhD capability evaluation, using canonical labels
  `Class 1:<level>:<area>:<test>`.
- `Class2`: game tests, including chess, checkers, go, poker, and backgammon.
- `Class3`: certification tests built from multiple Class1 components and,
  when appropriate, Class2 game components.

Class1 general areas are `1 language`, `2 logic`, `3 geography`, `4 math`,
`5 physics`, and `6 chemistry`.

`Class 1:0` is the floor, but it is not just "transport works." It should
show minimal coherent response:

- `Class 1:0:1:1`: language - coherent output to the simplest prompt
- `Class 1:0:1:2`: language - echo a simple word, such as `one`
- `Class 1:0:2:1`: logic - basic symbol recognition, such as `"a"` is a letter and `"2"` is a number
- `Class 1:0:3:1`: geography - basic place/category recognition
- `Class 1:0:4:1`: math - basic number recognition or single-step arithmetic
- `Class 1:0:5:1`: physics - basic physical-world category recognition
- `Class 1:0:6:1`: chemistry - basic matter/material category recognition

The full taxonomy is recorded in:

```text
project-development-logs/aichess_v2_test_class_taxonomy_20260725.md
```

## Stack Configuration

All tested agents were local Ollama stacks using:

- Provider: Ollama local HTTP API
- Shim: `ollama_generate`
- Non-Qt screening path: `./aichess.sh --eval-mode ollama_generate_uci_srcvalmvs`
- Existing binary/Qt self-play path retained as the stricter bf/af
  board-transition protocol test
- Board source: Stockfish/rules harness

Implemented AIChess clue modes:

- `0`: no extra clue
- `1`: valid UCI move set
- `2`: board-state-valid clue
- `3`: both valid UCI move set and board-state-valid clue

There is no implemented clue mode `4`.

## Class2 Chess Entry Candidates

These agents produced at least one legal UCI chess move from the start position
under the simpler non-Qt contract. That is only an entry screen, not proof of
reliable chess play.

| Key | Agent | Skill class | Entry result | Selected move | Clue 0 | Clue 1 | Clue 2 | Clue 3 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 11 / agent1 | granite4:3b | Class 2.chess entry | pass | e2e4 | fail | fail | fail | fail |
| 12 / agent2 | qwen2.5-coder:3b | Class 2.chess entry | pass | e2e4 | fail | fail | fail | fail |
| 14 / agent4 | qwen2.5:latest | Class 2.chess entry | pass | e2e4 | fail | fail | fail | fail |
| 15 / agent5 | qwen:4b | Class 2.chess entry | pass | a2a3 | fail | fail | fail | fail |
| 19 / agent9 | llama3.2:1b | Class 2.chess entry | pass | a2a3 | fail | fail | fail | fail |
| 1C / agent12 | phi3:mini | Class 2.chess entry | pass | b1a3 | fail | fail | fail | fail |
| 1D / agent13 | phi4-mini:latest | Class 2.chess entry | pass | g1f3 | fail | fail | fail | fail |
| 1E / agent14 | mistral:latest | Class 2.chess entry | pass | a2a3 | fail | fail | fail | fail |

The clue-mode rows still fail under the current Qt/bf-af board-transition
contract. That is useful signal: raw models can sometimes produce legal moves,
but the full board-state protocol needs better prompting, training examples, or
a less brittle adapter.

## Class1 Capability / Training Candidates

These downloaded local agents failed the one-move chess entry screen. They are
not discarded. They should be mapped through Class1 general capability tests
and later through Class2 games other than chess where useful.

| Key | Agent | Observed Class1 / skill notes |
| --- | --- | --- |
| 13 / agent3 | qwen2.5:0.5b | smallest Qwen baseline; responds well to simple prompts; produced correct content for arithmetic/spelling/geography except checkers rule in latest sample |
| 16 / agent6 | robit/qwen3.5-9b-r7-research:q4km | currently poor fit for short-answer probes; needs separate diagnosis |
| 17 / agent7 | smollm2:135m | verbose responses often contain useful facts, but chess entry failed |
| 18 / agent8 | gemma3:270m | handles some simple copy/selection/geography tasks |
| 1A / agent10 | gemma3:1b | handles several simple Class1-style prompts and basic checkers rule |
| 1B / agent11 | tinyllama:latest | handles exact copy and basic checkers rule, but often answers verbosely |

## Evidence Artifacts

Latest full non-Qt Class2.chess entry screen:

```text
runs/aichess_eval_260725021811_y4wKgo/results.jsonl
```

Latest Class 1:0-style liveness/capability probe run:

```text
runs/class0_capability_260725025113_556NXg/results.jsonl
```

Latest taxonomy file:

```text
project-development-logs/aichess_v2_test_class_taxonomy_20260725.md
```

## Immediate Lesson

This is a curriculum and compatibility-mapping problem, not just model
selection.

The useful question is not only "can it play chess?" It is:

- What does the stack respond to?
- What general Class1 level does it reach?
- Which Class2 games can it perform?
- What fails first?
- What training data would move it up the ladder?
