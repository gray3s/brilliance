# AIH v1 Project Goals

Created: 2026-07-13

## Core Goal

Develop bounded hallucination tests that deliberately create opportunities for
AI agents, human reviewers, and human/AI pairs to hallucinate, then record what
was asked, what happened, what was accepted, and what should have been rejected.

## Boundary Choice

Anything we ask an AI agent to do is a valid hallucination-test candidate.

AIH v1 does not need to be marketed as a formal test, study, certification, or
evaluation program. For now it is a controlled project-development effort for
managing each test's potential to induce AIH.

For v1, AIH does not require a clean independent `x:y` agent test. If two roles
share model state, context family, backend, or runtime assumptions, the result
is still useful as an AI self-test or AI-mediated process test.

The concern is recorded explicitly: shared internal data means the result should
not be over-described as an independent `x:y` comparison. That distinction
matters when independence or concurrency is the variable under test.

For the current v1 goal, that distinction is not central. We want controlled
conditions that make hallucination observable, classifiable, and measurable.

This issue will recur throughout project-goal development. Realistic
implementations involve compromises: shared state, limited hardware, bounded
time, simplified validators, proxy choices, model availability, prompt limits,
and human-review constraints. Those compromises affect overall performance and
metric analysis.

AIH should not hide those compromises. Each run should record the practical
implementation boundaries that shape the result, so later reports do not
overstate what was actually tested.

The major analysis issue is expected nonlinearity. Implementation compromises
will not necessarily add up in a simple way. Small changes in hardware load,
context sharing, prompt shape, validator strictness, retry policy, or human
review behavior may interact and produce threshold effects. A metric that looks
stable under one configuration may shift sharply when two ordinary compromises
appear together.

Therefore AIH reports should avoid assuming that one factor's measured effect
can be linearly extrapolated across stack, workload, or review-process changes.

## RAIH And UAIH

Each test should identify whether it is expected to induce recoverable AIH,
unrecoverable AIH, or both.

`RAIH` means recoverable AIH. The process can detect the hallucination, reject
the bad state, retry safely, ask for help, or terminate without damaging the
project record.

`UAIH` means unrecoverable AIH. The hallucination becomes accepted state,
corrupts the record, causes invalid downstream work, or forces a manual reset
because the process cannot reliably determine what happened.

The current priority is not public marketing. The priority is controlling this
boundary: make hallucination observable while preventing test-induced failures
from contaminating project state.

## Current Class Order

Classes are ordered from most severe to least severe:

```text
Class 1 = rule-bound state/game-action hallucination
Class 2 = provenance/workflow-history hallucination
Class 3 = source-bound knowledge/education-ladder hallucination
```

Class 3 can be subdivided into education bands such as K-3, 3-6, 6-9, 9-12,
undergraduate years by major, master's tracks by field, and PhD tracks by
field. It can also include professional-license and certification background
tests divided by required category. Public summaries can still say `Class 3 AIH
test`; the repository should carry the detailed level, field, source, category,
difficulty-distribution, and grading rules.

For professional-license/certification paths, individual questions can be
targeted to a normal curve within the overall category: easy recognition,
routine application, mixed-context application, edge-case reasoning, and
expert-level discrimination. The goal is controlled hallucination testing, not
claiming to reproduce or replace an actual licensing exam.

The general Class 3 structure is:

```text
track -> level/license -> field/category -> difficulty band -> question
```

This allows multiple subcategories within each academic or professional path
without making public summaries overly specific.

The preliminary Class 3 candidate inventory should be migrated into this
structure before baseline retesting. The first retest goal is to find egregious
misclassifications across tracks, levels, categories, and difficulty bands, not
to claim broad education or professional-license evaluation coverage.

This is somewhat circular by design: the class structure defines the tests, and
the test results challenge the class structure. AIH does not need a perfect
classification ontology at this stage. It needs a defensible one that can be
revised when evidence shows that a category, level, or difficulty band is
misleading.

## CHRR Direction

AIH is working toward a CHRR-style metric for resistance to common sources of
combined human and AI hallucination.

The target is not only catastrophic failure or dramatic rail-excursion. The
target is ordinary hallucination pressure:

- bad assumptions,
- stale state,
- weak evidence,
- ambiguous scope,
- unverified claims,
- invalid tool output,
- unsupported attribution,
- fluent output that should have been rejected.

CHRR should eventually describe how often the process catches and rejects those
conditions before they become accepted project state.

## Evidence Standard

Each test should preserve:

- task prompt or prompt hash,
- agent response,
- parsing result,
- accepted/rejected status,
- deterministic referee or grader result where available,
- human or AI examiner judgment where applicable,
- raw artifact path,
- stack identity,
- run-isolation status.

## Run Boundary

Ordinary AIH evidence should be collected serially. Do not run multiple
simulations at once unless concurrency/load is the explicit variable under
test.
