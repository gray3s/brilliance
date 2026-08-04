# Run AIH v4 Binary - Local Default

Date: 2026-08-03
Project: AIH v4 AIChess
Binary: `/home/sag/RPA2/myLLC/AI/brilliance/aih/aichess/v4/bin/aih_v4`

## Current Default

The AIH v4 binary entry point launches the v4 wrapper script. The default run
is local-only and uses local AI agents discovered from the local qualification
registry or the local Ollama model list.

Current ordinary binary defaults:

- Smoke stage: `local-retry`
- Cloud agents: not used by default
- Max plies: `50`
- Self-play: rejected by default
- Referee: `harness`
- Response attempts: `3`
- Fatal turn errors: `1`
- Output tokens: `1024`
- Log level: `5`
- Clue mode: `6`

## Rationale for 50 Plies

The default local maxply is set to `50` because the planned virtual chess
tournament may disqualify roughly half of the participating agents after their
first game. Those agents may not get another opportunity to demonstrate
performance. A higher first-game default gives each local agent a more
meaningful participation record before possible elimination, while still
keeping the default run local-only and avoiding cloud-token cost.

With this default, one tournament round should already produce useful
comparative evidence. Rounds two and three should improve confidence and show
whether performance is consistent, but they should not be required before the
run has analytical value.

## Gemini Cloud Representative

The ordinary binary run remains local-only. To spend Gemini credits
deliberately and run the Gemini cloud tournament slice, use:

```bash
/home/sag/RPA2/myLLC/AI/brilliance/aih/aichess/v4/bin/aih_v4 --cloud-representative-gemini
```

This mode pairs `gemini:gemini-3.5-flash-lite` against the discovered passing
local roster by default. It derives the board count from the selected local
opponent list, so it is a cloud-vs-local tournament slice, not a one-board
smoke test and not Gemini self-play.

To split a cloud tournament into smaller batches, set
`AIH_V4_CLOUD_REPRESENTATIVE_LOCAL_START` and
`AIH_V4_CLOUD_REPRESENTATIVE_LOCAL_COUNT`.

## Self-Play Policy

Default v4 tournament runs should not pair an agent against itself. The
tournament should be organized around agents playing other agents, because the
main purpose is relative comparison, ranking, and elimination. A player playing
itself is a different kind of test and should not define the tournament.

Same-agent same-mode pairings are rejected by default. A later explicit
mixed-mode runoff among the top three agents may be considered after the main
tournament, but that would be a separate post-tournament analysis phase, not
the ordinary default binary behavior.

## 100-Ply Evaluation

To evaluate whether local matches should be allowed to run to 100 plies, use
an explicit local run:

```bash
/home/sag/RPA2/myLLC/AI/brilliance/aih/aichess/v4/bin/aih_v4 --local-retry-smoke --local-maxplys=100
```

The purpose is to measure how often games can reach or justify 100 plies
before an earlier terminal condition. The default remains 50 plies until the
100-ply results show that a higher default is useful.

## Normal Run Command

```bash
/home/sag/RPA2/myLLC/AI/brilliance/aih/aichess/v4/bin/aih_v4
```

## Explicit Local Run Command

This is equivalent to the default local posture, but records the intent in the
command line:

```bash
/home/sag/RPA2/myLLC/AI/brilliance/aih/aichess/v4/bin/aih_v4 --local-retry-smoke
```

## Override Local Maxply

Use this only when intentionally testing a different local game depth:

```bash
/home/sag/RPA2/myLLC/AI/brilliance/aih/aichess/v4/bin/aih_v4 --local-maxplys=50
```

## Cloud Warning

Do not use `--allow-cloud`, `--full-agent-set`, or `--cloud-smoke-*` unless
cloud agents are intentionally part of the run. The v4 default is local-only.
