# AI Chess Match AIH v2

Created: 2026-07-12
Forked from upgraded v1: 2026-07-22

Shared AIH role vocabulary:

```text
../../AIH_ROLE_TERMS_v2_20260722.md
```

## Purpose

AIChess v2 examines which `aiversion`s can function through the local
`ollama_generate` shim under the AIChess shell harness.

AI Chess Match tests agentic AI state fidelity, legal action, timing,
rule-following, and hallucination behavior in a bounded chess environment.

## 20260722 Table-Backed Plot Checkpoint

The current v2 shell checkpoint records module descriptions and test rows in:

```text
module_description.csv
test_table.csv
```

`module_description.csv` identifies the modules used in the test chain.
`test_table.csv` records run counts, fail percentages, and the compact I/O
state clock for the specific hard-wired module chain under test.

The first plot artifacts are simple 1280x720 24-bit BMPs:

```text
plots/agent_size_vs_failure_rate_20260722.bmp
plots/source_family_failure_rate_20260722.bmp
```

Links:

- [Agent size vs failure rate](plots/agent_size_vs_failure_rate_20260722.bmp)
- [Source/family vs failure rate](plots/source_family_failure_rate_20260722.bmp)

These plots are preliminary. Each point currently comes from one assisted-mode
run per installed local Ollama agent. Repeated runs are needed before the error
bars can carry useful failure uncertainty.

Handholding mode for all points currently shown:

```text
00 no board sourced, no valid-move list sourced
10 board sourced only
01 valid-move list sourced only
11 board sourced and valid-move list sourced
```

All current plot points use mode `11`. The other clue modes are not included in
these plots yet.

## Position In AIH

```text
AIH test family 1: personal-history agentic-AI failure tests
AIH test family 2: K-PhD knowledge-ladder tests
AIH test family 3: AI Chess Match state-fidelity tests
```

V2 keeps the current deterministic tester/grader shape and focuses on the
solver role:

```text
cbase=qtcpp+bash
harness=aichess_v2_shell
shim=ollama_generate
test=dtest
solve=aisolve
grade=dgrade
srcvalbrd=yes
srcvalmvs=yes|no
```

The clue-supplied comparison path uses deterministic testing/setup and
deterministic refereeing/grading. The AI agent performs only the solver role:

```text
dtest -> aisolve -> dgrade
```

The immediate v2 question is:

```text
Which local Ollama-compatible aiversions can produce valid AIChess moves under
the same shell harness, first with validated moves hidden and then sourced?
```

## Core Idea

A referee owns the official board state. Agents observe the board and propose
moves. The referee validates moves, applies legal moves, records time, and
classifies game endings.

The current Class1 v2 player test is a board-transition test. In the default
mode, the player receives the current full FEN but does not receive the legal
move list and does not return a move field. The player returns only:

```text
bf=<the full current board FEN exactly as given>
af=<the full board FEN after the player move>
```

The harness derives `mv` from the reported `bf -> af` transition by comparing
the reported after-state with the deterministic after-state for each hidden
legal move. The referee result `rf` is legal only when exactly one hidden legal
move explains the transition. This keeps the legal move list out of the normal
agent prompt while still using deterministic rules for scoring.

An assisted comparison mode is available with `--srcvalmvs yes`. That mode gives
the player both the board state and the complete immediate valid UCI move set
for the current board position. It is useful for A/B testing, but it is not the
default AIH measurement path because it lets the agent choose from a supplied
valid-move set.

The shell wrapper now exposes that prompt-support choice with the AIH term
`srcvalmvs`:

```text
srcvalmvs=yes  source the complete immediate valid move set into the solver prompt
srcvalmvs=no   hide the immediate valid move set from the solver prompt
```

`--legal-list` and `--feed-legal-list` are compatibility aliases for
`--srcvalmvs yes`. `--hide-legal-list` is a compatibility alias for
`--srcvalmvs no`. `--srcmvs` and `--srcvalmovs` remain accepted as older
compatibility aliases.

