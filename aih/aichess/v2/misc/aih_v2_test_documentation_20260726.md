# AIH v2 Test Documentation

Timestamp: 2026-07-26 2130 MT

Agent list:

- [AIH v2 agent list](https://github.com/gray3s/brilliance/blob/main/aih/aichess/v2/misc/aih_v2_agent_list_20260726.md)

## Test Classes

AIH v2 separates tests by purpose.

## Preschool Floor

Preschool is the all-pass floor for responsive active agents. If a responsive
agent cannot pass a preschool content test, promote that test to K-level or
diagnostics.

Current preschool probes:

```text
adapter_liveness_sum_0_plus_1
adapter_liveness_name_letter
```

Promoted out of preschool for now:

```text
copy-token
color naming
broader arithmetic
division
floating point
open-vocabulary facts
```

## K-Level And Diagnostics

These tests can reveal differences between agents, but do not block the
preschool floor:

```text
copy/return token
closed-list colors
0+0 candidate arithmetic
general addition
integer division
floating point arithmetic
same/different
choose A/B
reading comprehension
rule comprehension
```

## Chess Admission Diagnostics

These should run before rejecting an agent from AIChess:

```text
piece counts: king/queen singles, rook/knight/bishop pairs, pawns eight per side
piece movement: rook, bishop, queen, king, knight, pawn
side-to-move recognition
legal starting move recognition
illegal starting move recognition
coordinate move formatting
```

## AIChess Clue Ladder

```text
6  natural legal-move floor; accept any legal UCI move evidence in the answer
5  exact bf plus exact expected af
4  suggested legal move plus exact bf
3  legal move list plus board-valid clue
2  board-valid clue only
1  legal move list only
0  no clue
```

AIChess clue level 6 is the compatibility admission floor. It is not a strategy
benchmark. It asks whether the agent can provide at least one legal move under
maximal fair scaffolding.

## Grading Principle

Ask a natural task, extract all answer evidence, and grade the strongest valid
answer for the admission floor. Preserve defects separately:

```text
first candidate legal or illegal
number of legal candidates
number of illegal candidates
mixed legal/illegal response
format compliance
board-transition correctness
side confusion
timeout/no-response
```

## Qualification Rule

Do not reject an agent from a level based on one failed qualification run.

```text
test-level hallucination: sometimes passes, sometimes fails
concept-level hallucination: never passes after repeated fair attempts
```

Only concept-level non-qualification should block an agent from that level,
unless the failure is actually adapter, transport, timeout, key, or prompt
design.
