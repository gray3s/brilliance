# AIH v5 Implementation Plan

Date: 2026-08-13

This file supplements `AIH_V5_PROJECT_GOALS.md`. The goals file defines what AIH v5 is intended to measure and publish; this file describes the current practical implementation and the steps needed to keep the benchmark reproducible.

Permanent public artifacts:

- Project goals: https://github.com/gray3s/brilliance/blob/main/aih/aichess/v5/AIH_V5_PROJECT_GOALS.md
- Project implementation plan: https://github.com/gray3s/brilliance/blob/main/aih/aichess/v5/AIH_V5_IMPLEMENTATION_PLAN.md
- Current CSV analysis: https://htmlpreview.github.io/?https://github.com/gray3s/brilliance/blob/main/aih/aichess/v5/AIH_V5_CSV_AGGREGATE_LATEST.html
- Current source code ZIP: https://github.com/gray3s/brilliance/blob/main/aih/aichess/v5/AIH_V5_SOURCE_LATEST.zip

## 1. Preserve the Current User Entry Point

Keep the normal launch command stable:

```bash
./bin/aih_v5
```

The binary launcher should continue to hand control to `aih_v5.sh`, which owns configuration, registration, repeat-run behavior, tournament selection, reporting, terminal progress, and failure handling.

Do not require a cloud key for the default path. Ollama/localhost remains the local baseline.

## 2. Current Source Components

Maintain these principal source components:

- `aih_v5.sh`: orchestration and configuration wrapper.
- `qwen_ollama_chess_qt/main.cpp`: Qt/C++ chess/model interaction engine.
- `qwen_ollama_chess_qt/qwen_ollama_chess_qt.pro`: Qt project definition.
- `tools/script_binary_launcher.cpp`: launcher support.
- `tools/build_aih_v5.sh`: launcher build helper.
- `tools/generate_aih_v5_html_report.cpp`: current single-run and all-CSV report generator.
- `tools/generate_aih_v5_repeat_html.cpp`: registration-repeat aggregate generator.
- `tools/run_aih_v5_repeat.cpp`: repeat-run support.
- `tools/run_aih_v5_single_game.cpp`: bounded core board-play wrapper for
  direct uni-agent and inter-agent tests.
- local-stack discovery/intake/scan scripts under `tools/`.

Compiled binaries are build products, not publication source.

## 3. Registration Flow

Registration remains intentionally weaker than gameplay:

1. Discover/select candidate agents.
2. Test each candidate through the configured stack.
3. Record pass/fail, reason, elapsed time, timeout, and later move timing when available.
4. Keep failures in the evidence set.
5. Admit only passing candidates to tournament play.
6. Reset/unload local models between batches when required for stability.

Useful laptop defaults remain approximately:

```text
registration timeout      = 5 s
registration batch size   = 5
stack-reset settle        = 5 s
registration order        = random
minimum top-4 entrants    = 4
local keep-alive          = 0 s where supported
```

Registration CSV snapshots belong directly under `v5/data`.

## 4. Chess Harness

For each measured ply:

1. Maintain the board state and legal UCI move set.
2. Construct a constrained prompt requesting one legal UCI move.
3. Send it to the selected agent/provider path.
4. Parse UCI candidates from the response.
5. Accept a legal candidate.
6. Record illegal, unparseable, irrelevant, empty, timeout, or transport-failed responses as AIH/failure evidence.
7. Record timing and throughput fields when the provider/runtime exposes them.

Default execution should remain serial/conservative on local hardware unless the user explicitly raises concurrency.

## 4A. Core Board-Play Binary

Maintain a separate binary for direct board-play tests:

```bash
./bin/aih_v5_single_game
```

This binary should let the user test local agents without rerunning full
registration and wrapper-level tourney setup. It must write local logs by
default under:

```text
logs/single_game/
```

Required current manual test strings:

```bash
cd /home/sag/RPA2/myLLC/AI/brilliance/aih/aichess/v5
./bin/aih_v5_single_game --nruns=1 --uni-agent-play nemotron-3-nano:4b
```

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

The second command is the current bounded local-agent size ladder. It is used
to watch for responsiveness, memory, swap, timeout, and AIH degradation as the
selected local model footprint increases. It is not yet an automatic scan of
every installed local model.

The core binary should support both `--uni-agent-play` and
`--inter-agent-play`. Wrapper-level AIH v5 execution may call this core binary
with the appropriate mode once registration and model selection are complete.

## 5. Tournament and Ranking

The primary v5 tournament remains top-4 ladder-rungs:

- seed with round-robin observations;
- identify the top four candidates;
- run bounded head-to-head ladder placement;
- retain lower-rung evidence when more candidates are present.

Ranking semantics:

- AIH% is the primary reliability measure; lower is better.
- Legal/usable response rate is supporting evidence.
- Average legal-event time and AIH-event time expose latency differences.
- Weighted performance and tokens/sec are secondary efficiency measurements where valid data exists.

