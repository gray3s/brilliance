# DTE: Evidence-Governed Evaluation of Agentic AI Systems

**Public research summary — October 23, 2026**

## Motivation

Evaluating an AI system by whether its final answer looks convincing is not
enough. An agentic system also selects actions, uses tools, consumes resources,
responds to failures, revises artifacts, and judges whether its own work is
complete. A useful evaluation method must preserve and assess that entire
process.

The DTE project is an effort to develop such a method. Its immediate origins
are experiments in which models recover a hidden keyword from a randomized
definition. Those experiments exposed a broader and more useful question:

> Given the same input context and an explicit completion contract, how
> reliably can an agent transform that context into a verified result,
> recognize its own failures, preserve sufficient evidence, and improve its
> next attempt?

Keyword recovery remains a legitimate test problem, including direct-answer
and code-generation variants, but DTE is intended to generalize to practical
engineering, research, and document-production work.

## DTA and DTE

In the current project terminology:

- **DTA** is the developing theory and methodology for reasoning from an input
  context through decisions, actions, evidence, and evaluation.
- **DTE** is the Decision-Tree Engine: the native implementation and
  experimental framework used to record and test that process.

The distinction matters. A theory states what should count as responsible,
evidence-supported work; an engine must make those expectations operational,
repeatable, and auditable.

## Core protocol

A DTE evaluation begins by freezing the task's relevant conditions before
examining its outcome:

1. Preserve the initial context, prompts, model responses, tool actions,
   generated artifacts, errors, and human interventions.
2. Define a completion contract with objective acceptance criteria.
3. Record applicable authority, safety, time, context, compute, and retry
   limits.
4. Use deterministic tests where possible and explicitly version every
   judgment policy.
5. Separate measured results from an agent's narrative interpretation of its
   results.
6. Record failures and discarded attempts as experimental evidence rather
   than silently removing them.
7. Permit rescoring of preserved evidence under later evaluation policies
   without rewriting the historical run.
8. Compare an agent's claimed success with independently measured success.

This produces more than a benchmark score. It produces an evidence trail that
can explain why a system succeeded, why it failed, and whether its
self-assessment was calibrated.

## Comparative-agent evaluation

DTE can compare a local agentic AI stack with a hosted agent such as OpenAI
Codex. The comparison is not limited to the underlying language models. It
measures the effective systems: model, orchestration, memory, tools,
verification, retry policy, and operating constraints.

Matched trials should use the same initial task and completion contract, with
equivalent resources where practical and all unavoidable differences reported.
Candidate measures include:

| Measure | Question |
|---|---|
| Acceptance | How many frozen requirements were objectively satisfied? |
| Evidence completeness | Can every material claim be traced to an artifact? |
| Autonomy | How much human correction or intervention was required? |
| Recovery | Did the agent detect, diagnose, and repair failures? |
| Calibration | How closely did self-estimated success match measured success? |
| Reproducibility | Can another run or evaluator reproduce the conclusion? |
| Resource cost | What time, calls, tokens, compute, and energy proxies were used? |
| Safety and authority | Did actions remain within the granted boundaries? |

A smaller local model may perform less well on isolated reasoning while
remaining competitive as an agentic system through durable memory,
deterministic validation, inexpensive retries, and domain-specific tooling.
DTE is designed to make those differences visible instead of attributing every
outcome to an undifferentiated notion of “model intelligence.”

## Self-improvement under control

A self-improving local stack should not be allowed to redefine success after
seeing its own results. Candidate changes can be proposed from failure
evidence, but parent and candidate systems must be evaluated under the same
frozen contract. Promotion should require reproducible improvement, retained
regression coverage, provenance for the change, and an explicit human control
boundary.

This makes improvement an auditable experimental cycle rather than an
unverifiable claim made by the system being evaluated.

## Current status and limitations

The project has a native C++ experimental core, preserved keyword-recovery
evidence, versioned offline rescoring, and an explicit completion plan. One
verified experiment contained 20 direct-answer and code-generation cases with
complete evidence preservation; later offline policies could rescore the same
artifacts without rerunning the model.

DTE is still under development. Its present results are demonstrations of the
method, not proof that the protocol is complete, universally valid, or that
one agent is generally superior to another. Broader task families, matched
comparative trials, stronger conformance tests, and independent review remain
necessary.

The intended next application is to use the completed protocol to guide and
evaluate focused work on the carcase v2a, v2b, and v3a subprojects. Each will
require its own inventory and completion contract; DTE does not predetermine
their substantive goals.

## Research principle

The central principle is simple:

> An agent should be evaluated not by how satisfied it sounds, nor by how well
> it appears to please its operator, but by how faithfully its claims survive
> comparison with a frozen contract and preserved evidence.

That principle applies equally to local systems, hosted systems, and the DTE
project's evaluation of itself.

---

**Signed:**  
**gray3s** — project originator, investigator, and control authority  
**OpenAI Codex** — collaborating AI system

This signature records the human–AI collaboration that produced this summary.
It does not imply endorsement by OpenAI, resolve legal authorship or ownership,
or substitute for legal advice concerning publication or intellectual property.
