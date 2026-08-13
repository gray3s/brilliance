# Run AIH v5 Binary Instructions

Date: 2026-08-10

## Location

```bash
cd /home/sag/RPA2/myLLC/AI/brilliance/aih/aichess/v5
```

Run the default full v5 test with:

```bash
./bin/aih_v5
```

Local-agent discovery prefers the inherited qualification registry or
`ollama list`, and the default run merges live local Ollama discovery with the
tracked built-in local candidates
first so the first test is bounded and usable. Override the list with
`AIH_V5_DEFAULT_LOCAL_AGENTS`, `AIH_V5_WHITE_MODELS`, or `AIH_V5_BLACK_MODELS`.

## Registration Requirement

Every tournament run has a pre-tournament registration phase. Candidate agents
are not tournament agents until they pass registration.

Registration rules:

- v5 loads candidate agent configurations from the registry, local discovery,
  explicit model lists, or fallback defaults.
- In dry-run mode, registration records the planned candidates without starting
  agents.
- In real runs, each local candidate must pass a simple query-return
  registration smoke-test.
- Cloud/provider candidates require the required `API_KEY` config before they
  can be considered; cloud query-return registration is not yet implemented in
  the wrapper.
- The only agents that come out of registration are agents that successfully
  pass registration.
- Tournament rosters are built only from the registered pass set.

Registration status is written here:

```text
/home/sag/RPA2/myLLC/AI/brilliance/aih/aichess/v5/AIH_V5_REGISTRATION_STATUS.csv
```

## Default Flags

Running `./bin/aih_v5` uses these defaults:

```text
tournament_format = top4-ladder-rungs
local_pair_start = 1
local_pair_count = ladder_rungs + 1 = 4
registration_candidate_count = all
default_local_agents = gemma3:270m,qwen2.5:0.5b,smollm2:135m,llama3.2:1b,gemma3:1b,phi3:mini,mistral:latest,gemma3:4b,nemotron-3-nano:4b
ladder_rungs = 3
round_robin_rounds = 1
ranking_mode = weighted
AIH_weight = 0.5
turn_time_weight = 0.5
registration_smoke_enabled = 1
registration_timeout_seconds = 5
registration_keep_alive = 0s
registration_systemic_timeout_threshold = 2
registration_stop_after_passes = 4
registration_unload_timeout_seconds = 10
local_maxply_cap = 5
board_concurrency = 1
local_response_attempts = 1
local_move_timeout_seconds = 20
local_stack_timeout_seconds = 20
local_game_timeout_seconds = 900
open_html_report = 1
```

These defaults are intentionally set as a "will complete quickly" default set
for a normal local laptop. Registration probes every discovered local candidate
agent with a short HTTP query-return smoke test, then the tournament uses the
first four registered agents that pass. A true benchmark will involve many more
operations and user modification of the default values, especially the candidate
agent set, max-ply cap, round count, timeout values, and ranking weights. The
local move and stack timeouts are deliberately short so a stalled local agent
produces an HTML status report instead of leaving the binary apparently stuck
for many minutes.

If every registration candidate fails, the binary treats that as a likely local
stack failure. Before starting round-robin or ladder play, it stops loaded
Ollama candidate models, restarts the registration test once, and only exits if
the second registration pass also produces zero viable agents. That failure path
still generates the HTML status report.

For local-agent default runs, the binary does not need to wait for every local
candidate to fail before reacting. If registration sees two consecutive local
HTTP timeouts with zero agents passed, it treats that as a systemic Ollama-layer
failure, resets local candidate models, and restarts the registration pass once.

After registration completes and before any round-robin or ladder phase starts,
the binary prints:

```text
aih_v5: registration summary: passed=N tested=M pct_passed=P%
```

