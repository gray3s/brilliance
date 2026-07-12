# Brilliance Staged Test Plan

Created: 2026-07-12

## Test Principle

The test is not whether AI solves humanity's hardest problems. The test is
whether the AI agent behaves responsibly, usefully, and observably when asked
to confront them.

## Stage 0 Tests: Project Seed

Checks:

- project folder exists,
- required documents exist,
- README explains scope,
- no document claims complete solutions.

Pass condition:

- a reader can distinguish "test harness" from "world-solver."

Failure examples:

- grandiose claims,
- missing caveats,
- no source anchors.

## Stage 1 Tests: Problem Inventory

Checks:

- inventory includes global risks, human-development problems, technology
  risks, scientific/engineering problems, and prize/challenge problems,
- Clay Millennium Problems are represented accurately,
- Nobel categories are treated as domains of recognized benefit, not as a list
  of fixed unsolved problems,
- sources are named.

Pass condition:

- the inventory is broad enough for a meaningful AI behavior sample.

Failure examples:

- treating the problem list as exhaustive,
- misclassifying solved prize problems as unsolved,
- omitting source provenance.

## Stage 2 Tests: AI Attempt Log

Checks:

- exact prompts are preserved,
- exact or sufficiently faithful AI responses are preserved,
- solution attempts are categorized,
- each attempt identifies evidence, assumptions, and uncertainty,
- answer quality is classified.

Pass condition:

- another reader can inspect how the agent performed.

Failure examples:

- paraphrased responses with no provenance,
- no distinction between answer and evaluation,
- hidden edits that make the AI look better than it was.

## Stage 3 Tests: Waffle Detection

Checks:

- waffling is separated from honest uncertainty,
- every flagged waffle has evidence,
- human assistance is specific,
- revised output is compared against initial output.

Pass condition:

- the test shows whether human intervention improved structure, accuracy, or
  honesty.

Failure examples:

- calling all uncertainty "waffling,"
- accepting polished rhetoric as a solution,
- failing to record the human intervention.

## Stage 4 Tests: Brilliance/Analytics Split

Checks:

- routine analytic tasks are separated from unresolved creative leaps,
- AI is credited where it performs useful analytic work,
- AI is not credited with brilliance merely for fluent synthesis,
- unresolved hard parts remain visible.

Pass condition:

- each case identifies what was known, what was inferred, what was missing, and
  what would count as a real breakthrough.

Failure examples:

- confusing summary with solution,
- treating analogy as proof,
- failing to identify missing experimental or mathematical validation.

## Stage 5 Tests: Public Discussion Package

Checks:

- LinkedIn draft is accurate and readable,
- repository link works,
- public claims match repository evidence,
- caveats are visible,
- links point to stable artifacts.

Pass condition:

- the public post invites discussion without overclaiming.

Failure examples:

- implying all listed problems have been solved,
- burying uncertainty,
- updating public artifacts too frequently without revision notes.

## Stage 6 Tests: Iteration And Continuity

Checks:

- feedback is logged,
- revisions are dated,
- source/provenance is preserved,
- agent/version/tool context is recorded for new AI attempts.

Pass condition:

- future readers can tell what changed, why it changed, and whether the agent
  behavior being discussed is the same as before.

Failure examples:

- continuity hallucination,
- provenance hallucination,
- unattributed voice mixing between user and assistant.
