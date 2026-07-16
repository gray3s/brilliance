# Multi-Agent Chess Result Schema v1

Created: 20260715_1943MDT

## Purpose

Define the expected result fields for fixture and later real-agent multi-agent AIChess benchmark runs.

## Required Top-Level Fields

```text
benchmark_id
run_id
created
test_class
test_family
configuration_id
agent_count
local_or_cloud_or_mixed
role_map
board_count
position
candidate_moves
final_move
referee_votes
metrics
errors
notes
```

## Role Map

```json
{
  "board_1_white_agent_1": "stack_id",
  "board_1_black_agent_1": "stack_id",
  "board_1_referee_1": "stack_id",
  "board_1_referee_2": "stack_id"
}
```

Only roles present in the configuration are required.

## Candidate Moves

Candidate moves preserve each board's active-side player-agent move separately
from the final selected move on that board:

```json
[
  {
    "board_id": "board_1",
    "role": "board_1_white_agent_1",
    "stack_id": "fixture_local_player",
    "raw_response": "Recommend e2e4",
    "parsed_uci": "e2e4",
    "legal": true,
    "latency_ms": 0.0
  }
]
```

## Referee Votes

```json
[
  {
    "role": "referee_1",
    "stack_id": "deterministic_referee",
    "move": "e2e4",
    "legal": true,
    "reason": "move_in_fixed_start_legal_set"
  }
]
```

## Metrics

```text
legal_final_move
legal_recommendation_rate
referee_agreement
unparseable_output_count
role_latency_ms
configuration_latency_ms
selected_move_by_board
```

## Scoring Rule

The fixture benchmark is not a chess-strength score. It scores whether the benchmark package can preserve role-separated evidence and detect Class 1 failures:

- illegal final move,
- illegal recommendation,
- unparseable output,
- referee disagreement,
- false state or rule claim,
- missing role evidence.
