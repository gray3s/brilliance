# AIH Approach v2

Created: 20260712 1741MDT

## Summary

AIH v2 turns the Brilliance discussion into a small, linked set of tests for
agentic-AI hallucination, reliability, state fidelity, and task suitability.

The goal is not to publish unfinished implementations yet. The goal is to
publish the approach, the test structure, and the project goals so interested
readers can follow the chain from problem analysis to future validation.

## Component Chain

1. Problem analysis identifies the core risk: agentic AI can produce fluent
   output while losing provenance, state, evidence, or task boundaries.

2. Project goals define the bounded claim: AI can generate useful source
   material and expose weak reasoning without pretending to solve every hard
   problem.

3. AIH Test Suite v1 turns the working material into three current test
   families: personal-history failures, K-PhD exams, and AI Chess Match.

4. The personal-history test family uses observed agentic-AI failures such as
   provenance hallucination, continuity hallucination, and decision instability.

5. The K-PhD knowledge-ladder test family evaluates agents through bounded
   exams with controlled time, I/O, and reference-source rules.

6. The Wikipedia-only exam is the first concrete knowledge-ladder example,
   limiting the agent to a defined reference packet rather than another remote
   AI agent.

7. AI Chess Match is the first formal state-fidelity test, using a referee to
   validate legal moves, timing, board state, move faults, and game endings.

8. HybridAI versions remain agent implementations; AI Chess Match is a
   HybridAI subproject and AIH smoke test used to evaluate those implementations.

9. The future v1/v2/vn implementation work can be published later after the
   bugs in the test harnesses and local stack adapters are better understood.

## Current Test Families

### 1. Personal-History Agentic-AI Failure Test

Tests whether an agent can accurately track what it did, what changed, what it
knows, and what it is merely inferring.

Primary risks:

- provenance hallucination,
- continuity hallucination,
- unstable decisions,
- non-auditable reasoning,
- project-flow disturbance.

### 2. K-PhD Knowledge-Ladder Test

Tests whether an agent performs reliably across educational levels, reference
rules, and academic tracks.

Current extensions:

- K-12 tracks including gifted and talented programs,
- college tracks including STEM, communication, law/public policy, health
  professions, and graduate research,
- controlled source modes such as `wikipedia_only`,
- human/agent grading disputes as evidence.

### 3. AI Chess Match

Tests whether an agent can operate inside a bounded formal state machine.

Primary measurements:

- legal move rate,
- move time,
- move faults,
- time faults,
- board-state fidelity,
- game-ending type,
- local-vs-cloud comparison,
- suitability of HybridAI versions for bounded agentic tasks.

## Linked Source Chain

- Brilliance problem analysis: https://github.com/gray3s/brilliance/blob/main/PROBLEM_ANALYSIS.md
- Brilliance project goals: https://github.com/gray3s/brilliance/blob/main/PROJECT_GOALS.md
- Brilliance development plan: https://github.com/gray3s/brilliance/blob/main/docs/project_development_plan_20260712.md
- Brilliance v1 direction: https://github.com/gray3s/brilliance/blob/main/docs/v1_direction_20260712.md
- AIH Test Suite v1: https://github.com/gray3s/brilliance/blob/main/v1/AIH_TEST_SUITE_v1_20260712.md
- Wikipedia-only exam definition: https://github.com/gray3s/brilliance/blob/main/v1/experiments/wikipedia_exam/wikipedia_only_exam_v1_20260712.md
- AI Chess Match plan: https://github.com/gray3s/brilliance/blob/main/v1/agent_chess_project_development_plan_20260712.md
- v1 progress report: https://github.com/gray3s/brilliance/blob/main/v1/progress_reports/v1_progress_report_20260712.md

## Publication Boundary

The current publication is the approach and test design.

The actual `vn` implementations, refined harnesses, and local/cloud stack
results should be published later, after the bugs in the current test harnesses
and HybridAI adapters have been worked through.
