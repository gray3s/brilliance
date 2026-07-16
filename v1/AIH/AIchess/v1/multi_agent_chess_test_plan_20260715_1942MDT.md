# Multi-Agent AI Chess Class 1 Test Plan

Created: 20260715_1942MDT

## Purpose

Define the starter direction for AIChess board-based player-agent and
referee-team tests.

These remain Class 1 tests because the target is rule-bound state/game-action hallucination: legal moves, board-state fidelity, timing, referee agreement, and action validation in a bounded chess environment.

## Core Idea

Use multiple agents and boards in the same chess test so the harness can measure not only one agent's move quality, but also:

- disagreement handling,
- referee agreement,
- resource scaling as agent count increases,
- mixed-agent behavior when different models/stacks participate in the same test.

## Agent Roles

Current role vocabulary:

```text
board_N_white_agent_1
board_N_black_agent_1
board_N_referee_N
```

Role meanings:

- `board_N_white_agent_1`: the AI agent assigned to play White on board N.
- `board_N_black_agent_1`: the AI agent assigned to play Black on board N.
- `board_N_referee_N`: a referee assigned to board N to check legality, board state, and rule compliance.

In this vocabulary, a `player` is the chess side/role on a board. A `player
agent` is the AI agent instance assigned to that side.

## Starter Configurations

### Class 1 Team Test

```text
boards = 1
board 1 white side = 1 player agent
board 1 black side = 1 player agent
board 1 referee team = 3 referees
referee_decision_rule = majority_rule, 2 of 3
```

Use this as the baseline team form. It tests legal move production, state
fidelity, timing, and stronger referee validation on one board.

### Class 2 Team Test

```text
boards = 2
each board white side = 1 player agent
each board black side = 1 player agent
each board referee team = 1 referee
```

Use this when the target failure is workflow/provenance tracking around which
board produced each event, which side agent proposed each move, which referee
verified each board, and whether the result artifact accurately preserves that
event record.

### Class 3 Team Test

```text
boards = 4
each board white side = 1 player agent
each board black side = 1 player agent
total referees = 4
referee assignment = 1 referee per board
```

Use this when the target failure is source-bound chess knowledge from a defined
academic or instructional source packet.

## Mixed-Agent Class 1 Tests

The same Class 1 chess test may mix different agents or stacks.

Examples:

```text
board_1_white_agent_1 = local_qwen_stack
board_1_black_agent_1 = local_qwen_stack
board_1_referee_1 = deterministic_engine_or_rule_harness
board_2_white_agent_1 = local_or_cloud_stack
board_2_black_agent_1 = local_or_cloud_stack
board_2_referee_1 = agentic_referee_stack
```

Mixed-agent tests are useful because required local resources should scale with
board count and role. A small local model may be enough for one board while a
stronger local or cloud model can be reserved for another board or referee
slot.

## AIChess Class 2 And Class 3 Role Scaling

AIChess-derived Class 2 and Class 3 tests use larger board sets.

Working convention:

```text
Class 1 AIchess core = one board, one player agent per side, three referees
Class 2 AIchess core = two boards, one player agent per side per board, one referee per board
Class 3 AIchess core = four boards, one player agent per side per board, four referees total
```

Referee convention:

```text
Class 1 AIchess referees = three referees on one board, 2-of-3 majority
Class 2 AIchess referees = one referee per board, two referees total
Class 3 AIchess referees = one referee per board, four referees total
```

Near-term scope: use board assignments, player agents, and referee teams only.
Other role types are deferred until the board-runner package is stable.

Class 2 AIchess examples:

```text
board_1 = white_agent_1 + black_agent_1 + referee_1
board_2 = white_agent_1 + black_agent_1 + referee_1
```

Class 3 AIchess examples:

```text
board_1 = white_agent_1 + black_agent_1 + referee_1
board_2 = white_agent_1 + black_agent_1 + referee_1
board_3 = white_agent_1 + black_agent_1 + referee_1
board_4 = white_agent_1 + black_agent_1 + referee_1
```

These class labels should still be used carefully. The class assignment should record what failure mode is being tested: Class 2 for cooperation/provenance/workflow-state failures around multi-agent side handling; Class 3 for source-bound chess knowledge, instructional material, or academic/course-style chess analysis.

## Measurements

In addition to existing AIChess measurements, multi-agent tests should record:

- board role map,
- model/stack used for each role,
- candidate moves from each board's active-side player agent,
- final selected move by board,
- referee agreement/disagreement,
- illegal recommendation rate,
- illegal final move rate,
- board-state disagreement rate,
- latency by role,
- total local resource use by role if available,
- failure recovery path.

## Guardrails

- The official board state must remain owned by the harness or deterministic referee.
- Agent referees may provide useful comparison data, but they should not be the only source of legality unless the test is explicitly measuring agent-referee failure.
- Each player-agent move must be logged separately by board.
- Mixed-agent configurations must identify each model/stack by board slot.
- The test should distinguish chess weakness from hallucination: a bad legal move is not the same failure as an illegal move, false board state, invented rule, or false claim about verification.

## Next Implementation Direction

Class 1, Class 2, and Class 3 now have C++/Bash fixture runners. The next
implementation step is to replace fixture responses with local model or Qt UI
integration while preserving the same result shape.

Harness language note: the preferred local baseline is C++/Qt/Bash. This is an
implementation-control choice, not a claim that other harness languages are
invalid.
