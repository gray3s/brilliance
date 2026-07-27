# AIH v2 Agent Compatibility Notes

Timestamp: 2026-07-26 2130 MT

AIH v2 is currently focused on agent compatibility: can each agent under
consideration pass a fair basic response floor and a fair AIChess admission
floor before deeper comparisons are treated as meaningful?

This page links the current agent list and test notes for the July 26, 2026
AIH v2 update.

## Agent List

Current count:

```text
16 local agents
9 cloud agents
```

- [AIH v2 agent list](https://github.com/gray3s/brilliance/blob/main/aih/aichess/v2/misc/aih_v2_agent_list_20260726.md)

The agent list separates local and cloud agents and records the current adapter,
execution locus, model identity, size information where available, and latest
compatibility evidence.

## Test Notes

- [AIH v2 test documentation](https://github.com/gray3s/brilliance/blob/main/aih/aichess/v2/misc/aih_v2_test_documentation_20260726.md)

The test notes separate admission floors from diagnostic ladders:

```text
preschool_liveness
aichess clue level 6
K-level and diagnostic tests
chess admission diagnostics
```

## Current Admission Surfaces

The current preschool floor is intentionally small. If a responsive active
agent cannot pass a candidate preschool test, that test is promoted to K-level
or diagnostics.

The current AIChess admission floor is clue level 6. It asks the agent for a
legal move in a natural way and accepts the floor if the answer contains at
least one legal move that the runner can validate.

## Qualification Rule

Do not reject an agent from a level based on one failed qualification run.

One pass establishes possible capability. Mixed pass/fail behavior is
reliability evidence. Repeated no-pass after fair attempts is evidence of
non-qualification for that level.

Adapter, transport, timeout, credential, and prompt-design failures are tracked
separately from model capability failures.