V1 should grow toward board-based player-agent and referee-team chess tests.
The current scope is intentionally limited to board assignments, one player
agent per side per board, and board-specific referee coverage before adding any
other role types.

Starter plan:

```text
multi_agent_chess_test_plan_20260715_1942MDT.md
```

Local-hardware vs cloud-software benchmark package:

```text
local_vs_cloud_benchmark_package_20260715_1943MDT.md
```

Concrete runner and fixture package artifacts:

```text
./aichessv1
./aichess.sh
class1_cpp/class1_aichess_fixture.cpp
class1_cpp/build_class1_fixture.sh
class1_cpp/run_class1_fixture.sh
./aichess.sh -nb 2
class2_cpp/class2_aichess_fixture.cpp
class2_cpp/build_class2_fixture.sh
class2_cpp/run_class2_fixture.sh
./aichess.sh -nb 4
class3_cpp/class3_aichess_fixture.cpp
class3_cpp/build_class3_fixture.sh
class3_cpp/run_class3_fixture.sh
class1_cpp_test/class1_aichess_fixture_test.cpp
class1_cpp_test/build_class1_fixture.sh
class1_cpp_test/run_class1_fixture.sh
configs/aichess_class1_one_board_one_agent_sides_one_referee_v1_20260715_2110MDT.json
configs/aichess_class1_one_board_one_agent_sides_three_referees_v1_20260715_2016MDT.json
configs/aichess_class2_two_boards_one_agent_sides_one_referee_each_v1_20260715_2016MDT.json
configs/aichess_class3_four_boards_one_agent_sides_four_referees_v1_20260715_2016MDT.json
configs/aichess_option_two_boards_one_agent_sides_three_referees_each_v1_20260715_2110MDT.json
configs/aichess_option_four_boards_one_agent_sides_three_referees_each_v1_20260715_2110MDT.json
schemas/multi_agent_chess_result_schema_v1_20260715_1943MDT.md
```

Run instructions:

```text
AICHESS_IMPLEMENTATION_INSTRUCTIONS_20260715_1951MDT.md
RUN_CLASS1_AICHESS_TESTS_20260715_1945MDT.md
```

## Class Movement Rule

AIChess-related tests can be classified into Class 2 or Class 3 when the target
failure mode changes. Core legal-move/state-fidelity/referee/timing tests remain
Class 1. Workflow/provenance tests about what the agent claims it did, verified,
changed, or logged should be Class 2. Source-bound chess knowledge or
instructional-source tests should be Class 3.

Current role-count convention:

```text
Basic Class 1 AIchess core = one board, one player agent per side, one referee
Class 2 AIchess core = two boards, one player agent per side per board, one referee per board
Class 3 AIchess core = four boards, one player agent per side per board, four referees total
```

Three-referee-per-board configs are available as a stronger referee option for
the 1-board, 2-board, and 4-board shapes.

Any multi-referee-per-board run must preserve referee-dependent statistics:
per-referee votes, per-board referee agreement, majority results,
disagreement counts, legal-vote rates, and referee-specific latency/error
fields when available.

Near-term implementations should use board assignments, player agents, and
referee teams only. Other role types are deferred until the board-runner
package is stable.

The active v1 harness uses C++/Qt/Bash only. No Python runner, Python module,
Python cache, or Python-generated active fixture is part of the supported
AIChess v1 path.

`aichessv1` is the compiled C++ runner with wrapper-style CLI handling. The
shell wrapper remains available for compatibility.

## AIH Role Terms

AIH v2 terminology separates the test roles from their implementations:

```text
cbase    code language/runtime family, such as bash, cpp17, or qtcpp
harness  orchestration layer that runs roles, prompts, timeouts, and logs
shim     adapter between the harness and an AI version
aiversion specific AI version or local alias
test     task-producing role
solve    task-solving role
grade    scoring role
check    independent validation/audit role
qa       final review layer
```

