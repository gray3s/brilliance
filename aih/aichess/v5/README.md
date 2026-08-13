# AIH v5

AIH v5 is the final local-first AIChess benchmark line.

It adds tournament-format and ranking-metric support while keeping the default
run small enough to complete quickly on a normal local laptop. The current
local-agent set is sufficient for a basic v5 benchmark build; larger
open-source/local rosters can be added later as local support improves.

## Primary Files

- `aih_v5.sh` - v5 wrapper/launcher
- `bin/aih_v5` - v5 binary launcher for `aih_v5.sh`
- `bin/aih_v5_single_game` - bounded core board-play binary
- `tools/run_aih_v5_single_game.cpp` - source for the core board-play binary
- `AIH_V5_PROJECT_GOALS_20260810.md` - v5 goals
- `AIH_V5_PROJECT_DEVELOPMENT_PLAN_20260810.md` - v5 development plan
- `RUN_AIH_V5_BINARY_INSTRUCTIONS_20260810.md` - run instructions
- `aih_v5_github_update_files_20260812.sh` - reviewed push helper

## Tournament Formats

- Ladder: `./aih_v5.sh --ladder ...`
- Round-robin: `./aih_v5.sh --round-robin ...`
- Round-robin then ladder: `./aih_v5.sh --round-robin-ladder ...`
- Default full v5 test: `./bin/aih_v5`
- Lower-rung round-robin into top-4 ladder override: `./bin/aih_v5 --top4-ladder-rungs ...`

## Core Board-Play Modes

- Uni-agent play: `./bin/aih_v5_single_game --nruns=1 --uni-agent-play MODEL`
- Inter-agent play: `./bin/aih_v5_single_game --nruns=1 --inter-agent-play WHITE_MODEL BLACK_MODEL`

Use uni-agent play first when testing larger local models that may not fit
comfortably beside another large model.

Core board-play logs are written under:

```text
logs/single_game/
```

All generated binaries must follow `/home/sag/RPA2/REQUIREMENTS.md`: write
local logs by default, with logging level tunable per binary or run.

## Ranking Modes

- AIH-only: `--ranking-mode=aih`
- Turn-time-only: `--ranking-mode=turn-time`
- Weighted: `--ranking-mode=weighted`

Weighted mode accepts:

- `--ranking-aih-weight=N`
- `--ranking-turn-time-weight=N`

## Latest Ranking Output

```text
AIH_V5_LATEST_RANKINGS.csv
```

## Caveat

This is currently a wrapper-level v5 fork. Some underlying engine behavior,
binary names, generated paths, and report labels still come from AIH v5 until
the engine is refactored.
