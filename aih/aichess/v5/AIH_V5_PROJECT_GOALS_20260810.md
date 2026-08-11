# AIH v5 Project Goals

Date: 2026-08-10

AIH v5 is a local-first agentic AI benchmark prototype. The immediate goal is to compare locally available AI agents under a small, repeatable chess-harness workload that can run on a normal laptop and improve as the local hardware improves.

## Primary Goals

- Run on the current local machine without requiring cloud APIs.
- Use only agents that pass a pre-tournament registration test.
- Keep default settings short enough to complete in a practical laptop session.
- Rank agents by observed AI hallucination behavior and useful response performance.
- Generate user-readable HTML output by default.
- Preserve per-run CSV data so multiple runs can be aggregated.

## Current v5 Tournament Model

AIH v5 uses a registration stage followed by tournament play.

Registration:

- Candidate agents are tested through the configured local stack.
- Agents that time out or fail the registration smoke test are filtered out of tournament play.
- Registration failures remain visible in aggregate reporting.

Tournament:

- Default tournament play uses a top-4 ladder-rungs format.
- Round-robin stages are used to seed and/or resolve rungs.
- Board execution is local and sequential by default to avoid saturating the host.
- The default maxply setting is intentionally limited so the binary is a will-complete-quickly baseline, not a full-strength chess benchmark.

Reporting:

- Per-run CSV files record registration status and timing.
- Repeat-run HTML aggregation can summarize multiple CSV runs.
- Future run CSVs include real move timing; older dummy CSVs may contain deliberately artificial placeholder move timing for display testing.

## Design Principle

The benchmark should work first on a present-day laptop. Better CPUs, GPUs, RAM, and SSDs should make the workload faster and possibly more capable, but they should not be required for the baseline test to complete.

## Publication Target

This file and the linked dummy HTML report are the public target artifacts for the AIH v5 prompt/build challenge. The implementation can be changed, but the target behavior is a local-first repeatable benchmark with registration filtering, tournament execution, and aggregate HTML reporting.
