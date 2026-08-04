# AIH v4 Project-Development Issue - Invalid Ranking and Run Mode

Date: 2026-08-03

## Problem

The AIH v4 HTML report can present invalid or questionable rows as top-ranked
agents. The `20260803_192609` report showed top-ranked rows with invalid-row
signals, including `AIH% = 0`, `Legal% = 0`, `AgntOH% = 100.000`, and
`HrnOH% = 0`.

The report must not rank suspect rows at or near the top. If a row is judged
invalid, its rank should be `n/a`.

## Invalid-Row Reason Ranking

Rank these invalidation reasons from strongest to weakest:

1. Source result is `fail.qta` or quota/transport invalidated.
2. Source result is infrastructure-invalidated before useful agent evidence can
   be collected.
3. Source result is `fail.stp`.
4. `AgntOH% = 100.000`.
5. `AIH% = 0` and `Legal% = 0`.
6. `AIH% = 0` and `AgntOH% = 0`.
7. `AIH% = 0`.
8. `HrnOH% = 0`.
9. Row is unscored / `n/a`.

Do not require `Cmplt = yes` before using row statistics. This table is not a
completed-game counter. Incomplete games can still provide valid event
statistics when the failure itself is the measured behavior. Completion status
should be reported and used as context, not as a blanket reason to discard all
statistics.

## HTML Report Requirements

The aggregate ranking table columns should be:

`Rank | AIH% | Legal% | AgntOH% | HrnOH% | L/C | Agent Title`

Rules:

- Use `HrnOH%`, not `OH%`.
- Use uppercase `L` or `C` in the `L/C` column.
- Do not prepend `l` or `c` to `Agent Title`.
- Put `L/C` immediately before `Agent Title`.
- Use lowercase `n/a`, not `N/A`.
- If a row is invalid or suspected invalid, set `Rank` to `n/a`.
- Do not allow invalid or suspected-invalid rows to sort as top ranked rows.

## Run-Mode Requirements

The binary needs a coherent run-mode flag:

- `--run-mode parallel`
- `--run-mode serial`

For now, `parallel` may remain the default, but the selected run mode must be
explicitly recorded in the test table description, summary, JSONL metadata, and
HTML report.

`--boards N` must remain the board assignment count. It must not implicitly
mean "run N local games concurrently" unless the run mode is parallel.

## Restart Preflight Requirements

When a benchmark is restarted, the binary should:

1. Unload all resident local agents/models.
2. Treat resident model loads as possibly stale or corrupt.
3. Verify no prior AIH v4 process is still active.
4. Clear generated artifacts from invalid or questionable runs.
5. Initialize the tournament with a consistent recorded run mode.

## Agent Health Check Requirements

Before the official tournament starts, and before each board match officially
starts, run a one- or two-message exchange with each player on that board.

The pre-match exchange should verify:

- the stack is alive,
- the expected model/agent is responding,
- the response is not stale/corrupt output,
- the latency is within configured timeout.

If either player fails the pre-match health check, that board should not count
as an official match result.
