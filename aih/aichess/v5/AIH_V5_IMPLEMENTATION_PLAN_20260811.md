# AIH v5 Implementation Plan

Date: 2026-08-11

This file supplements `AIH_V5_PROJECT_GOALS_20260811.md`. The goals file defines the target behavior. This implementation plan describes one practical way to build it.

Public artifact links:

- Project goals: `AIH_V5_PROJECT_GOALS_20260811.md`
- Project implementation plan: `AIH_V5_IMPLEMENTATION_PLAN_20260811.md`
- Current aggregate HTML summary: `data/AIH_V5_REGISTRATION_AGGREGATE_LATEST.html`

## 1. Build Shape

Create a local command-line benchmark with this user entry point:

```bash
./bin/aih_v5
```

Recommended internal components:

- `aih_v5` launcher: parses user flags, sets defaults, starts repeat mode when needed, and prints timestamps.
- Registration module: tests candidate agents and writes registration CSV data.
- Game harness module: sends chess move prompts to agents and parses UCI move responses.
- Tournament module: schedules round-robin and top-4 ladder-rungs phases.
- Single-run HTML reporter: builds a per-run report and status/failure page.
- Repeat-run HTML reporter: aggregates direct `v5/data/registration_status_run_*.csv` files.

The implementation may be shell, C++, Python, or another local-friendly stack, but it should produce a binary-like user command in `./bin/aih_v5`.

Recommended file layout:

```text
aih/aichess/v5/
  bin/
    aih_v5
    aih_v5_repeat
    aih_v5_html_report
    aih_v5_repeat_html
  tools/
    run_aih_v5_repeat.<ext>
    generate_aih_v5_html_report.<ext>
    generate_aih_v5_repeat_html.<ext>
  data/
    AIH_V5_CURRENT_RUN_AGGREGATE.jsonl
    AIH_V5_LATEST_RANKINGS.csv
    AIH_V5_REGISTRATION_STATUS.csv
    AIH_V5_REGISTRATION_AGGREGATE_LATEST.html
    registration_status_run_N_START_END.csv
    aichess_v5_*_rankings_top4-ladder-rungs_weighted.csv
  AIH_V5_REGISTRATION_DIAGNOSTICS.log
```

The exact language can vary, but active CSV data belongs directly in `v5/data`, not in top-level `v5` and not in `runs/` subfolders.

## 2. Default Flow

The default `./bin/aih_v5` flow should be:

1. Print start timestamp.
2. Reset or inspect the local Ollama stack.
3. Select local candidate agents.
4. Run registration tests in randomized order.
5. Save every candidate registration result to a direct CSV file in `v5/data`.
6. Filter tournament entrants to agents that passed registration.
7. Refuse tournament play if too few agents passed for the selected mode.
8. Run the seed round-robin phase.
9. Run the top-4 ladder-rungs phase.
10. Save run JSONL/summary artifacts under `v5/data`.
11. Generate HTML output.
12. Open the HTML output by default with Firefox using a file URI.
13. Print end timestamp and elapsed time.

For `--nruns=N`, wrap that single-run flow N times and save one registration CSV snapshot per child run. At the end, generate and open one aggregate HTML page.

## 3. Registration Implementation

Registration should be intentionally simple and weaker than gameplay.

Default liveness prompt:

```text
OK
```

Expected behavior:

- Send the prompt to each candidate agent through the configured stack.
- Count a response before timeout as a pass.
- Count timeout, transport failure, or empty response as a fail.
- Record the failure reason.
- Unload or reset local models between batches.

Recommended defaults:

```text
registration_timeout_seconds = 5
registration_batch_size = 5
registration_stack_reset_settle_seconds = 5
registration_order = random
registration_min_passes = 4
registration_keep_alive = 0s
registration_candidate_count = all
```

The registration CSV must be a direct file in `v5/data` and must include:

```text
candidate,status,reason,elapsed_seconds,timeout_seconds,avg_move_seconds,move_attempts
```

At registration time, `avg_move_seconds` and `move_attempts` may be blank/zero. After the tournament completes, backfill real per-agent move timing for that same run before saving the CSV snapshot in `v5/data`.

## 3A. Implemented Agent Label And Thought Mode Handling

The current v5 implementation emits one displayed agent label for each
ranking/report row. The label builder maps provider/model strings into these
displayed forms:

- `openai:<model>` becomes `c openai <model>`.
- `anthropic:<model>` becomes `c anthropic <model>`.
- `gemini:<model>` becomes `c google <model>`.
- `google:<model>` becomes `c google <model>`.
- `codex:<model>` becomes `l openai <model>`.
- every other model string becomes `l ollama <model>`.

