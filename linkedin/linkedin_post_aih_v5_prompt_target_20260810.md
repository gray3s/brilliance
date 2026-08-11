# LinkedIn Draft: AIH v5 Prompt Target

AIH v5 is the version where the benchmark becomes local-first.

The target is not a published binary. The target is a prompt bundle that can generate the binary, with enough structure that another user can adapt it to their own laptop, local agent stack, API key configuration, and hardware limits.

The key change is registration before competition:

-> discover candidate local agents
-> start or contact only agents supported on the host
-> run a simple query-return smoke test
-> admit only agents that pass
-> run the tournament over the viable roster

The default v5 tournament mode is intentionally small enough to run quickly:

-> four local agents register first
-> the top four placement candidates run a seed round-robin
-> the top four placement candidates then go through a three-rung ladder
-> rung 1 has two 1v1 boards
-> rung 2 ranks the winners
-> rung 3 ranks the first-round losers

The ranking metric is intentionally pragmatic: low AIH percentage plus low net turn time per ply. By default those are equal weight. If the user wants AIH quality to dominate, one flag changes the balance:

```bash
./bin/aih_v5 --AIH_weight=0.75
```

That makes turn-time weight the complement, so there is only one weighting knob to reason about.

Why this matters for local agentic AI:

Local performance is not just model quality. It is runtime startup, agent viability, turn latency, storage behavior, system load, and whether the host can actually support the acceleration path being tested. AIH v5 treats those as measurable conditions instead of assumptions.

Project goals:
https://github.com/gray3s/brilliance/blob/main/aih/aichess/v5/published_results/AIH_V5_PROJECT_GOALS_20260811.md

Project implementation plan:
https://github.com/gray3s/brilliance/blob/main/aih/aichess/v5/published_results/AIH_V5_IMPLEMENTATION_PLAN_20260811.md

Current aggregate HTML report:
https://htmlpreview.github.io/?https://github.com/gray3s/brilliance/blob/main/aih/aichess/v5/published_results/AIH_V5_REGISTRATION_AGGREGATE_LATEST.html

Validation report:
The current aggregate HTML report is generated from the completed AIH v5 registration CSV set in `v5/data`.

This is still an experiment, but it is now an experiment with a repeatable registration phase, tournament rules, ranking output, and an HTML report.
