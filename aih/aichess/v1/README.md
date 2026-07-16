# AI Chess Match AIH Test

Created: 2026-07-12

## Purpose

This is one of the three distinct AIH test families.

AI Chess Match tests agentic AI state fidelity, legal action, timing,
rule-following, and hallucination behavior in a bounded chess environment.

## Position In AIH v1

```text
AIH test family 1: personal-history agentic-AI failure tests
AIH test family 2: K-PhD knowledge-ladder tests
AIH test family 3: AI Chess Match state-fidelity tests
```

## Core Idea

A referee owns the official board state. Agents observe the board and propose
moves. The referee validates moves, applies legal moves, records time, and
classifies game endings.

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
