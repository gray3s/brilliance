# AI Chess Match AIH Test Family

Created: 2026-07-12

Canonical path:

```text
v1/AIH/AIchess/
```

Versioned test definitions live under:

```text
v1/AIH/AIchess/v1/
v1/AIH/AIchess/v2/
...
v1/AIH/AIchess/vn/
```

New chess/state-fidelity tests can be added as the project identifies them.

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

AIChess should also support 2-, 3-, 4-, and 5-agent Class 1 tests. These can
include separate planning agents, spare agents that make alternative move
recommendations, multiple referees, and mixed-agent stacks inside the same
Class 1 test.

Starter plan:

```text
v1/multi_agent_chess_test_plan_20260715_1942MDT.md
```

Local-hardware vs cloud-software benchmark package:

```text
v1/local_vs_cloud_benchmark_package_20260715_1943MDT.md
```

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
- spare-agent recommendation quality,
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

## AIH v4 Preliminary Results

- Summary: `v4/AIH_V4_PRELIMINARY_RESULTS_20260729.md`
- Rendered HTML: https://htmlpreview.github.io/?https://github.com/gray3s/brilliance/blob/main/aih/aichess/v4/AIH_V4_PRELIMINARY_RESULTS_20260729.html
- Binary publish path: successful `v4/bin/aih_v4` runs

## AIH v4 Versions

- `v4/20260803-local-default-50ply` (latest)
  - Binary: `v4/bin/aih_v4`
  - Latest timestamped archive: `aih-v4_20260803_1640MDT.zip`
  - Archive timestamp: 2026-08-03 16:40 MDT
  - Normal default: local-only `local-retry`, 50 max plies, harness referee
  - Tournament policy: no same-agent same-mode self-play by default
  - Cloud representative: explicit Gemini-vs-local mode via `--cloud-representative-gemini`
  - 100-ply evaluation: explicit opt-in via `--local-retry-smoke --local-maxplys=100`
  - Run note: `v4/RUN_AIH_V4_BINARY_LOCAL_DEFAULT_20260803.md`
  - Candidate plan: `v4/AIH_V4_LOCAL_MODEL_CANDIDATE_PLAN_DEEPSEEK_VS_KIMI_20260803.md`