Thought, reasoning, and verbosity are implemented as separate run configuration
controls. The wrapper sets these controls for each selected reasoning and
verbosity configuration:

- `AICHESS_REASONING_PERFORMANCE_MODE`
- `AICHESS_OPENAI_REASONING_EFFORT`
- `AICHESS_VERBOSITY`
- `AICHESS_OPENAI_TEXT_VERBOSITY`

For reasoning-matrix runs, the wrapper also records selected reasoning and
verbosity values in the reference config name.

The current ranking CSV has one `agent` column for base provider/model
identity. It also includes `tournament_format`, `ranking_mode`, `aih_weight`,
and `turn_time_weight`. It does not emit independent `reasoning` or
`verbosity` columns in `AIH_V5_LATEST_RANKINGS.csv`.

If this implementation state is not changed, a run using one thought/reasoning
configuration remains readable and correctly grouped by base agent. A run that
tests the same base model under multiple thought/reasoning/verbosity settings
will merge those settings into the same base agent row in the latest ranking
CSV and ranking table. That means same-model thought-mode comparisons must be
interpreted through separate run artifacts or reference config names until the
reporter adds explicit thought/reasoning/verbosity columns.

## 4. Game Harness Implementation

Use a minimal deterministic chess harness.

For each ply:

- Maintain a board state and legal UCI move list.
- Select a legal hint move.
- Prompt the agent for exactly one legal UCI move.
- Parse the response for UCI candidates.
- Accept a legal candidate.
- Mark illegal, unparseable, irrelevant, timeout, and transport failures as AIH events.

Recommended prompt form:

```text
Chess move request.
Ply: <n>
FEN: <fen>
Clue: use this legal UCI move: <uci>
Return exactly one legal UCI move and nothing else.
Answer format: e2e4
No prose. No explanation. No punctuation.
```

Recommended defaults:

```text
maxply = 16
board_concurrency = 1
move_timeout_seconds = 20
stack_timeout_seconds = 20
game_timeout_seconds = 900
response_attempts = 1
max_fatal_turn_errors = 1
ollama_num_thread = 1
starting_output_tokens = 1024
token_decrease_step = 5
token_increase_step = 10
```

Resetting the local stack before each board is acceptable if local stability requires it.

## 5. Tournament Implementation

Default mode:

```text
top4-ladder-rungs
```

Required behavior:

- Use registered agents only.
- Run a seed round-robin phase.
- Rank seed results by low AIH% and response timing.
- Select top four for ladder placement.
- Run semifinal boards.
- Run winners board for rank 1/2.
- Run losers board for rank 3/4.
- Save all phase outputs.

Support additional modes:

- Flat round-robin.
- Ladder.
- Round-robin into ladder.
- Top-4 ladder with lower rungs resolved by round-robin sequences.

## 6. Repeat Runner Implementation

Support:

```bash
./bin/aih_v5 --nruns=N
./bin/aih_v5 --minregs=N
```

Repeat-run behavior:

- Create a timestamped repeat folder.
- For each child run, print run number and status.
- Suppress repeated browser tabs from child runs.
- Preserve per-run summary HTML files.
- Preserve per-run registration CSV snapshots directly in `v5/data`.
- Generate a final aggregate HTML report.
- Open the final aggregate report.

For live progress, a stable live HTML wrapper may be opened once and refreshed as child run summaries are updated.

## 7. Aggregate HTML Implementation

The repeat HTML reader should use only CSV files named:

```text
data/registration_status_run_*.csv
```

Supported flags:

```bash
./bin/aih_v5_repeat_html --nruns=N
./bin/aih_v5_repeat_html --nruns=0
```

Behavior:

- `--nruns=0` reads all direct `v5/data/registration_status_run_*.csv` files.
- `--nruns=N` reads the oldest N matching direct CSV files in `v5/data` for the current target.
- Agents may differ by run.
- Attempts are summed per agent across processed rows.
- Coverage% is based on how many selected CSV files include that agent.
- Registration failures remain visible in the main ranking table.
- The input section should show only `data/registration_status_run_*.csv` and the number of processed CSVs. Do not list every CSV filename.
- The generated aggregate report should be written directly under `v5/data` as `AIH_V5_REGISTRATION_AGGREGATE_<timestamp>.html`, with `AIH_V5_REGISTRATION_AGGREGATE_LATEST.html` updated to the latest report.

Recommended columns:

```text
Rank
Agent
AIH%
Attempts
Total Runs
Coverage%
Pass%
Fail
Timeout
Timeout%
Avg Sec
```