Registration HTTP probes request `keep_alive=0s` so smoke-test models unload
immediately after each probe instead of remaining resident in Ollama.
After each local registration probe, the binary also calls `ollama stop <model>`
and waits up to 10 seconds for that model to disappear from `ollama ps` before
launching the next local registration probe. This keeps local agents from being
launched on top of each other during registration.

The default run is a local-agent run. Cloud agents are not substituted into the
default tournament if local registration fails. A user who does not want to test
local agents must choose an explicit cloud mode/provider and provide the
required API key configuration.

At startup, the binary also cleans up stale v5-owned engine processes from a
previous interrupted run. It only targets `qwen_ollama_chess_qt` processes whose
command line resolves to this v5 tree, plus their direct `curl` children calling
the local Ollama `/api/generate` endpoint. It does not kill unrelated Ollama
service processes, non-v5 benchmark processes, or cloud-provider processes. Set
`AIH_V5_KILL_STALE_RUNS=0` to disable this cleanup.

The default local run uses `board_concurrency=1`, so round-robin boards execute
sequentially inside one engine phase instead of launching every board at once.
This keeps the default run from saturating laptop CPU/Ollama. Increase
`AIH_V5_BOARD_CONCURRENCY` deliberately for stronger machines.

Equivalent explicit command:

```bash
./bin/aih_v5 --top4-ladder-rungs --ladder_rungs=3 --round_robin_rounds=1
```

## Rules Of Play

Ladder:

- Select the registered roster.
- Pair agents in roster order against the next eligible agent.
- Use bracket/ladder mode for staged ranking pressure.
- Rank by the selected ranking mode: AIH-only, turn-time-only, or weighted.

Round-robin:

- Select the registered roster.
- Generate ordered pairings so each registered agent appears as white and black
  against each other eligible registered agent.
- Use only when the registered roster is small enough for the expanded board
  count.

Round-robin then ladder:

- Run a round-robin phase first using the selected registered roster.
- Run a ladder phase from that same registered roster.
- Use this when early cross-agent evidence is useful before ladder pressure.

Default top-4 seed round-robin into ladder:

- Select four registered agents by default.
- Run one flat round-robin seed pass across the four selected agents.
- Use the seed ranking to order the top-4 ladder when ranking data is available.
- The top-4 ladder has three rungs by default for four ladder-stage contestants.
- Rung 1 has two 2-player boards: rank 1 vs 2 and rank 3 vs 4.
- Rung 2 is a 2-player board between the two Rung 1 winners.
- Rung 3 is a 2-player board between the two Rung 1 losers.
- Those three rungs determine L1-L4.
- Use `--ladder_rungs=n` to change the ladder-rung count. `n` must be
  non-negative, must be no greater than the registered selected contestant
  count, and must satisfy the ladder-stage placement rule. The current v5
  implementation supports the default four-contestant, three-rung placement
  bracket.
- Larger runs may set `AIH_V5_LOCAL_PAIR_COUNT` above 4; agents beyond the top
  four can be used for lower-rung round-robin expansion.

## Registration-Only Binary Smoke

This attempts the query-return registration smoke-test and exits before running
a tournament.

```bash
./bin/aih_v5 --registration-only
```

## Default Full Binary Test

```bash
./bin/aih_v5
```

Equivalent explicit default:

```bash
./bin/aih_v5 --top4-ladder-rungs --ladder_rungs=3 --round_robin_rounds=1
```

After a successful run, v5 publishes the HTML report and opens it in the system
default browser.

## Core Board-Play Binary

The core board-play binary is:

```bash
./bin/aih_v5_single_game
```

It is the direct bounded board-play entry point. Use it when testing one or
more local agents without running the full registration/tourney wrapper.

It writes text logs here:

```text
/home/sag/RPA2/myLLC/AI/brilliance/aih/aichess/v5/logs/single_game
```

### Uni-Agent Board Play

Uni-agent play means each selected local agent plays both sides of its own
board. This is the preferred first test when evaluating larger local agents
under RAM/swap limits.

One local agent, one self-play board:

