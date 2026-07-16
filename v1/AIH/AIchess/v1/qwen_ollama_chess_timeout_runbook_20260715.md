# Qwen Ollama AI Chess Timeout Runbook

Created: 2026-07-15

Language policy for this run: Bash plus C++/Qt. No Python runner, Python
module, Python cache, or Python-generated active fixture is used.

Purpose: run bounded AI chess tests against locally installed Qwen-family Ollama models, measure how long each model runs before completing or needing termination, and see whether any model can complete a chess game under referee control.

## Local Version Snapshot

Installed locally:

```text
ollama client version: 0.21.0
tinyproxy version:     1.11.1
```

Current upstream release checks on 2026-07-15:

```text
ollama latest:    v0.32.0, published 2026-07-11
tinyproxy latest: 1.11.3, published 2026-03-07
```

Practical note:

- Ollama is materially out of date: local `0.21.0` versus upstream `0.32.0`.
- Tinyproxy is only slightly out of date: local `1.11.1` versus upstream `1.11.3`.

Release references:

- https://github.com/ollama/ollama/releases/tag/v0.32.0
- https://github.com/tinyproxy/tinyproxy/releases/tag/1.11.3

## Installed Qwen Models

Verified from shell `ollama list` on 2026-07-15:

```text
qwen2.5:latest
qwen2.5-coder:3b
qwen:4b
robit/qwen3.5-9b-r7-research:q4km
```

Use the explicit model list in the commands below.

## C++/Qt Runner

Project:

```text
/home/sag/RPA2/myLLC/AI/brilliance/v1/AIH/AIchess/v1/qwen_ollama_chess_qt/
```

Binary:

```text
/home/sag/RPA2/myLLC/AI/brilliance/v1/AIH/AIchess/v1/qwen_ollama_chess_qt/qwen_ollama_chess_qt
```

The runner uses:

- QtCore `QProcess` for child-process control and hard timeout termination.
- Ollama for model calls.
- Stockfish at `/usr/games/stockfish` as the chess referee/legal-move source.
- Bash only for build/run commands.

Output directory:

```text
/home/sag/RPA2/myLLC/AI/brilliance/v1/AIH/AIchess/v1/runs/qwen_ollama_chess_timeout_probe_qt_20260715/
```

The runner writes:

- one `.jsonl` file containing full per-model results,
- one `_summary.md` file with a compact table.

## What The Test Measures

Each model is asked for strict UCI chess moves.

The referee owns the board state. The model only proposes moves.

Termination states:

```text
completed          one-move response returned before timeout
move_timeout       model failed to return one move before per-move timeout
game_timeout       full game exceeded the total game timeout
illegal_move_limit model produced too many illegal or unparseable moves
ply_limit          game reached maximum plies without ending
game_completed     Stockfish reported no legal moves from the current position
```

For the "can any model complete a game" question, look for:

```text
Completed game = true
Termination    = game_completed
```

## Step 1: Capture Usage Before Running

In the Codex composer, run:

```text
/usage daily
```

Optionally also run:

```text
/status
```

Save or screenshot the output if you want to compare token cost after the test setup and run.

## Step 2: Build The C++/Qt Runner

```bash
cd /home/sag/RPA2/myLLC/AI/brilliance/v1/AIH/AIchess/v1/qwen_ollama_chess_qt
qmake6 qwen_ollama_chess_qt.pro
make
```

## Step 3: Make Sure Ollama Is Running

Check whether Ollama is reachable:

```bash
ollama list
```

If model runs fail with a connection error, start Ollama in a separate terminal:

```bash
ollama serve
```

Leave that terminal open while tests run.

## Step 4: Dry Run

This confirms the planned run without invoking any model:

```bash
/home/sag/RPA2/myLLC/AI/brilliance/v1/AIH/AIchess/v1/qwen_ollama_chess_qt/qwen_ollama_chess_qt \
  --dry-run \
  --mode both \
  --models qwen2.5:latest,qwen2.5-coder:3b,qwen:4b,robit/qwen3.5-9b-r7-research:q4km \
  --move-timeout 20 \
  --game-timeout 120 \
  --max-plies 40
```

## Step 5: Fast One-Move Timeout Probe

Use this first. It tests whether each model can return one legal move quickly.

```bash
/home/sag/RPA2/myLLC/AI/brilliance/v1/AIH/AIchess/v1/qwen_ollama_chess_qt/qwen_ollama_chess_qt \
  --mode one-move \
  --models qwen2.5:latest,qwen2.5-coder:3b,qwen:4b,robit/qwen3.5-9b-r7-research:q4km \
  --move-timeout 30
```