`Avg Sec` should combine average registration seconds and average move seconds:

```text
registration_seconds / move_seconds
```

Use consistent fixed decimal formatting.

## 8. Ranking and AIH Calculation

AIH% should be the primary rank basis. Lower is better.

Count these as AIH events:

- registration failure;
- registration timeout;
- illegal move;
- unparseable move;
- irrelevant response;
- transport failure;
- move timeout.

Count these as successful/usable events:

- registration pass;
- legal move accepted by the harness.

Recommended sort:

1. AIH% ascending.
2. Pass/legal rate descending.
3. Timeout count ascending.
4. More attempts descending.
5. Agent name ascending.

## 9. Failure Handling

The binary should generate HTML even when the run fails.

Failure report should include:

- failure phase;
- reason;
- registration CSV path;
- diagnostic log path if present;
- candidate pass/fail table;
- start/end timestamps if available.

Do not leave the user with a silent terminal or no HTML output.

## 10. Validation Checklist

Before publishing an AIH v5 build, verify:

- `./bin/aih_v5` starts and prints a timestamp.
- Registration candidates are listed one per line.
- Registration pass/fail summary is printed.
- Failed registration agents appear in HTML.
- Round-robin and ladder phases print board/pair progress.
- Per-run CSV files include the required columns.
- Repeat HTML reads only direct `v5/data/registration_status_run_*.csv` files.
- `--nruns=0` aggregates all saved CSVs.
- `--nruns=N` aggregates the oldest N CSVs.
- Final aggregate HTML opens in the browser.
- A failed run still produces an HTML status page.

## 11. Publication Artifacts

Minimum public artifact set:

- `AIH_V5_PROJECT_GOALS_20260811.md`
- `AIH_V5_IMPLEMENTATION_PLAN_20260811.md`
- `data/AIH_V5_REGISTRATION_AGGREGATE_LATEST.html`

The aggregate HTML should be a real multi-run summary after a validated longer run.

## 12. Required Flags and Environment Controls

The implementation should support these user-facing flags:

```text
--nruns=N
--minregs=N
--registration-random
--registration-forward
--registration-reverse
--ladder_rungs=N
--round_robin_rounds=N
--AIH_weight=FLOAT
--local-maxplys=N
--no-html-open
--html-open
```

Useful environment controls:

```text
AIH_V5_DEFAULT_LOCAL_AGENTS
AIH_V5_REGISTRATION_TIMEOUT_SECONDS
AIH_V5_REGISTRATION_BATCH_SIZE
AIH_V5_REGISTRATION_STACK_RESET_SETTLE_SECONDS
AIH_V5_LOCAL_MAXPLY_CAP
AIH_V5_BOARD_CONCURRENCY
AIH_V5_MOVE_TIMEOUT_SECONDS
AIH_V5_OLLAMA_NUM_THREAD
AIH_V5_OPEN_HTML_REPORT
AIH_V5_NRUNS
AIH_V5_MINREGS
```

The CLI flags should override environment defaults where practical.

## 13. Minimal Data Schemas

Registration CSV:

```text
candidate,status,reason,elapsed_seconds,timeout_seconds,avg_move_seconds,move_attempts
```

Repeat status CSV:

```text
run_index,started,ended,status,command
```

JSONL event data should be rich enough to reconstruct:

- model/agent name;
- ply number;
- raw response;
- parsed UCI move;
- legal/illegal status;
- failure class;
- elapsed move time;
- timeout or transport failure state;
- board/game termination reason.

The HTML reader should not require terminal logs to compute the aggregate report.

## 14. Browser Behavior

Single run:

- Generate a timestamped HTML page.
- Copy/update a latest HTML page.
- Open the HTML page unless `--no-html-open` or equivalent environment state disables it.

Repeat run:

- Do not open one tab per child run.
- Preserve per-run HTML output files.
- Optionally open one live per-run HTML wrapper that refreshes.
- Always generate the final aggregate HTML.
- Open the final aggregate HTML by default with `firefox --new-tab file://<report-path>` unless HTML opening is disabled.

## 15. Recreation Acceptance Test

A recreation is close enough to the current AIH v5 target when this sequence works:

```bash
cd aih/aichess/v5
./bin/aih_v5 --nruns=3
```

Expected result:

- Registration candidates print one per line.
- Some agents may fail registration and still appear in aggregate HTML.
- Tournament play uses only registered agents.
- Per-run CSV files appear directly in `v5/data`.
- Final aggregate HTML opens and contains the required columns.
- No new browser tab opens for every child run.
- A failing run still creates a readable HTML status page.
