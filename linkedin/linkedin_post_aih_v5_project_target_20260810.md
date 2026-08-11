AIH v5 target is now simple:

Build a local-first agentic AI benchmark that can run on a normal laptop, register only the agents that actually respond through the local stack, run a compact chess-harness tournament, and publish an aggregate HTML report.

This version is not about claiming chess strength. It is about measuring whether local AI agents can stay inside a small rules-bound task without drifting, timing out, or failing the harness.

Project goals:
https://github.com/gray3s/brilliance/blob/main/aih/aichess/v5/published_results/AIH_V5_PROJECT_GOALS_20260811.md

Project implementation plan:
https://github.com/gray3s/brilliance/blob/main/aih/aichess/v5/published_results/AIH_V5_IMPLEMENTATION_PLAN_20260811.md

Current aggregate HTML summary:
https://htmlpreview.github.io/?https://github.com/gray3s/brilliance/blob/main/aih/aichess/v5/published_results/AIH_V5_REGISTRATION_AGGREGATE_LATEST.html

The current plan:

- run AIH v5 locally across repeated trials
- aggregate the registration CSVs from `v5/data`
- publish the current real multi-run summary
- keep the benchmark open-source and locally reproducible

The important constraint is local-first: it should work on the machine in front of you. Better hardware should make it faster, and maybe better, but the baseline should not require better hardware to exist.
