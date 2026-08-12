# AIH v5 Project Goals and Implementation Target

Date: 2026-08-11

Public artifact links:

- Project goals: `AIH_V5_PROJECT_GOALS.md`
- Project implementation plan: `AIH_V5_IMPLEMENTATION_PLAN.md`
- Current CSV analyzer HTML summary: `AIH_V5_CSV_AGGREGATE_LATEST.html`

AIH v5 is a local-first agentic AI benchmark prototype. The immediate goal is to compare locally available AI agents under a small, repeatable chess-harness workload that can run on a normal laptop and improve as local hardware improves.

This document is intentionally written as a project recreation target. A user should be able to paste it into a coding agent as the primary prompt, add ordinary build/environment details for their machine, and ask the agent to recreate the AIH v5 behavior.

## Primary Goals

- Run on the current local machine without requiring cloud APIs.
- Use only agents that pass a pre-tournament registration test.
- Keep default settings short enough to complete in a practical laptop session.
- Rank agents by observed AI hallucination behavior and useful response performance.
- Generate user-readable HTML output by default.
- Preserve run CSV data directly in `v5/data` so multiple runs can be aggregated from one data folder.
- Prefer open-source and locally inspectable implementation choices.
- Treat better CPU, GPU, RAM, and SSD hardware as accelerators, not as baseline requirements.

## Runtime Architecture

Implement AIH v5 as a command-line binary or binary-fronted script named:

```bash
./bin/aih_v5
```

The binary should orchestrate four layers:

- Registration layer: discovers candidate agents and verifies that each candidate can respond through the configured local stack.
- Game harness layer: runs small chess-position move requests against registered agents.
- Tournament layer: schedules round-robin and ladder/rung phases.
- Reporting layer: writes CSV data and HTML summaries.

The default local AI backend is Ollama over localhost HTTP. Cloud agents may be supported later, but the default build should be local-first and should not require API keys.

The implementation should avoid saturating the host by default:

- Run one board at a time unless the user explicitly raises concurrency.
- Use one Ollama thread by default where supported.
- Reset or unload local models between registration batches and before tournament boards when needed.
- Use conservative timeouts so dead or overloaded agents are filtered instead of holding up the tournament.
- Produce terminal progress output continuously so the user knows the current run state.

## Default User Command

The default command should work without extra flags:

```bash
./bin/aih_v5
```

For repeated trials, support:

```bash
./bin/aih_v5 --nruns=9
```

For target registration coverage, support:

```bash
./bin/aih_v5 --minregs=9
```

When `--nruns` is greater than 1, the runner should preserve per-run summaries and create an aggregate summary without opening a new browser tab for every child run. A live per-run summary page that refreshes during the repeat run is acceptable. The final aggregate HTML should open when the repeat run completes.

## Candidate Agent Defaults

The default candidate set may be changed by the user, but a useful local starting set is:

```text
gemma3:270m
tinyllama:latest
qwen2.5:0.5b
smollm2:135m
llama3.2:1b
gemma3:1b
phi3:mini
mistral:latest
gemma3:4b
llama3.2:3b
phi4-mini:latest
granite4:3b
qwen2.5:latest
qwen2.5-coder:3b
qwen:4b
robit/qwen3.5-9b-r7-research:q4km
```

The candidate order should be random by default so registration behavior is not biased by a fixed order. Also provide flags for deterministic order when debugging.

## Registration Stage

Registration is a pre-tournament filter, not a ranking tournament.

- Candidate agents are tested through the configured local stack.
- Agents that time out or fail the registration smoke test are filtered out of tournament play.
- Registration failures remain visible in aggregate reporting.
- Registration should be weaker than tournament play. Passing registration means the agent can talk to the harness, not that it can play well.
- A simple liveness query is acceptable as the default registration smoke test.
- Track elapsed registration time per candidate.
- Track timeout count and failure reason per candidate.
- Save rows for agents tested in that run, including failures, to CSV.
- Batch local registration attempts, for example 5 agents per batch, and reset/unload the local stack between batches.
- Use a stack-reset settle wait after reset, for example 5 seconds.
- If consecutive local timeout behavior suggests a stack problem, reset the local stack and continue with bounded retry behavior.
- Do not enter an agent into tournament play unless it passed registration.

