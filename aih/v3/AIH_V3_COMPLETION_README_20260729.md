# AIH v3 completion checkpoint - 2026-07-29

AIH v3 is complete for its intended scope.

The v3 runner is not a relative agent-ranking system. It is a configured-ply
AIChess harness used to test whether AI agents continue playing chess against
themselves for more plies as the configured `maxply` limit increases, and to
make the stopping/failure modes visible.

Relative chess performance and AIH ranking are AIH v4 goals.

## Final v3 data set

The current static latest manifest is:

```text
aih/aichess/v3/static/latest/manifest.txt
```

The final merged summary is:

```text
aih/aichess/v3/static/latest/aichess_run_summary_latest.csv
```

The manifest records:

```text
timestamp=20260729170252
merge_mode=partial_missing-key_into_existing_all
source_run_dir=aih/aichess/v3/runs/daily_aichesskv3_binsucc_20260729170252
```

The final merged table contains:

- 518 AIChess summary rows
- 42 distinct tested agent lanes
- 12 OpenAI cloud lanes
- 3 Google cloud lanes
- 12 Anthropic cloud lanes
- 14 Ollama local lanes
- 1 Codex local lane

## Cloud failure repair

The final v3 data includes a targeted repair pass for rows that had failed
because the required cloud key was unavailable.

Repair coverage:

- expected missing-key cells from the prior all-agent summary: 147
- repaired cells produced by the patch run: 147
- missing expected repair keys: 0
- extra unexpected repair keys: 0
- missing-key cells skipped by stride: 0
- AIChess test executions in the repair pass: 147

The repair mode replaced only the prior failed cloud-run rows and preserved the
existing data that was not regenerated in the repair pass.

## Overall result

The v3 result supports the limited v3 conclusion:

AIH v3 can show continuation behavior as configured `maxply` increases.

The strongest continuation signal is the maximum observed ply count. The final
merged summary reached a maximum observed ply count of 94 at the 256 maxply
level.

High-level `maxply` trend from the final merged summary:

| maxply | rows | pass | average plies | max observed plies |
| ---: | ---: | ---: | ---: | ---: |
| 4 | 11 | 7 | 3.273 | 4 |
| 8 | 22 | 7 | 2.682 | 8 |
| 16 | 32 | 5 | 2.719 | 16 |
| 32 | 42 | 2 | 2.357 | 32 |
| 64 | 53 | 2 | 4.792 | 64 |
| 128 | 64 | 0 | 4.250 | 89 |
| 256 | 74 | 0 | 3.108 | 94 |

The larger-cap rows expose more failure behavior rather than producing clean
completed long games. That is useful for v3 because the purpose was harness
validation and continuation behavior, not final model ranking.

## Failure modes

Final merged termination counts:

| result | termination | rows |
| --- | --- | ---: |
| fail | white_forfeit_invalid_or_unparseable_move | 424 |
| fail | white_forfeit_transport_failure | 44 |
| pass | draw_by_configured_ply_limit | 23 |
| fail | game_timeout | 15 |
| fail | black_forfeit_invalid_or_unparseable_move | 6 |
| fail | black_forfeit_transport_failure | 3 |
| fail | black_forfeit_move_timeout | 2 |
| fail | white_forfeit_invalid_or_unparseable_move+token_limit_detected | 1 |

These failure modes should not be collapsed into a single "bad agent" result.
V4 must separate AIH behavior from transport, stack, timeout, token-limit, and
possible prompt errors.

## Caveats

The final v3 data is a merged table, not one single uninterrupted full rerun.
The cloud-key repair pass introduces statistical distortion because those cells
were regenerated later under a corrected environment.

The only way to remove that artifact would be a full v3 rerun under one
consistent environment.

For the v3 goal, this is acceptable because the objective is not a precise
ranking. The objective is to verify agent coverage and determine whether the
harness can expose continuation behavior as `maxply` increases.

## V4 handoff

AIH v4 should use a different design for relative performance and AIH ranking.

The v4 goals are:

- pair agents and have them play complete games
- record per-run gameplay records
- retry the exact same prompt up to three times when a completed response is
  invalid
- concede only after the configured response-error attempts are exhausted
- separate AIH behavior from transport, stack, timeout, token-limit, adapter,
  and possible prompt errors
- record turn timing from first prompt through referee determination
- record move count and color played in per-turn data during early v4
- add an explicit response terminator marker
- replace repeated dot-progress I/O logging with one pending line and one
  terminal complete/error line
- build an error-config database with unique error configs and instance counts
- generate cross-correlations across agent, prompt, and stack configuration

V3 established enough harness behavior to move into v4 with some assurance.
V4 is where relative AIH ranking starts.
