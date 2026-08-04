# AIH v4 Project Development Plan

Date: 2026-08-03

Version:

`AIH v4 / AIChess`

## Purpose

AIH v4 evaluates AI agent state fidelity, rule-following, legal-action behavior,
and support behavior through bounded chess tournament runs.

The v4 line is a tournament/evaluation version, not a one-off smoke-test-only
version.

The purpose of AIH v4 is to generate hallucination statistics during bounded
agent action. It is not a chess-strength ranking system. Chess move legality,
continuation, termination, and board-state fidelity are instruments for
measuring hallucination behavior; they are not the primary claim.

## PDP / PG Relationship

This project-development plan is the working control document for v4 execution.
It exists to achieve the v4 project goal recorded in:

`AIH_V4_PROJECT_GOALS_20260729.md`

The PDP/PG pair is recorded in:

`../VERSION_PDP_PG_INDEX_20260803.md`

Work on v4 should start from this PDP, identify the relevant project-goal item,
then implement, test, document, and publish through the PDP structure.

## Entry Points

- Binary launcher: `bin/aih_v4`
- Wrapper script: `aih_v4.sh`
- Engine: `qwen_ollama_chess_qt/qwen_ollama_chess_qt`

## Current Run Modes

- Default local tournament slice: `bin/aih_v4`
- Explicit local retry run: `bin/aih_v4 --local-retry-smoke`
- Gemini cloud tournament slice: `bin/aih_v4 --cloud-representative-gemini`
- Local max-ply evaluation: `bin/aih_v4 --local-retry-smoke --local-maxplys=100`

Required next run-mode work:

- Add `--run-mode parallel`.
- Add `--run-mode serial`.
- Keep `parallel` as the near-term default until serial is implemented and
  verified.
- Record the selected run mode in JSONL metadata, summary Markdown, HTML, and
  the test table description.
- Keep `--boards N` as the board assignment count. It must not silently double
  as the run-mode or scheduling policy.

## Tournament Scope

The default binary posture remains local-only to avoid accidental cloud-token
spend.

The Gemini cloud mode is a cloud-vs-local tournament slice. It should pair
Gemini against the discovered passing local roster unless explicit environment
variables narrow the slice.

Batch controls:

- `AIH_V4_CLOUD_REPRESENTATIVE_LOCAL_START`
- `AIH_V4_CLOUD_REPRESENTATIVE_LOCAL_COUNT`

Tournament scheduling must be explicit. A user may choose parallel mode, but
AIH v4 reports must make that scheduling choice visible. Parallel execution on
one machine means agents are sharing local system resources; the report must not
hide that condition.

## Current Defaults

- Local max plies: `50`
- Cloud max plies: derived from local/cloud ratio and capped at `10`
- Local/cloud max-ply ratio: `4`
- Referee: `harness`
- Default local response attempts: `3`
- Default cloud representative response attempts: `3`
- Default cloud representative move/stack timeout: `10` seconds
- Irrelevant agent returns are AIH events and referee relevance failures, not
  invalid chess moves.
- Same-agent same-mode self-play: rejected by default

## Hallucination Statistics Policy

AIH v4 statistics are not completed-game counts. Do not require `Cmplt = yes`
before using the associated event statistics for each player. An incomplete
game can still provide valid hallucination evidence when the stop condition is
the measured behavior.

Completion status, termination reason, and transport/quota state must be kept
visible so statistics are interpreted correctly.

Use these categories separately:

- Completed-game evidence: full-game statistics and termination.
- Partial-game hallucination evidence: legal moves, agent output
  hallucinations, harness output hallucinations, invalid/unparseable returns,
  irrelevant returns, and stopped-playing events.
- Infrastructure-invalidated evidence: quota, authorization, transport, stale
  load, corrupt load, or failed preflight. These rows should not be presented as
  competitive AIH ranking rows.

## HTML Report Requirements

The aggregate HTML table columns should be:

`Rank | AIH% | Legal% | AgntOH% | HrnOH% | L/C | Agent Title`

Rules:

- Use `HrnOH%`, not `OH%`, for harness output hallucination.
- Use uppercase `L` or `C` in the `L/C` column.
- Do not prepend `l` or `c` to `Agent Title`.
- Put `L/C` immediately before `Agent Title`.
- Use lowercase `n/a`, not `N/A`.
- Do not rank a row at or near the top if there is reason to suspect it is
  invalid. Set its rank to `n/a`.
- Do not convert infrastructure-invalidated rows into clean-looking rankings.

Invalid or suspect row signals, from strongest to weakest:

1. Source result is `fail.qta` or quota/transport invalidated.
2. Source result is infrastructure-invalidated before useful agent evidence can
   be collected.
3. Source result is `fail.stp`.
4. `AgntOH% = 100.000`.
5. `AIH% = 0` and `Legal% = 0`.
6. `AIH% = 0` and `AgntOH% = 0`.
7. `AIH% = 0`.
8. `HrnOH% = 0`.
9. Row is unscored / `n/a`.

These signals are report-quality and row-quality checks. They do not mean
incomplete games are useless; they mean the report must not misrepresent the
evidence as a valid top ranking.

## Restart Preflight

Before restarting a benchmark, the binary/wrapper path should:

1. Unload all resident local agents/models.
2. Treat resident model loads as possibly stale or corrupt.
3. Verify no prior AIH v4 process is still active.
4. Clear generated artifacts from invalid or questionable runs.
5. Initialize the tournament with a consistent recorded run mode.

If unload or preflight verification cannot be confirmed, the next run should be
marked invalid before it starts.

## Agent Health Checks

Before the official tournament starts, and before each board match officially
starts, run a one- or two-message exchange with each player on that board.

The exchange should verify:

- the stack is alive,
- the expected model/agent is responding,
- the response is not stale or corrupt output,
- the latency is within the configured timeout.

If either player fails the pre-match health check, that board should not count
as an official match result.

## Artifacts

- Run JSONL files under `runs/aih_v4_pairwise_prototype_20260729/`
- Run summaries under `runs/aih_v4_pairwise_prototype_20260729/`
- Timestamped run reports under `data/`
- Preliminary summary: `AIH_V4_PRELIMINARY_RESULTS_20260729.md`
- Preliminary HTML: `AIH_V4_PRELIMINARY_RESULTS_20260729.html`
- Version run note: `RUN_AIH_V4_BINARY_LOCAL_DEFAULT_20260803.md`

## Project-Development Notes

- `project-development-logs/aih_v4_support_flag_no_self_diagnosis_20260803.md`
- `project-development-logs/aih_v4_invalid_ranking_and_runmode_issue_20260803.md`

## Known Issues

- Runtime binary runs generate local artifacts only. Git publication is not part
  of the wrapper runtime path.
- The current binary does not yet implement a coherent `--run-mode` flag.
- The current restart path does not yet unload local agents, verify clean model
  loads, clear questionable artifacts, or run pre-match health checks.
- Cloud tournament runs can terminate early when an agent produces an
  unparseable or illegal move and the configured fatal-turn threshold is hit.
- Published preliminary results are not chess rankings. They are AIH
  hallucination-statistics evidence and must not hide invalid-row signals.

## Verification

Minimum checks before treating v4 as ready:

```bash
bash -n aih_v4.sh
GEMINI_API_KEY=dummy ./bin/aih_v4 --cloud-representative-gemini --dry-run
```

Dry-run should show the expected board count and model assignments without
spending cloud tokens.

## Publication Rule

Do not publish or archive a v4 package unless this project-development plan and
the version run note reflect the current behavior of `bin/aih_v4`.