Default registration settings should be tuned for a laptop:

- Registration timeout: about 5 seconds unless local testing shows a better value.
- Registration batch size: 5.
- Registration minimum passes for tournament start: 4 when using top-4 ladder-rungs mode.
- Registration keep-alive: 0 seconds for Ollama where supported.

## Agent Identity And Thought Mode Reporting

The implemented v5 report uses one displayed agent label for each ranked
agent row. The label identifies locality/provider and model identity. Current
displayed forms are:

- `l ollama <model>` for local Ollama-backed models.
- `c openai <model>` for OpenAI cloud models.
- `c google <model>` for Google/Gemini cloud models.
- `c anthropic <model>` for Anthropic cloud models.

Thought, reasoning, and verbosity are implemented as separate run configuration
controls, not as a second line appended to the displayed agent label. Current
implemented controls include:

- `AICHESS_REASONING_PERFORMANCE_MODE`
- `AICHESS_OPENAI_REASONING_EFFORT`
- `AICHESS_VERBOSITY`
- `AICHESS_OPENAI_TEXT_VERBOSITY`

For benchmark interpretation, the same base model under a different thought,
reasoning, or verbosity setting is a different test configuration. If the
current implementation is left unchanged, the human-readable ranking table and
latest ranking CSV continue to group rows by the one base agent label. That is
acceptable for a single default thought/reasoning configuration. It is not
sufficient for direct side-by-side ranking of the same base model across
multiple thought/reasoning/verbosity settings unless the run is separated by
reference config, run artifact, or a future report column.

## Tournament Stage

The default tournament format is top-4 ladder-rungs:

- First run a round-robin seeding phase over the registered agents selected for the trial.
- Select or rank the top four candidates from the seed results.
- Run a ladder/rung phase for the top four.
- For four ladder contestants, run two 1v1 boards, then one winners board and one losers/consolation board to determine ranks 1-4.
- Lower rungs may be resolved by round-robin sequences when more candidates are present.
- Run one board at a time by default.
- Reset the local stack before each board if local stability requires it.

Default game settings should prefer completion over depth:

- Maxply default: 16 for the current v5 target.
- Board concurrency: 1.
- Move timeout: about 20 seconds by default.
- Response attempts: 1 by default for local repeatable runs.
- Fatal turn error threshold: 1 by default.
- Clue/scaffold mode may include a legal UCI move hint so the test measures whether the agent can stay inside a constrained answer format.

The move prompt should ask for exactly one legal UCI move and no prose. The parser should accept legal UCI candidates from a verbose response when possible, but hallucinated, illegal, irrelevant, empty, timed-out, or transport-failed responses should be counted as AIH events according to the reporting model.

## Ranking Model

Rank agents by low AIH rate first, then useful response behavior.

AIH% should include:

- registration timeouts/failures as AIH events for aggregate visibility;
- gameplay illegal moves;
- unparseable move responses;
- irrelevant agent returns;
- transport failures and request timeouts when they affect the agent's ability to play.

Legal% should measure legal or accepted agent outputs over total attempts.

For a weighted ranking mode, support an AIH weight flag:

```bash
./bin/aih_v5 --AIH_weight=0.75
```

If only one weighting flag is supplied, derive the complementary turn-time weight as `1.0 - AIH_weight`. Equal weighting should be the default when no explicit weight is given.

## CSV Data Contract

Write a per-run registration CSV with at least these columns:

```text
candidate,status,reason,elapsed_seconds,timeout_seconds,avg_move_seconds,move_attempts
```

Column meaning:

- `candidate`: agent/model name.
- `status`: `pass` or `fail`.
- `reason`: concise pass/fail reason.
- `elapsed_seconds`: registration test elapsed time.
- `timeout_seconds`: registration timeout used for that row.
- `avg_move_seconds`: average observed move time for that agent in that run, if available.
- `move_attempts`: number of move attempts contributing to `avg_move_seconds`.

