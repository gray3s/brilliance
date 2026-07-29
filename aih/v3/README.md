# AIH v3

AIH v3 is the current AIChess-based all-agent eval runner.

The v3 runner compares qualified cloud and local AI agent slots through the
AIChess binary harness, records configured maxply results, and publishes static
CSV summaries for later inspection.

AIH v3 uses the preschool validation test as its validation seed, with no
separate promotion ladder.

## Current Runner

The active v3 entry point is:

```bash
cd /home/sag/RPA2/myLLC/AI/brilliance/aih/aichess/v3
./bin/aih_all_v3
```

With no flags, the binary reads `static/latest` and does not execute AIChess.

To regenerate test data and run the full matrix:

```bash
./bin/aih_all_v3 --regenerate-test-data
```

## Agent Labels

Cloud agent labels include source and thinking mode:

- `c06-hi`
- `c06-xh`
- `c06-md`

Local agent labels use `lNN`, for example:

- `l11`
- `l0A`

Each test line prints the exact child selector, for example:

```text
agent=c06-hi cluelvl=6 loglvl=6 maxplys=4
```

## Work Cap

Maxply is a per-cell work cap, not an expected game length.

For each AIChess log level and clue level:

```text
maxplys = 2 ^ (14 - (cluelvl + loglvl))
```

In this context, `cluelvl` is an AIChess clue/hint setting. It is not a school
grade level.

Examples:

- `loglvl=6, cluelvl=6` -> `maxplys=4`
- `loglvl=6, cluelvl=0` -> `maxplys=256`
- `loglvl=0, cluelvl=6` -> `maxplys=256`
- `loglvl=0, cluelvl=0` -> `maxplys=16384`

## Stride-Selected Eval

Cloud token burn is part of the v3 design problem, so AIH v3 does not simply
brute-force every cloud model and thinking mode through every matrix cell.

The runner uses a stride selector across the stable sorted agent/thinking-mode
rows. In one-based slice notation, each log level tests:

```text
start:stride:row(N)
```

If there are 20 agent/thinking-mode rows and `stride=4`, the first log level
tests:

```text
1:4:20  ->  1, 5, 9, 13, 17
```

The next log levels advance the starting row:

```text
2:4:20  ->  2, 6, 10, 14, 18
3:4:20  ->  3, 7, 11, 15, 19
4:4:20  ->  4, 8, 12, 16, 20
```

Then the selector wraps and starts at row `1` again.

The selected rows are then expanded across the configured clue levels for that
log level.

The current default is `stride=4`.

This cuts the run size by roughly the stride factor while still spreading
coverage across the full sorted agent set. A `stride=4` run is about one
quarter of the full agent/log/clue matrix. Full validation is `stride=1`.

Uneven row counts are allowed. The final rows simply appear in the offset
bucket that matches their row index.

The selector is reproducible. A failed cell can be retested with the same
agent row, stride, log level, clue level, and maxply cap.

## Cloud Failure Repair

AIH v3 now includes a targeted repair mode for failed cloud-agent test data.

When invoked with:

```bash
./bin/aih_all_v3 --regenerate-test-data --agent-set missing-key
```

the runner inspects the latest prior all-agent `aichess_run_summary.csv`,
detects rows whose termination was recorded as
`missing_artifact_or_command_failed_*`, reruns those exact failed cells, and
merges the replacement rows back into the existing all-agent summary.

The merge is row-specific. It replaces only the prior failed cloud-run rows and
preserves the existing successful OpenAI, Google, Anthropic, Codex, and local
agent data that was not regenerated in the repair run.

The repair path still honors stride, but scales it against the smaller subset
of failed agent rows so a partial repair run does not accidentally skip most of
the failed cloud cells.

Reduced-cadence agents are not deleted from the run. They are sampled at the
stride cadence unless a full-validation run is requested.

After each log-level sweep, the runner can still rank pruning candidates using
percent of maxply used, not raw ply count. That keeps agent ranking comparable
when different cells have different maxply caps.

For cloud agents, reduced cadence skips odd log levels and still runs even log
levels, including `loglvl=0`. This keeps cloud capability represented in the
final low-log-level pass.

The default policy is:

- `drop_policy=lowest-costliest`
- `drop_scope=cloud-first`
- `cloud_normal_floor=9`

The cost side prefers the most thought-intensive cloud modes first instead of
blindly penalizing the highest-performing cloud agents.

Local agents are favored in mixed cloud/local runs because they do not burn
cloud-token budget.

## Outputs

Regenerated runs write:

- `qualification_summary.csv`
- `aichess_run_summary.csv`
- `aichess_derived_maxply_summary.csv`
- `elimination_summary.csv`
- `aichessav3_binsucc_*.csv`

The latest static outputs are published under:

```text
aih/aichess/v3/static/latest
```

## Completion Checkpoint

The v3 completion checkpoint is recorded in:

```text
aih/v3/AIH_V3_COMPLETION_README_20260729.md
```

That checkpoint summarizes the final merged v3 data set, the targeted
missing-key repair pass, the supported v3 conclusion, and the handoff boundary
for AIH v4.
