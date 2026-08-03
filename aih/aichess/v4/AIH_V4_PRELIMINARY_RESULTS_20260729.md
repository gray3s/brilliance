# AIH v4 preliminary results - 2026-07-29

These are preliminary release-mode data points from the current v4 prototype.
They are not final AIH rankings. The local default maxply has been raised
and the local/cloud maxply multiplier range is 2x to 4x.

Rendered HTML results:
https://htmlpreview.github.io/?https://github.com/gray3s/brilliance/blob/main/aih/aichess/v4/AIH_V4_PRELIMINARY_RESULTS_20260729.html

## Current default run controls

- Local retry/expand/default local maxply: 50
- Cloud provider-key default maxply: 10, derived from local maxply / ratio
- Local maxply cap: 50
- Cloud maxply cap: 10
- Default local/cloud maxply multiplier: 4x
- Allowed local/cloud maxply multiplier range: 2x to 4x
- CLI controls: `--local-maxplys=N`, `--local-cloud-maxply-ratio=N`

## Latest binary-published summary

Source summary: `runs/aih_v4_pairwise_prototype_20260729/aichess_v4_pairwise_prototype_20260729_20260803_164352_summary.md`

| Model | Mode | Termination | Completed game | Plies | Legal moves | Failed turns | Rejected attempts | Elapsed s |
| --- | --- | --- | --- | ---: | ---: | ---: | ---: | ---: |
| gemini:gemini-3.5-flash-lite vs granite4:3b | aichess_hallucination_game | black_forfeit_invalid_or_unparseable_move | false | 1 | 1 | 1 | 1 | 25.522 |

## Preliminary interpretation

The latest preliminary row is generated and pushed by successful
`bin/aih_v4` runs from the newest v4 run summary.