Interpretation:

- If a model hits `move_timeout`, it is not ready for game testing at that timeout.
- If a model completes but `legal` is false, it responded but failed the referee.
- If a model completes and `legal` is true, it is worth trying in game mode.

## Step 6: Short Game Completion Probe

This asks each model to play both sides under a tight total budget.

```bash
/home/sag/RPA2/myLLC/AI/brilliance/v1/AIH/AIchess/v1/qwen_ollama_chess_qt/qwen_ollama_chess_qt \
  --mode game \
  --models qwen2.5:latest,qwen2.5-coder:3b,qwen:4b,robit/qwen3.5-9b-r7-research:q4km \
  --move-timeout 20 \
  --game-timeout 180 \
  --max-plies 40 \
  --max-illegal 3
```

Use this to see which models fail quickly due to timeouts or illegal move loops.

## Step 7: Longer Game Completion Probe

Run this only after the short probe identifies models worth testing.

```bash
/home/sag/RPA2/myLLC/AI/brilliance/v1/AIH/AIchess/v1/qwen_ollama_chess_qt/qwen_ollama_chess_qt \
  --mode game \
  --models qwen2.5:latest,qwen2.5-coder:3b,qwen:4b,robit/qwen3.5-9b-r7-research:q4km \
  --move-timeout 60 \
  --game-timeout 900 \
  --max-plies 160 \
  --max-illegal 5
```

This is the better test for whether any model can actually complete a game.

## Step 8: Test One Model At A Time

If the all-model run is too slow, run one model explicitly:

```bash
/home/sag/RPA2/myLLC/AI/brilliance/v1/AIH/AIchess/v1/qwen_ollama_chess_qt/qwen_ollama_chess_qt \
  --mode game \
  --models qwen2.5:latest \
  --move-timeout 60 \
  --game-timeout 900 \
  --max-plies 160 \
  --max-illegal 5
```

Swap in any of these:

```text
qwen2.5:latest
qwen2.5-coder:3b
qwen:4b
robit/qwen3.5-9b-r7-research:q4km
```

## Step 9: Read The Results

List summaries:

```bash
ls -lt /home/sag/RPA2/myLLC/AI/brilliance/v1/AIH/AIchess/v1/runs/qwen_ollama_chess_timeout_probe_qt_20260715/*_summary.md
```

Open the newest summary:

```bash
gedit "$(ls -t /home/sag/RPA2/myLLC/AI/brilliance/v1/AIH/AIchess/v1/runs/qwen_ollama_chess_timeout_probe_qt_20260715/*_summary.md | head -n 1)"
```

Look for:

```text
Termination
Completed game
Plies
Legal moves
Illegal/unparseable
Elapsed s
```

## Step 10: Capture Usage After Running

In the Codex composer, run:

```text
/usage daily
```

Compare against the before snapshot.

The local Ollama model execution itself should not consume OpenAI tokens. The Codex cost comes from planning, file edits, command output, reading summaries, and follow-up analysis in this session.

## Recommended First Sequence

Run these in order:

```bash
cd /home/sag/RPA2/myLLC/AI/brilliance/v1/AIH/AIchess/v1/qwen_ollama_chess_qt
qmake6 qwen_ollama_chess_qt.pro
make
```

```bash
ollama list
```

```bash
/home/sag/RPA2/myLLC/AI/brilliance/v1/AIH/AIchess/v1/qwen_ollama_chess_qt/qwen_ollama_chess_qt \
  --dry-run \
  --mode both \
  --models qwen2.5:latest,qwen2.5-coder:3b,qwen:4b,robit/qwen3.5-9b-r7-research:q4km \
  --move-timeout 20 \
  --game-timeout 120 \
  --max-plies 40
```

```bash
/home/sag/RPA2/myLLC/AI/brilliance/v1/AIH/AIchess/v1/qwen_ollama_chess_qt/qwen_ollama_chess_qt \
  --mode one-move \
  --models qwen2.5:latest,qwen2.5-coder:3b,qwen:4b,robit/qwen3.5-9b-r7-research:q4km \
  --move-timeout 30
```

```bash
/home/sag/RPA2/myLLC/AI/brilliance/v1/AIH/AIchess/v1/qwen_ollama_chess_qt/qwen_ollama_chess_qt \
  --mode game \
  --models qwen2.5:latest,qwen2.5-coder:3b,qwen:4b,robit/qwen3.5-9b-r7-research:q4km \
  --move-timeout 20 \
  --game-timeout 180 \
  --max-plies 40 \
  --max-illegal 3
```

Then inspect the newest `_summary.md`.