Do not hide failed registrations or failed move attempts merely because they did not contribute to a successful game result.

## 6. Repeat Runs

Support repeat execution through `--nruns=N` and target-registration execution through `--minregs=N`.

Repeat behavior should:

- print run progress;
- suppress repeated child-run browser tabs;
- preserve child-run evidence;
- keep active CSV/JSONL data under `v5/data`;
- generate one final aggregate report; and
- open only the final report unless explicitly configured otherwise.

## 7. Current All-CSV Analyzer

The current analyzer is implemented in:

```text
tools/generate_aih_v5_html_report.cpp
```

Build it with a C++17 compiler, then run it from the v5 root with one of its equivalent all-data options:

```bash
./<compiled-html-reporter> --csv-all AIH_V5_CSV_AGGREGATE_LATEST.html
```

Equivalent recognized switches are `--csv-data` and `--all-csv`.

The all-CSV path scans `v5/data`, combines available ranking CSV, registration CSV, and event JSONL information, writes the requested report, and updates:

- `AIH_V5_CSV_AGGREGATE_LATEST.html`
- local mirror `data/AIH_V5_CSV_AGGREGATE_LATEST.html`

The broader CSV aggregate is separate from `AIH_V5_REGISTRATION_AGGREGATE_LATEST.html`.

For this publication run, the CSV processor included 277 CSV files, ranked 6 agents, and observed 6340 events.

## 8. Data Contract

Keep enough machine-readable evidence to recompute the reports. Registration data should retain at least:

```text
candidate,status,reason,elapsed_seconds,timeout_seconds,avg_move_seconds,move_attempts
```

Event JSONL should retain enough information to reconstruct model identity, legal/illegal outcome, failure class, transport state, elapsed time, and board/ply context.

Ranking CSV should continue to retain base agent identity and ranking configuration. A future schema improvement should add explicit reasoning/verbosity columns before same-model multi-thought-mode runs are merged into one public ranking table.

## 9. Failure Handling

Every run should end with readable status information. If tournament execution fails, the system should still preserve registration/evidence files and generate a status/failure HTML page when practical.

Terminal output should identify start time, selected candidates, registration progress, tournament phase progress, report path, end time, and elapsed time so an apparent hang can be distinguished from a long local inference step.

All generated binaries must follow `/home/sag/RPA2/REQUIREMENTS.md`: write
local logs by default, keep logging level tunable, and preserve enough
start/end/exit, command, phase, error, and output-path evidence for post-run
analysis. Logs are evaluation evidence and should remain out of public push
scope unless explicitly included in a reviewed publication task.

## 10. Source Publication ZIP

The canonical public source archive is:

```text
AIH_V5_SOURCE_LATEST.zip
```

Rebuild it at publication time from the current v5 source tree. Include C/C++/Qt source and supporting shell/build files. Exclude:

- compiled executables and object files;
- raw CSV and JSONL data;
- logs and caches;
- `runs/`, `published_results/`, qualification caches, and other generated output;
- Git metadata; and
- GitHub-publication/handoff helper scripts that are procedure tooling rather than AIH v5 runtime source.

Include a manifest in the archive describing the inclusion/exclusion policy and listing the archived source files.

## 11. Canonical Publication Procedure

A publication update should modify and stage exactly these four repository paths:

```text
aih/aichess/v5/AIH_V5_PROJECT_GOALS.md
aih/aichess/v5/AIH_V5_IMPLEMENTATION_PLAN.md
aih/aichess/v5/AIH_V5_CSV_AGGREGATE_LATEST.html
aih/aichess/v5/AIH_V5_SOURCE_LATEST.zip
```

Do not stage dated copies, raw run data, binaries, local project-summary files, or unrelated working-tree changes.

Before committing:

1. Confirm the repository is `gray3s/brilliance` on branch `main`.
2. Fetch `origin/main`.
3. Require local HEAD to match remote HEAD before making the publication commit.
4. Refuse to continue if unrelated files are already staged.
5. Regenerate the CSV aggregate.
6. Rebuild the source ZIP.
7. Rewrite the two canonical Markdown files.
8. Stage only the four paths above.
9. Confirm the staged set exactly matches the four-path allowlist.

After committing and pushing, verify the remote bytes of all four canonical files against the local files.

## 12. Local-Only GitHub Procedure Summary

The GitHub-update project summary is operational documentation, not a public AIH v5 artifact. Keep it outside the Git repository at:

```text
~/RPA2/gpt/aih/v5/AIH_V5_GITHUB_UPDATE_PROJECT_SUMMARY.txt
```

It must never be included in the Git staging allowlist.

## 13. Acceptance Test

The publication process is complete when:

- the four canonical local files exist and are nonempty;
- only those four paths are staged;
- one publication commit is pushed to `origin/main`;
- remote `main` equals the resulting local commit;
- byte-for-byte verification succeeds for all four remote files; and
- the permanent GitHub/LinkedIn links remain unchanged.