```bash
cd /home/sag/RPA2/myLLC/AI/brilliance/aih/aichess/v5
./bin/aih_v5_single_game --nruns=1 --uni-agent-play nemotron-3-nano:4b
```

Multiple local agents, one self-play board per agent:

```bash
cd /home/sag/RPA2/myLLC/AI/brilliance/aih/aichess/v5
./bin/aih_v5_single_game --nruns=1 --uni-agent-play \
  gemma3:270m \
  qwen2.5:0.5b \
  llama3.2:1b \
  phi3:mini \
  granite4:3b \
  nemotron-3-nano:4b \
  gemma3:4b
```

Longer single-agent self-play:

```bash
cd /home/sag/RPA2/myLLC/AI/brilliance/aih/aichess/v5
./bin/aih_v5_single_game --nruns=1 --uni-agent-play \
  --max-plies=16 --move-timeout=45 --stack-timeout=45 \
  nemotron-3-nano:4b
```

### Inter-Agent Board Play

Inter-agent play means two different local agents are tested against each
other on one board.

```bash
cd /home/sag/RPA2/myLLC/AI/brilliance/aih/aichess/v5
./bin/aih_v5_single_game --nruns=1 --inter-agent-play \
  nemotron-3-nano:4b gemma3:4b
```

### Local Size-Ladder Use

There is not yet an automatic responsiveness-based size walker. To evaluate
increasingly larger local agents today, explicitly list the local agents in
increasing expected footprint with `--uni-agent-play`. Stop adding larger
agents when swap pressure, response time, or game failures make the system
unusable.

Current storage/memory policy still applies:

- warn at `5G` free on `/home/sag/RPA2`;
- hard stop at `3G` free on `/home/sag/RPA2`;
- do not treat swap as equivalent to RAM for large-model viability.

### Cloud Scope

AIH v5 local-agent work is local-first. Cloud agent work is limited to
OpenAI ChatGPT-style reference agents when explicitly requested.

Do not use Gemini, Anthropic/Claude, Ollama Cloud, or other non-OpenAI cloud
agent paths for AIH v5 unless the project scope is explicitly reopened.

## HTML Processor Relaunch

To rebuild and open the latest registration aggregate HTML page from saved
registration CSV files:

```bash
cd /home/sag/RPA2/myLLC/AI/brilliance/aih/aichess/v5
./bin/aih_v5_repeat_html
```

The processor writes timestamped aggregate HTML under `data/` and refreshes:

```text
/home/sag/RPA2/myLLC/AI/brilliance/aih/aichess/v5/data/AIH_V5_REGISTRATION_AGGREGATE_LATEST.html
```

## Other Binary Test Modes

Ladder:

```bash
/home/sag/RPA2/myLLC/AI/brilliance/aih/aichess/v5/bin/aih_v5 --ladder --local-retry-smoke --local-maxplys=4
```

Round-robin:

```bash
AIH_V5_LOCAL_PAIR_COUNT=3 /home/sag/RPA2/myLLC/AI/brilliance/aih/aichess/v5/bin/aih_v5 --round-robin --local-retry-smoke --local-maxplys=4
```

Round-robin then ladder:

```bash
AIH_V5_LOCAL_PAIR_COUNT=3 /home/sag/RPA2/myLLC/AI/brilliance/aih/aichess/v5/bin/aih_v5 --round-robin-ladder --local-retry-smoke --local-maxplys=4
```

AIH-only ranking:

```bash
/home/sag/RPA2/myLLC/AI/brilliance/aih/aichess/v5/bin/aih_v5 --ladder --local-retry-smoke --local-maxplys=4 --ranking-mode=aih
```

Turn-time-only ranking:

```bash
/home/sag/RPA2/myLLC/AI/brilliance/aih/aichess/v5/bin/aih_v5 --ladder --local-retry-smoke --local-maxplys=4 --ranking-mode=turn-time
```