For old/demo CSV files that lack real move timing, artificial placeholder move times may be used only for display testing. Use values that are obviously impossible or conservative, and do not confuse them with real run timing.

Repeated runs should save CSV snapshots named similarly to:

```text
registration_status_run_N_START_END.csv
```

The repeat HTML aggregator should read only direct files in `v5/data` matching:

```text
data/registration_status_run_*.csv
```

It should not read CSV files from `runs/` or from subfolders under `v5/data`. It should handle different candidate sets across runs and compute per-agent totals from whatever rows exist.
If an agent does not appear in a given processed CSV, the HTML aggregator should not fabricate a row or treat that absence as AIH data.

## HTML Reporting Contract

The binary should generate HTML output by default and open it in the system browser.

Single-run report:

- Show run timestamp and settings.
- Show maxply value in the ranking section title.
- Show each agent with rank, attempts, AIH%, Legal%, Local?, Maxplys?, agent name/title, and registration timeout data.
- Include agents that failed registration, not only tournament entrants.
- Show registration timing and basic efficiency data.
- If a run fails before tournament play, still generate an HTML failure/status report explaining where it failed.

Repeat-run aggregate report:

- Read `registration_status_run_*.csv` files directly from `v5/data`.
- Support `--nruns=N`.
- Support `--nruns=0` to aggregate all saved direct `v5/data` registration CSVs.
- For the current target, when `--nruns=N`, summarize the oldest N matching CSV files in `v5/data`.
- Show how many CSV files were aggregated and what percentage of saved run CSVs they represent.
- `Coverage%` means: Agent in this % of CSVs.
- Rank rows by AIH% ascending, then pass rate descending, then timeout count ascending, then agent name.
- Show the input file set as a relative path spec and processed count, not a full filename listing.
- Use concise table columns:

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

`Avg Sec` should show registration seconds and move seconds in one cell, for example:

```text
4.300 / 1000.000
```

Use consistent decimal formatting and tabular numerals so the table is readable.

## Terminal Output Contract

The binary should not appear to hang silently. It should print:

- start timestamp;
- selected registration candidates;
- registration test start;
- each agent start/pass/fail line;
- registration pass/total/percentage summary;
- round-robin start and board/pair progress;
- ladder/rung start and board/pair progress;
- HTML report path;
- end timestamp and elapsed time.

For repeat runs, print the current run index and total run count, or current run index and `minregs` target.

## Hardware Baseline Philosophy

The target is a laptop-class local benchmark. The current reference context is approximately an older i3-class laptop with limited RAM and SSD storage. Defaults should be selected so the run completes in reasonable time on that class of system.

The benchmark should not require GPU acceleration. If CUDA/GPU acceleration exists, the local stack may use it, but the benchmark should not estimate or fake GPU performance on hardware that does not provide it.

Storage and memory matter because local agent runs can stress RAM, swap, and SSD. The implementation should avoid unnecessary swap churn and avoid parallel local model execution by default.

## Publication Target

This file, the implementation plan, and the current aggregate HTML report are the public target artifacts for the AIH v5 prompt/build challenge. The implementation can be changed, but the target behavior is:

- local-first operation;
- registration filtering;
- bounded tournament execution;
- CSV persistence directly in `v5/data`;
- aggregate HTML reporting;
- browser opening of the generated aggregate report with `firefox --new-tab file://<report-path>` when HTML opening is enabled;
- defaults that complete on a modest laptop.

## Minimal Recreation Prompt

Use this document as the core prompt:

```text
Create AIH v5 as a local-first command-line benchmark named ./bin/aih_v5. Implement registration filtering for local Ollama agents, run a compact chess-harness tournament over registered agents, save registration CSV snapshots directly in v5/data, support repeated runs with --nruns and --minregs, and generate browser-opened single-run and aggregate HTML summaries. Follow the runtime architecture, default values, CSV contract, terminal output contract, and HTML reporting contract in AIH_V5_PROJECT_GOALS.md.
```

The user or builder should then provide the target language, build system, host OS, local model list, and any required Ollama installation details.
