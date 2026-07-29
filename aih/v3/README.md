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
agent=c06-hi sel=06,6 loglvl=6 maxplys=4
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
rows. If there are 20 rows and `stride=4`, the first log level tests rows:

```text
1, 5, 9, 13, 17
```

The next log level starts at row 2:

```text
2, 6, 10, 14, 18
```

Then:

```text
3, 7, 11, 15, 19
4, 8, 12, 16, 20
```

The selected rows are then expanded across the configured clue levels for that
log level.

This cuts the run size by roughly the stride factor while still spreading
coverage across the full sorted agent set. A `stride=4` run is about one
quarter of the full agent/log/clue matrix. Full validation is `stride=1`.

Uneven row counts are allowed. The final rows simply appear in the offset
bucket that matches their row index.

The selector is reproducible. A failed cell can be retested with the same
agent row, stride, log level, clue level, and maxply cap.

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
