# AIH v4

AIH v4 is the current AIChess tournament/evaluation line.

## Current Run

Gemini cloud-rep tournament:

```bash
bin/aih_v4 --cloud-rep-gemini
```

Latest completed run:

- curDateTime: `2026-08-03T19:26:09`
- HTML: `data/aichess_v4_pairwise_prototype_20260729_20260803_192609.html`
- JSONL: `data/aichess_v4_pairwise_prototype_20260729_20260803_192609.jsonl`
- Summary: `data/aichess_v4_pairwise_prototype_20260729_20260803_192609_summary.md`

## Current Result Codes

- `ahg`: AIChess hallucination game
- `fail.stp`: agent stopped playing chess
- `b.fto`: black forfeit by move timeout
- `b.fft/agt/nrsp`: black forfeit; agent/model no response while stack is alive
- `b.fft/stk/nrsp`: black forfeit; stack no response
- `qta`: cloud quota or rate-limit throttle
- `fir`: irrelevant agent return forfeit
- `fim`: invalid move forfeit

## Latest Result

The `20260803_192609` run is evidence, not a valid ranking. It exposed report
and run-control issues: top displayed rows can have `AgntOH% = 100.000`,
`AIH% = 0`, `Legal% = 0`, or `HrnOH% = 0`, and the report must not present
those rows as top-ranked agents.

The summary table uses compact row values for readability; the JSONL keeps the
full fields.

## Current AIH v4 Issues

- AIH v4 development is currently hapered severely by Codex hallucination.
  Further research into this will be a prerequesite for AIH v5.
- AIH v4 is for hallucination statistics during bounded chess activity, not
  chess-strength ranking.
- Incomplete games can still provide valid hallucination evidence; `Cmplt = no`
  must be visible context, not an automatic reason to discard statistics.
- Infrastructure-invalidated rows such as quota, transport, stale-load,
  corrupt-load, or failed-preflight rows must not be ranked as valid agent
  results.
- Suspect rows must not rank at or near the top. If a row is invalid or
  suspected invalid, its rank should be `n/a`.
- The HTML aggregate table should use:
  `Rank | AIH% | Legal% | AgntOH% | HrnOH% | L/C | Agent Title`.
- `HrnOH%` means harness output hallucination. The former `OH%` label is
  ambiguous and should not be used.
- `L/C` should contain uppercase `L` or `C`; the agent title should not be
  prefixed with `l` or `c`.
- The binary still needs a coherent `--run-mode parallel|serial` flag. For now,
  `parallel` may remain the default, but the selected run mode must be recorded
  in JSONL, Markdown, HTML, and table descriptions.
- A restarted benchmark should unload resident local agents/models, treat old
  loads as possibly stale or corrupt, verify no prior AIH v4 process is active,
  clear questionable generated artifacts, and initialize a fresh run mode.
- Before official tournament start and before each board match, each player
  should pass a one- or two-message stack health exchange.

## Controls

- Default local run: `bin/aih_v4`
- Gemini cloud-rep run: `bin/aih_v4 --cloud-rep-gemini`
- Current cloud move timeout: `10` seconds
- Current cloud stack timeout: `10` seconds
- Current cloud response attempts: `3`
- Current cloud invalid/relevance threshold: `3`

## Artifacts

- Timestamped data: `data/`
- Raw run output: `runs/aih_v4_pairwise_prototype_20260729/`
- Latest HTML copy: `AIH_V4_PRELIMINARY_RESULTS_20260729.html`
- PDP: `AIH_V4_PROJECT_DEVELOPMENT_PLAN_20260803.md`
- PG: `AIH_V4_PROJECT_GOALS_20260729.md`
