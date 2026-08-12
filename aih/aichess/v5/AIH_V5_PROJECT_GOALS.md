# AIH v5 Project Goals and Implementation Target

Date: 2026-08-12

Permanent public artifacts:

- Project goals: https://github.com/gray3s/brilliance/blob/main/aih/aichess/v5/AIH_V5_PROJECT_GOALS.md
- Project implementation plan: https://github.com/gray3s/brilliance/blob/main/aih/aichess/v5/AIH_V5_IMPLEMENTATION_PLAN.md
- Current CSV analysis: https://htmlpreview.github.io/?https://github.com/gray3s/brilliance/blob/main/aih/aichess/v5/AIH_V5_CSV_AGGREGATE_LATEST.html
- Current source code ZIP: https://github.com/gray3s/brilliance/blob/main/aih/aichess/v5/AIH_V5_SOURCE_LATEST.zip

AIH v5 is a local-first agentic-AI benchmark prototype. Its immediate purpose is to compare AI agents with a small, repeatable chess-harness workload while measuring observable AI hallucination/failure behavior, usable responses, timing, and throughput on hardware that a normal user can own and control.

The project is deliberately local-first rather than cloud-dependent. Local Ollama models are the baseline. Cloud/provider paths may be tested as explicit configurations, but a provider that requires an account, token, API key, or working Internet connection is not classified as local AI.

## Primary Goals

- Run a useful benchmark on laptop-class hardware without requiring cloud APIs.
- Register candidate agents before tournament play and exclude candidates that cannot reliably talk to the harness.
- Preserve registration failures and other AIH events in reporting rather than silently dropping them.
- Use bounded chess positions and constrained UCI-move prompts as a repeatable agent test harness.
- Rank agents primarily by measured AIH behavior, with response timing and weighted metrics available as additional ranking information.
- Persist machine-readable run data under `v5/data` so results can be recomputed rather than inferred from screenshots or terminal text.
- Generate browser-readable HTML summaries from the saved data.
- Keep the implementation inspectable and reproducible with C++, Qt, shell tooling, and local data files.
- Publish stable public links whose contents can be refreshed without changing the LinkedIn/GitHub URLs.

## Current Runtime Shape

The canonical user entry point is:

```bash
./bin/aih_v5
```

The current implementation is a binary-fronted local workflow:

- `aih_v5.sh` is the main orchestration wrapper.
- `./bin/aih_v5` is the binary launcher for that wrapper.
- `qwen_ollama_chess_qt/main.cpp` is the current Qt/C++ chess and model-interaction engine source.
- `tools/generate_aih_v5_html_report.cpp` is the current C++ HTML/CSV analysis reporter.
- `tools/generate_aih_v5_repeat_html.cpp` is the registration-repeat aggregate reporter.
- `tools/run_aih_v5_repeat.cpp` and shell helpers support repeat execution and local-stack handling.

The default local backend is Ollama over localhost HTTP. The current code also contains explicit provider/model labeling and controls for reasoning/performance/verbosity configurations. Those settings are benchmark configuration state and must not be confused with model identity.

## Registration and Tournament Model

Registration is a liveness/compatibility filter, not the tournament itself. A passing candidate has demonstrated that it can communicate with the harness within the configured limits. A registration failure or timeout remains evidence and is retained in the data.

Tournament play uses registered agents only. The principal v5 format is top-4 ladder-rungs with a round-robin seeding phase followed by bounded head-to-head placement. Other round-robin and ladder forms remain supported by the wrapper.

The move request is intentionally constrained: the agent is asked for one legal UCI move. Illegal, unparseable, irrelevant, empty, timed-out, and transport-failed responses are observable AIH/failure events. Legal accepted moves are non-AIH/usable events for the benchmark.

## AIH and Ranking Semantics

The current CSV processor uses two event classes per agent:

- AIH events: observable failures such as hallucinated/illegal/unparseable output, irrelevant output, transport failure, timeout, and registration failure where applicable.
- Non-AIH events: usable observed events, principally accepted legal responses and successful registration evidence where represented by the source data.

For the aggregate analysis:

`AIH% = AIH events / observed events`

Lower AIH% is better. The current report also exposes CSV-row counts, AIH and non-AIH event counts, average legal-event time, average AIH-event time, weighted performance, and token throughput when the underlying records support those measurements.

For this publication run, the CSV processor included 277 CSV files, ranked 6 agents, and observed 6340 events.

## Data and Reporting Contract

Active run data belongs under:

```text
aih/aichess/v5/data/
```

The current CSV analyzer scans the saved v5 data and combines ranking CSV, registration CSV, and event JSONL evidence when available. It writes the canonical public report:

```text
AIH_V5_CSV_AGGREGATE_LATEST.html
```

and mirrors the same report locally under:

```text
data/AIH_V5_CSV_AGGREGATE_LATEST.html
```

The registration-only aggregate is a separate artifact and must not be substituted for the broader CSV aggregate.

## Thought / Reasoning / Verbosity Configuration

The current implementation keeps base provider/model identity separate from controls such as:

- `AICHESS_REASONING_PERFORMANCE_MODE`
- `AICHESS_OPENAI_REASONING_EFFORT`
- `AICHESS_VERBOSITY`
- `AICHESS_OPENAI_TEXT_VERBOSITY`

A base model tested under different reasoning or verbosity settings is a different benchmark configuration. Until every report carries those settings as explicit ranking columns, direct comparisons of the same base model under multiple thought modes should remain separated by run/reference configuration rather than being treated as one homogeneous sample.

## Hardware Philosophy

AIH v5 targets modest local hardware first. Better CPU, GPU, RAM, and SSD resources are accelerators, not assumptions built into the meaning of the benchmark. Default concurrency should remain conservative enough that local-model resource contention does not become the dominant measurement.

## Publication Contract

The public AIH v5 state is represented by four permanent files:

1. `AIH_V5_PROJECT_GOALS.md`
2. `AIH_V5_IMPLEMENTATION_PLAN.md`
3. `AIH_V5_CSV_AGGREGATE_LATEST.html`
4. `AIH_V5_SOURCE_LATEST.zip`

Their filenames and public links are intentionally stable. Publication updates replace their contents rather than creating new dated public links.

The source ZIP is source-only: current C/C++/Qt code plus supporting shell/build files. It excludes compiled binaries, object files, raw CSV/JSONL datasets, logs, caches, and run-output directories.

## Recreation Target

A recreation is close to the AIH v5 target when it can:

- build or expose `./bin/aih_v5`;
- register local candidates with bounded timeouts;
- execute the constrained chess harness over registered agents;
- retain observable failure/AIH evidence;
- persist run data under `v5/data`;
- repeat runs without opening a browser tab for every child run;
- regenerate the aggregate CSV/JSONL-derived HTML report from saved data; and
- reproduce the four canonical publication artifacts above.
