# Local Hardware Vs Cloud Software Benchmark Package

Created: 20260715_1943MDT

## Purpose

Define a useful benchmark package for comparing local hardware/agent stacks against cloud software/agent stacks using AIChess Class 1 tests.

The package should measure bounded behavior, not just chess strength:

- legal action,
- board-state fidelity,
- timing,
- recovery from invalid output,
- reproducibility,
- local resource cost,
- cloud/API cost,
- deployment and maintenance burden.

## Package Contents

Minimum benchmark package:

```text
1. fixed chess positions / opening states
2. fixed time controls
3. fixed prompt templates
4. fixed output schema
5. deterministic referee/engine validation
6. local stack manifest
7. cloud stack manifest
8. run logs
9. result JSON
10. summarized scoring table
```

## Hardware/Software Manifests

Local manifest fields:

```text
cpu
gpu
ram
storage
os
model
model_quantization
runtime_backend
context_window
thread_count
gpu_layers_or_acceleration
power_profile_if_known
```

Cloud manifest fields:

```text
provider
model
api_or_agent_surface
region_if_known
context_window
temperature_or_sampling
tool_access
rate_limit_notes
cost_basis
```

## Core Metrics

Behavior metrics:

- legal move rate,
- illegal move rate,
- unparseable output rate,
- board-state hallucination rate,
- rule hallucination rate,
- referee disagreement rate,
- recovery success after invalid output,
- completion rate,
- resignation/checkmate/stalemate/fault ending type.

Performance metrics:

- first-token latency if available,
- move latency,
- total game time,
- timeout count,
- retries required,
- throughput under repeated runs.

Cost/resource metrics:

- local CPU/GPU/RAM use if available,
- local wall-clock time,
- local energy estimate if available,
- cloud tokens or billed units,
- cloud dollar cost estimate,
- setup and maintenance notes.

## Benchmark Configurations

Recommended starter configurations:

```text
class1_one_board_one_agent_sides_three_referees_local_vs_cloud
class2_two_boards_one_agent_sides_one_referee_each_local_vs_cloud
class3_four_boards_one_agent_sides_four_referees_local_vs_cloud
```

Each configuration should be runnable with:

- all-local agents,
- all-cloud agents,
- mixed local/cloud agents,
- mixed local models on the same hardware when available.

## Mixed-Agent Comparison

Mixed-agent tests should identify role assignment:

```text
board_1_white_agent_1 = <stack>
board_1_black_agent_1 = <stack>
board_1_referee_1 = <stack or deterministic engine>
board_2_white_agent_1 = <stack, if used by class>
board_2_black_agent_1 = <stack, if used by class>
board_2_referee_1 = <stack or deterministic engine, if used by class>
```

This is useful because local resources should scale with board count and role.
Small local agents may be enough for one board. Stronger local or cloud agents
can be reserved for other boards or referee slots.

## Scoring Table Fields

Result summaries should include:

```text
run_id
configuration
local_or_cloud_or_mixed
role_map
board_count
model_stack_by_role
position_set
time_control
legal_move_rate
illegal_move_rate
unparseable_rate
state_hallucination_rate
median_move_latency
timeout_count
game_completion_rate
cost_estimate
notes
```

## Useful Benchmark Claim

A useful local-hardware vs cloud-software benchmark should be able to say:

```text
For this bounded Class 1 chess task, this local stack produced X legal move rate,
Y median latency, Z completion rate, and approximate local resource cost, compared
with the cloud stack under the same prompts, positions, timing, and referee rules.
```

It should not claim broad general intelligence or overall model superiority from chess results alone.