Role implementations use `ai` and `d` prefixes:

```text
aitest   AI creates/selects the test
aisolve  AI solves the test
aigrade  AI grades the result
dtest    deterministic code creates/selects the test
dsolve   deterministic code solves the test
dgrade   deterministic code grades the result
```

Checks use verb pairs:

```text
dtest_check
dsolve_check
dgrade_check
```

The current AIChess v2 baseline is:

```text
cbase=qtcpp+bash
harness=aichess_v2_shell
shim=ollama_generate
test=dtest
solve=aisolve
grade=dgrade
srcvalbrd=yes
srcvalmvs=yes|no
```

The shell wrapper accepts `aiqwenN` aliases for AI versions. They are mapped to
the size-sorted local Qwen models reported by Ollama:

```bash
./aichess.sh --list-models
./aichess.sh --srcvalmvs no -w aiqwen1:3 -b aiqwen1:3 -r harness -nb 3
./aichess.sh --srcvalmvs yes -w aiqwen1:3 -b aiqwen1:3 -r harness -nb 3
```

## V2 Aiversion Sweep

The preferred v2 shell path is:

```bash
./aichess.sh --list-models
./aichess.sh --loglvl 2 --srcvalmvs no -w aiqwen1:3 -b aiqwen1:3 -r harness -nb 3 -nl 1 -mxply 2 -sto 145 -otkns 512
./aichess.sh --loglvl 2 --srcvalmvs yes -w aiqwen1:3 -b aiqwen1:3 -r harness -nb 3 -nl 1 -mxply 2 -sto 145 -otkns 512
```

Current alias terms:

```text
aiqwenN   AI-version alias for the Nth local Qwen model sorted by size
qwenN     compatibility alias accepted by older v1 commands
graniteN  AI-version alias for the Nth local Granite model sorted by size
shim      adapter between harness and aiversion
cbase     code language/runtime family
```

Example default Class1 qwen4 run:

```bash
./aichessv1 --loglvl 2 -nb 1 -nl 1 -mxply 6 -sto 120 -otkns 256 qwen4
```

Example assisted comparison run:

```bash
./aichessv1 --loglvl 2 --legal-list -nb 1 -nl 1 -mxply 6 -sto 120 -otkns 256 qwen4
```

Preferred shell-wrapper examples:

```bash
./aichess.sh --loglvl 2 --srcvalmvs no -nb 1 -nl 1 -mxply 6 -sto 120 -otkns 256 aiqwen1
./aichess.sh --loglvl 2 --srcvalmvs yes -nb 1 -nl 1 -mxply 6 -sto 120 -otkns 256 aiqwen1
```

Level-2 transcript format:

```text
ho: "<full prompt sent by harness to Ollama>"
hi: "<full response returned from Ollama to harness>"
mv: "<move derived by the harness from bf/af>"
rf: "<referee/checker result>"
```

`ho` and `hi` preserve original line breaks and are not merged across separate
requests or responses.

## Measurements

- legal move rate,
- illegal move count,
- board-state hallucination,
- move time,
- time fault,
- move fault,
- plies to game end,
- game-ending type,
- local-vs-cloud stack comparison,
- local-hardware vs cloud-software benchmark comparison,
- single-agent vs multi-agent comparison,
- mixed-agent role comparison,
- referee agreement/disagreement,
- suitability of each agent stack for bounded stateful tasks.

## Current Source Material

- AI Chess Match plan: `../agent_chess_project_development_plan_20260712.md`
- AIH Test Suite v1: `../AIH_TEST_SUITE_v1_20260712.md`

## HybridAI Relationship

AI Chess Match can live as a HybridAI subproject and smoke test while also
serving as an AIH validation test.

```text
HybridAI vn        = agent implementation
HybridAI vn-chess  = chess-adapted version/subproject
AI Chess Match     = validation harness
```
