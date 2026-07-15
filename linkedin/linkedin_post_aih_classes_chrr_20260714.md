# Prototype LinkedIn Post - 2026-07-14

Today we moved the AIH / HybridAI work from isolated examples toward a more
usable test structure.

The point is not to build a universal AI benchmark.

The point is to define bounded test classes that make hallucination easier to
induce, observe, classify, reject, and record.

We are currently organizing AIH tests into three severity-ordered classes:

Class 1: rule-bound state / game-action hallucination

Class 2: provenance / workflow-history hallucination

Class 3: source-bound knowledge / education-ladder hallucination

The public label can stay simple: Class 1 AIH test, Class 2 AIH test, Class 3
AIH test.

The implementation details live in the repository.

We also started naming two important outcomes:

RAIH: recoverable AIH.

The process detects the hallucination, rejects the bad state, retries safely,
asks for help, or terminates without corrupting the project record.

UAIH: unrecoverable AIH.

The hallucination becomes accepted state, corrupts records, causes invalid
downstream work, or forces a manual reset because the process can no longer
reliably determine what happened.

That distinction matters because the real problem is not only whether an AI
agent hallucinates.

The real problem is whether the combined human/AI process catches ordinary
hallucination pressure before it becomes accepted project state.

That is where we are using CHRR as a working concept:

common hallucination rejection / resistance rate.

It is not an exact signal-processing analogy. It is a practical process metric
idea.

How resistant is the human/AI pair to common sources of hallucination: stale
state, weak evidence, bad assumptions, ambiguous scope, unverified claims,
invalid tool output, unsupported attribution, and fluent answers that should
have been rejected?

We also expanded Class 3 into a more general structure:

track -> level/license -> field/category -> difficulty band -> question

That lets us handle academic paths, professional-license background tests, and
category-specific difficulty distributions without pretending that one general
quiz evaluates everything.

This is still early work.

The goal for now is not to market a finished test program.

The goal is to build defensible local test structures, run small prototypes,
record the evidence, and learn where the structure itself misclassifies the
problem.

Small tools still need verification.

And local AI deployment still needs local evidence.

Project work:
https://github.com/gray3s/brilliance/tree/main/v1/AIH

HybridAI stack work:
https://github.com/gray3s/hybridai/tree/main/v3
