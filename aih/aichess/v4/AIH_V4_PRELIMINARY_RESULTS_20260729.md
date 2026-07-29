# AIH v4 preliminary results - 2026-07-29

These are preliminary release-mode data points from the current v4 prototype.
They are not final AIH rankings. The local run used a higher maxply limit to
exercise retry/concede behavior without cloud cost. The cloud runs kept maxply
low and broadened provider reasoning levels before increasing cloud depth.

Rendered HTML results:
https://htmlpreview.github.io/?https://github.com/gray3s/brilliance/blob/main/aih/aichess/v4/AIH_V4_PRELIMINARY_RESULTS_20260729.html

## Preliminary local/cloud results - local maxply 8, cloud maxply 2, local/cloud maxply multiplier 4x

In the model-pairing column, the first line is the white player and the second
line is the black player. Color-swapped runs should be shown as a separate
row with those two lines reversed.

| Stack class | Provider/stack | Model pairing | Verbosity | Maxply | Combined reasoning results |
| --- | --- | --- | --- | ---: | --- |
| local | Ollama | `granite4:3b`<br>`qwen2.5-coder:3b` | md | 8 | `md`: plies 4, legal 4, failed turns 1, rejected 3, elapsed 115.035s, termination `white_forfeit_invalid_or_unparseable_move` |
| cloud | OpenAI | `openai:gpt-4.1-mini`<br>`openai:gpt-4.1-mini` | md | 2 | `lo`: plies 2, legal 2, failed turns 0, rejected 0, elapsed 4.532s \| `md`: plies 2, legal 2, failed turns 0, rejected 0, elapsed 3.370s \| `hi`: plies 2, legal 2, failed turns 0, rejected 0, elapsed 3.099s |
| cloud | Anthropic | `anthropic:claude-3-5-haiku`<br>`anthropic:claude-3-5-haiku` | md | 2 | `lo`: plies 2, legal 2, failed turns 0, rejected 0, elapsed 5.457s \| `md`: plies 2, legal 2, failed turns 0, rejected 0, elapsed 5.719s \| `hi`: plies 2, legal 2, failed turns 0, rejected 0, elapsed 5.277s |
| cloud | Google/Gemini | `gemini:gemini-3.1-flash-lite`<br>`gemini:gemini-3.1-flash-lite` | md | 2 | `lo`: plies 2, legal 2, failed turns 0, rejected 0, elapsed 2.556s \| `md`: plies 2, legal 2, failed turns 0, rejected 0, elapsed 2.353s \| `hi`: plies 2, legal 2, failed turns 0, rejected 0, elapsed 3.152s |

## Preliminary interpretation

The cloud provider-key paths all produced legal two-ply games across low,
medium, and high reasoning at medium verbosity. No missing-key, entitlement,
transport, parser, or referee failures were observed in this cloud slice.

The local baseline reached four legal plies under the higher local maxply
limit, then produced a clean retry/concede data point: the white local agent
repeated an illegal move through the retry budget, the harness classified it
as an illegal move hallucination rather than a transport failure, and the game
ended by configured forfeit.