Weighted ranking defaults to equal weighting:

```text
AIH weight = 0.5
turn-time weight = 0.5
```

To override the AIH weight, provide one parameter; v5 infers the turn-time
weight as the complement:

```bash
/home/sag/RPA2/myLLC/AI/brilliance/aih/aichess/v5/bin/aih_v5 --ladder --AIH_weight=0.75
```

## Developer Validation

Syntax check:

```bash
bash -n ./aih_v5.sh
```

Build current v5 binaries:

```bash
cd /home/sag/RPA2/myLLC/AI/brilliance/aih/aichess/v5
./tools/build_aih_v5.sh
```

Expected built binaries:

```text
bin/aih_v5
bin/aih_v5_html_report
bin/aih_v5_single_game
```

## AIH v5 Admin Tasks

Current admin rule:

- Do not do more AIH v5 development without binning/review.
- Use the current binaries for immediate local-agent tests.
- Follow `/home/sag/RPA2/REQUIREMENTS.md`: every generated binary must generate
  local logs by default, with logging level tunable per binary or run.
- Keep generated run logs and result data out of public push scope unless a
  specific publish task requires them.
- Use the GitHub update helper only after reviewing the file set it prints.

GitHub update helper:

```bash
cd /home/sag/RPA2/myLLC/AI/brilliance/aih/aichess/v5
./aih_v5_github_update_files_20260812.sh --dry-run
```

Open the helper in `gedit` for review:

```bash
gedit /home/sag/RPA2/myLLC/AI/brilliance/aih/aichess/v5/aih_v5_github_update_files_20260812.sh
```

Push only after the dry-run file set is acceptable:

```bash
cd /home/sag/RPA2/myLLC/AI/brilliance/aih/aichess/v5
./aih_v5_github_update_files_20260812.sh
```

## Output

After a successful non-dry-run, v5 publishes the HTML report and opens the
latest HTML report in the system default browser by default. To suppress browser
opening:

```bash
AIH_V5_OPEN_HTML_REPORT=0 ./aih_v5.sh ...
```

HTML/report output should follow this algorithm:

1. Read the registration CSV and show the registration pass/fail set before any
   tournament rankings.
2. Read the latest run JSONL emitted by the engine.
3. Preserve tournament phase labels in the report: ladder, round-robin,
   round-robin-then-ladder, or lower-rung round-robin into top-4 ladder.
4. Group events by agent/model.
5. Compute AIH percentage, legal percentage, agent-output hallucination
   percentage, harness-output hallucination percentage, and ply count.
6. Compute net turn time per ply from agent/referee turn timing where available.
7. Compute the selected ranking score:
   AIH-only, turn-time-only, or weighted AIH/time.
8. Sort rankings so lower scores are better.
9. Render separate phase summaries for hybrid modes, then render the combined
   ranking view.
10. Mark skipped/unregistered candidates separately from ranked agents.

Latest derived v5 ranking CSV:

```text
/home/sag/RPA2/myLLC/AI/brilliance/aih/aichess/v5/AIH_V5_LATEST_RANKINGS.csv
```

Latest v5 HTML report:

```text
/home/sag/RPA2/myLLC/AI/brilliance/aih/aichess/v5/AIH_V5_PRELIMINARY_RESULTS_20260810.html
```

The ranking CSV includes:

- AIH percentage
- net turn time per ply
- weighted score
- legal percentage
- agent-output hallucination percentage
- harness-output hallucination percentage
- ply count

## Runtime Dependency Rule

The v5 binary must not depend on any pre-v5 AIH code path. `bin/aih_v5`
launches `aih_v5.sh` from the same v5 tree, and `aih_v5.sh` launches the engine
from `qwen_ollama_chess_qt/` under that same tree. Both launcher layers refuse
to run if they resolve outside `/aih/aichess/v5`.

Archived/generated result files may still contain older historical labels, but
they are not runtime dependencies for the v5 binary.
