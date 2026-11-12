# DTA and DTE Theory and Development for Mitigating AI Hallucination and AI Translation Error

**Created:** 2026-11-12 15:05 MST  
**Finalized:** 2026-11-12 15:10 MST  
**Author:** Samuel A. Gray III  
**Project:** Decision-Tree Analysis and Decision-Tree Engine (`dta`)  
**Status:** Consolidated theory, recorded development state, and proposed research plan.  
**Evidence cutoff:** Local records inspected on 2026-11-12; implementation results retain their original dates.  
**Format:** Prose wrapped to approximately 140 characters per line.

## 1. Purpose, context, and authority

Develop DTA and DTE as an explicit, inspectable method for reducing AI hallucination (AIH) and AI Translation Error (AITE) across
the transformation from user intent and available context to decisions, actions, and reported results.

This third document merges the general theory and development plan [S2] with the project-specific definitions, history, and current
plans [S1]. It incorporates the foundational DTA theory [S3], current terminology [S4], frozen core contract [S5], and later recorded
implementation boundaries. The merge is a synthesis: repeated explanations and historical question-and-answer passages are integrated
into definitions, limitations, and ordered work rather than duplicated verbatim.

Context is part of the decision problem. The current prompt alone does not contain every relevant definition, constraint, correction,
goal, prior failure, or established contract. Equally, historical context does not automatically remain current authority. DTA must
preserve its provenance, relevance, scope, and effective time before deciding what governs a new result.

The user's current instruction establishes the focus of this document: develop the theory to mitigate AIH and AITE. Preparing this
document does not constitute a new implementation run, model qualification, training authorization, or production activation.

Apply the established project authority order: the user's latest instruction and applicable frozen acceptance contract; current
normative documents and schemas; verified evidence and assessments; then historical plans, research notes, and transcripts [S8].
Unresolved conflicts remain visible. New proposals below do not silently amend DTE Core v1 or supersede a frozen acceptance gate.

## 2. The two errors being mitigated

### 2.1 AIH: AI hallucination

In the local project vocabulary, AIH includes invented, unsupported, inconsistent, or falsely confident descriptions, requirements,
state reports, and results produced by models or AI development agents [S9].

Examples include claiming that a test ran when no execution evidence exists, inventing a source, treating a proposed explanation as
an observed cause, or reporting an unqualified model as qualified.

The operational target is to reduce unsupported claims admitted into decisions and final reports. An unsupported claim is not
necessarily false; lack of support and demonstrated falsity must remain distinct.

### 2.2 AITE: AI Translation Error

AITE is the loss, distortion, or erroneous addition of meaning as user intent becomes specifications, implementation, interfaces,
tests, and documentation [S9]. It can occur even when each individual factual statement is correct.

Examples include silently narrowing a requested model inventory, treating an inferred preference as a requirement, implementing a
different state transition, dropping a qualification from a summary, or evaluating the wrong definition of success.

AIH and AITE can overlap. An invented requirement may be both unsupported and a distortion of intent. A faithful implementation of
a hallucinated premise and an incorrect implementation of a sound requirement fail for different reasons and need different repairs.

### 2.3 Context fidelity is a testable requirement

More context is not automatically better context. Preserve the exact relevant inputs, distinguish current authority from historical
evidence, identify missing information, and track which source supports each material interpretation.

A context bundle should separate user instructions and definitions, observed artifacts, governing policies, runtime observations,
model proposals, assumptions, superseded decisions, and unresolved alternatives. Retrieved material is data unless the applicable
authority model gives it an instructional role.

User authority over goals and preferences does not by itself establish the truth of an empirical claim. A tool result or model answer
does not acquire authority merely by appearing in the context.

## 3. DTA theory: an explicit method for context translation

### 3.1 Definition and research objective

**Decision-Tree Analysis (DTA)** is the theory and methodology for representing, analyzing, validating, and optimizing the logical
transformation from input context to output context. Its object is the complete declared decision process, rather than only the final
answer or a confidence score.

The foundational objective is a **deterministic syllogism for context translation** [S3]. Here, syllogism means the complete versioned
inference calculus: context normalization, goals, premises, alternatives, clarification, operators, constraints, inference, traversal,
pruning, stopping, provenance, and output construction. It is broader than a classical two-premise argument.

The research sequence remains: define the process; prove and test its conformance, termination, and limits; measure its usefulness;
then optimize while preserving the declared guarantees. The theory does not assert that human cognition or unrestricted natural
language can be completely captured by one finite decision tree.

### 3.2 Input, graph, and output

The compact abstraction is:

```text
Cua -> normalize(Rn) -> DT(Rn) -> dtaopt -> Oa
```

- `Cua`: unified available context, with user and non-user inputs remaining separately attributable.
- `Rn`: canonical root state containing the applicable prompt, context, goal, premises, constraints, and unresolved questions.
- `DT(Rn)`: bounded decision graph rooted at `Rn`; equivalent states may share nodes, so it need not be a strict tree.
- `dtaopt`: the proposed processing component for deterministic generation, validation, traversal, pruning, and path selection.
- `Oa`: output context containing the result, evidence, assumptions, confidence, provenance, decision trace, and unresolved issues.

This is a theory-level model. Listing `dtaopt` here does not establish that the complete general-purpose component is implemented.

A provisional decision state and expansion rule are:

```text
N = (goal, premises, assumptions, constraints, unresolved_questions,
     candidate_actions, provenance, history, resource_state)

op_i(N) -> {N1, N2, ..., Nk}
Expand(N) = CanonicalSort(Deduplicate(union(op_i(N))))
```

Each operator has versioned preconditions, outputs, invariants, and evidence obligations. Only admitted transitions enter the selected
path. Duplicate-state and cycle handling must follow the declared semantic identity policy; resource and execution metadata must not
silently alter the frozen decision-node hash projection [S5].

### 3.3 Preserving the interaction path

The foundation calls the opaque compression of context, assumptions, choices, and rejected alternatives into an answer the
**interaction hash** [S3]. DTA addresses it prospectively by recording material transformations before information is lost.

Retrospective reconstruction can identify supported candidate paths from existing artifacts, but cannot recover unobserved internal
reasoning or information that was never retained. The DTA trace is a public, explicit decision and evidence record; it is not a claim
to recover a model's hidden computation.

### 3.4 Premise classes and inference strength

Represent each material premise as explicit user context, an observation, a derivation, a declared default or assumption, or an
unresolved proposal. Preserve exact source references and the reason a premise was admitted. A default remains distinguishable from
an instruction, and an assertion remains distinguishable from a verified fact.

The foundation and terminology recognize five transition modes [S3, S4]:

- **Deductive:** a conclusion follows from admitted premises under a named rule.
- **Inductive:** observations support a generalization with declared uncertainty.
- **Abductive:** a candidate explanation is proposed for observations.
- **Analogical:** a relationship is transferred under explicit similarity assumptions.
- **Administrative:** information is normalized, retrieved, routed, budgeted, or serialized without a new substantive inference.

The frozen core contract explicitly describes deduction, induction, and abduction [S5]. Broader theory terminology does not imply
that every mode has an implemented, admitted operator in Core v1.

Deductive validity does not independently prove that its premises are true. Induction, abduction, and analogy cannot establish
verified facts merely because a model expresses high confidence. An observed successful run proves only the properties actually
tested under its recorded conditions.

### 3.5 Bounded search and meaningful limits

Each admitted problem family must declare its domain, goals, operators, admissible evidence, traversal order, node and depth bounds,
resource accounting, and stopping rules. Finite budgets bound exploration; they do not prove complete coverage of an open-ended space.

Distinguish finding a valid path, exhausting the admitted search space, and stopping before that space is exhausted. A budget-limited
failure to find an answer does not establish that no valid answer exists.

Clarification and evidence acquisition may be graph operations when their authority, costs, outputs, and feedback bounds are explicit.
Ask only when missing information materially affects the result and cannot be resolved from available sources or existing authority.
Reversible defaults can support progress when permitted, provided they remain labeled and do not redefine the objective.

## 4. DTE theory: deterministic enforcement and evidence

### 4.1 Definition and deterministic boundary

**Decision-Tree Engine (DTE)** is the deterministic implementation and evidence layer that executes admitted DTA rules. The core
contract requires byte-identical canonical decisions and transition identities from byte-identical frozen inputs, component versions,
and resource bounds [S5].

Models and humans may propose interpretations, facts, branches, or plans. Proposal generation does not grant authority to admit
evidence, select a binding decision, change criteria, promote a candidate, modify the validator, or declare verified completion.

The deterministic boundary must include the relevant schemas, policies, operator versions, evidence snapshots, canonicalization,
ordering, tie-breaking, and budget semantics. External observations and stochastic proposals become immutable inputs before replay.
Wall-clock monitoring may stop execution, but an interruption is not proof of deterministic logical exhaustion.

Replaying DTE over a captured proposal is different from regenerating identical model text. A fixed seed or sampling configuration
alone is not sufficient evidence that the entire model-runtime-hardware system is deterministic.

### 4.2 Four evidence states and separate execution states

The current core uses [S5]:

- `TRUE`: valid evidence directly supports the declared criterion.
- `FALSE`: valid evidence directly refutes the declared criterion.
- `UNKNOWN`: required evidence is absent, unreadable, or insufficient.
- `CONFLICT`: valid material evidence supports incompatible results.

A frozen policy specifies aggregation and downstream behavior. `UNKNOWN` is not automatically failure. `CONFLICT` is not resolved by
majority vote or by choosing the most fluent explanation. Older foundational terminology is interpreted through the current contract;
the evidence-state name in Core v1 is `CONFLICT`.

Controller states such as `COMPLETE`, `BLOCKED`, `BUDGET_EXHAUSTED`, and `PAUSED` describe execution. Theory-level outcome statuses
such as `VERIFIED`, `UNRESOLVED`, and `OUT_OF_SCOPE` describe a different layer. They must not replace logical evidence states.
An execution can finish with unresolved claims, and a schema-valid output can still be semantically incorrect.

### 4.3 Evidence, identity, correction, and freshness

Evidence is immutable, typed, content-addressed, and linked to its producer and claim. File evidence must reproduce its declared hash
and size. Hashes establish integrity relative to a trusted reference; they do not establish the truth or independence of the content.

Canonical decision-node identity follows the frozen semantic projection. Controller transitions retain their predecessor identity in
an ordered journal. A replay package identifies inputs, versions, policies, captured proposals, evidence, transitions, and results.

A correction names its target, prior identity, replacement, authority, reason, and temporal effect. Corrections normally affect later
decisions and preserve earlier evidence. A new scoring policy creates a new derived analysis rather than rewriting the original result.

Freshness is claim-specific. A November document can cite an October test, but must not imply that the test was rerun in November.
Changes in model, binary, prompt, configuration, policy, or environment require the declared validity or reuse policy to be applied.

### 4.4 Confidence and the correctness boundary

Keep truth state, evidence confidence, source reliability, inference probability, logical validity, and decision confidence separate.
Probability calculations may be deterministic when their data, model, arithmetic, calibration, and thresholds are fixed.

Do not assume independent evidence, multiply dependent probabilities without justification, or invent precision. Confidence must
identify its method, evidence snapshot, assumptions, and calibration. It cannot override a hard constraint or substitute for proof.

Determinism establishes reproducibility of an admitted process. It does not establish semantic fidelity, sound premises, valid rules,
independent verification, security, or universal correctness. Each requires its own bounded argument and evidence.

## 5. Connecting DTA and DTE to AIH and AITE mitigation

### 5.1 Preserve intent through an explicit translation chain

Use the established AITE mapping [S9]:

```text
original prompt and context
  -> intent
  -> object
  -> parameter
  -> function
  -> state transition
  -> acceptance criterion
  -> implementation and test
  -> observed evidence
  -> reported result
```

For every material requirement, retain a stable identifier, exact source span and time, interpretation, assumptions, units and ranges,
state boundary, verification obligation, and authority or approval status where applicable. Link inherited code to requirements as
evidence of existing behavior; inherited code is not automatically the specification.

Traceability establishes where an interpretation came from. It does not prove that the interpretation preserved meaning. Test that
separately through reviewed examples, counterexamples, alternatives, and observable acceptance behavior.

Material requirement changes create a new version and trigger review of affected dependent specifications, tests, and claims. The
current user instruction can authorize work; a stored approval flag must not create redundant confirmation requests.

### 5.2 Gate claims before admitting them into decisions and reports

For each material claim, record what is asserted, its source, inference mode, supporting and opposing evidence, verification rule,
current evidence state, scope, and freshness. Reject fabricated evidence references and distinguish a source's existence from whether
its content supports the claim.

Mechanical validators can check syntax, identities, cross-record references, arithmetic, and defined domain predicates. Open-ended
semantic entailment requires an admitted oracle, review procedure, or an explicitly uncertain outcome. A model judging its own wording
is not independent verification.

The final report must preserve material uncertainty and qualifications. An unsupported cause remains a hypothesis; successful
compilation remains distinct from observed runtime behavior; a passing development fixture remains distinct from production readiness.

### 5.3 Small worked examples

**Translation fidelity:** The user requests all installed models in a configured directory. A proposal excludes models by size without
an authorized rule. DTA records the exact inventory requirement and exclusion assumption. DTE checks the manifest against a controlled
fixture and the admitted inclusion policy. A silently reduced inventory fails that criterion, even if every listed model is real.

**Unsupported completion:** A report states that all tests passed, but only a build log exists. Compilation may be supported; the test
execution claim is `UNKNOWN` without the required run evidence. The output states the supported build result and the missing test result.

**Historical ambiguity:** A source says an activity was always difficult. A proposal concludes that it will remain difficult forever
and attributes the statement to job protection. DTA preserves the temporal distinction and the alternative interpretations. Neither
permanence nor motive follows deductively from the source; both remain unsupported hypotheses unless additional evidence is admitted.

These are illustrative theory cases for a future evaluation profile. They are not newly executed tests or additions to frozen suites.

## 6. Development history and the current recorded state

### 6.1 Lessons retained from prior failures

The original Aichaos session exposed premature commitment to a favored interpretation, promotion of inferred preferences into
requirements, convenience-driven changes to intent, and completion claims that exceeded runtime evidence [S1].

The resulting design requirements are to preserve input before interpretation, retain materially different alternatives, distinguish
facts from assumptions, freeze observable criteria, and preserve failed attempts and corrections. Compilation, runtime correctness,
experimental validity, and containment are separate claims. Later success never changes an earlier failure into success.

Offline rescoring showed why policy identity matters: exact matching and a plural-tolerant interpretation measured different questions
over the same immutable evidence. Candidate Zero work added contamination, data-partition, custody, and promotion boundaries. Its
central lesson is that a schema-valid declaration of independence cannot create actual independent control.

### 6.2 Last recorded results, not a new execution claim

The following results are reported in the inspected project records. This document review did not rerun binaries, reproduce the
experiments, rehash the complete evidence store, or qualify a local model.

- **DTE Core v1:** The 2026-10-23 completion report records `COMPLETE`, with 10/10 frozen acceptance criteria satisfied [S6].
- **Aichaos Rev 3:** The recorded Phi-4-mini run passed 14/14 acceptance criteria with 100% evidence completeness, 9/10 direct keyword
  recoveries, and 3/10 generated-C++ recoveries [S1, S6]. These are different measures from core acceptance.
- **Agent-model extension:** The 2026-10-27 progress marker records 81/81 conformance cases, 33 byte-verified handoff artifacts, and a
  native dependency audit of 34 files with zero violations [S10].
- **Local runtime inventory:** The source synthesis records an available local llama.cpp build and fourteen substantive GGUF candidates.
  Availability was inventoried historically; it was not re-probed for this document [S1].
- **Local model capability:** The recorded state remains `UNQUALIFIED`, with `production_eligible=false`. The sources do not demonstrate
  a completed repeated local role-qualification run or a qualified DTA/DTE-specific model [S1, S10].
- **Production custody:** The target group remains `UNFORMED`; the reviewed marker records no real production nominations, keys, or
  hardware bindings [S7, S10].

The frozen 39% historical prompt-processing estimate concerns an earlier interaction. It must remain separate from the 14/14 artifact
acceptance result, 10/10 core acceptance, and model recovery rates. None of these figures measures a general AIH or AITE reduction rate.

Some older status and index entries describe earlier incomplete machinery. The later dated completion and progress records refine
those historical implementation states; they do not erase the original records or demonstrate later model qualification.

### 6.3 Local execution and project boundaries

The intended deployed system uses local model artifacts, local inference, deterministic native DTE validation, and local evidence
storage [S1]. The statistical model remains a proposal generator outside the authority boundary.

The existing core and substantial native tools use C++/Qt6. The applicable RPA2 policy prohibits introducing Python source, runtime
dependencies, packaging, or training stacks under RPA2. A future training environment outside RPA2 would still need an admitted plan,
complete provenance, local execution where required, and compatible model artifacts before native evaluation.

nanochat remains a historically noted candidate training-pipeline reference in S1. This review did not reassess its current repository,
license, dependencies, model formats, or fitness. It is not a DTE component, qualification result, or prerequisite for the next rehearsal.

## 7. Proposed theory-development program

The following work develops the theory for AIH/AITE mitigation. It is proposed research, to be admitted through versioned contracts
before implementation. It preserves the existing core freeze and does not label new mechanisms as already implemented.

### 7.1 Specify the context-fidelity contract

Define the smallest useful records for source spans, requirement interpretations, premise classes, temporal authority, alternatives,
corrections, and verification obligations. State which transformations are lossless, which are interpretive, and which require review.

Acceptance evidence should include hand-worked examples in which materially relevant context is retained, irrelevant context is
excluded with a reason, corrections affect only their declared scope, and missing context remains explicit.

### 7.2 Specify inference and admission rules

Define each admitted operator's domain, preconditions, invariants, inference mode, evidence obligations, and rejection behavior.
State the assumptions under which a valid rule preserves a supported conclusion. Distinguish conditional proofs from empirical facts.

Specify evidence-state aggregation explicitly, including missing and conflicting evidence. Version any proposed treatment of analogy,
confidence, semantic entailment, or alternative ranking rather than extending core semantics through prose alone.

### 7.3 Specify traversal, termination, and coverage

Define canonical ordering, duplicate handling, cycle detection, deterministic work budgets, goal checks, and stopping states. Establish
termination for the bounded configuration. State whether a domain admits exhaustive coverage and what evidence would establish it.

Pruning and optimization must preserve the declared correctness contract. If a heuristic sacrifices completeness, record the tradeoff
and return a suitably limited conclusion. Bounded search should expose missing alternatives rather than conceal them.

### 7.4 Specify how claims reach the final output

Define a claim-to-evidence map and a requirement-to-behavior map. The first addresses AIH; the second addresses AITE. Require final
output construction to retain uncertainty, scope, failure attribution, and evidence identity from the selected decision path.

Test whether summaries preserve qualifications and whether the reported result corresponds to the evaluated requirement. A correct
internal record does not mitigate user-facing error if the final answer omits its material limitations.

### 7.5 Evaluate the theory's practical limits

Investigate which ambiguities can be detected mechanically, which require domain oracles, and which require human clarification.
Assess correlated reviewers, incomplete source material, stale context, incorrect premises, adversarial documents, and specification
errors in the validators themselves.

The central empirical hypothesis is that explicit context representation plus enforced evidence admission reduces unsupported claims
and intent distortion under controlled conditions. A deterministic engine can enforce a bad contract consistently, so this hypothesis
requires comparison evidence rather than architectural confidence alone.

## 8. Dependency-ordered implementation and research plan

### 8.1 Continue from the established project boundary

The general plan's early phases describe capabilities already developed in this project. Preserve the frozen core, reuse its admitted
validators, and extend through separate versioned profiles. Rebuilding the core from the beginning would disregard the project history.

The next recorded resume action is to select and content-address a bounded local rehearsal model set. Before launching a model, fix
the run manifest, resource envelope, synthetic case set, and evidence destination [S10]. This document does not execute that action.

### 8.2 Minimum local rehearsal

1. Freeze the rehearsal question, model-role criteria, repetition count, resource bounds, failure taxonomy, and stopping conditions.
2. Select a small representative subset of the historically inventoried local GGUF candidates; verify availability and hash actual bytes.
3. Record model, runtime executable, templates, prompts, tool registry, configuration, schemas, policies, cases, and evidence identities.
4. Implement the sixteen defined synthetic cases covering author, reviewer, custody-operator, and key/evidence-assistant roles [S1].
5. Validate evidence capture and classification with known-valid and known-invalid fixtures before model evaluation.
6. Run repeated local rehearsals, preserving every raw proposal, rejection, timeout, and failure.
7. Derive bounded model-role results through native validators; retain `UNKNOWN` and `CONFLICT` when warranted.
8. Attribute failures before deciding whether to change prompts, context handling, tools, the harness, controller logic, or a model.

The sixteen role cases serve the existing qualification plan. They do not automatically constitute a sufficient AIH/AITE benchmark.
Additional theory examples and mitigation tests require their own frozen profile and must not silently alter the existing case set.

### 8.3 AIH/AITE evaluation profile

Construct a bounded corpus with source-backed factual claims, ambiguous requirements, corrections, conflicting evidence, stale inputs,
unsupported premises, missing evidence, and final-report qualification checks. Include benign cases to measure unnecessary blocking.

Freeze expected outcomes, valid alternatives, oracle authority, permitted tools, adjudication rules, and dataset partitions before
candidate tuning. Unresolvable semantic cases remain explicitly unresolved rather than receiving convenient invented ground truth.

Compare the same candidate model under: a baseline workflow; documented DTA guidance; enforced DTE admission; and, where applicable,
DTE-governed tools. Keep tasks, evidence access, model identity, and resource conditions comparable. Record unavoidable differences.

Training, development, regression, and blind partitions have distinct purposes and authority. Repeated tuning against a held-out set
can compromise its independence. Independently controlled evaluation must be demonstrated through actual control and custody evidence.

### 8.4 Adaptation, promotion, and external custody

Use rehearsal evidence to decide whether model adaptation is justified. Exclude controller, transport, harness, and infrastructure
defects from model-training corrections. Preserve parent identity, dataset provenance, partitions, configuration, and rollback points.

The sources discuss local candidate development while the existing Candidate Zero path retains external readiness and custody gates.
Planning a local experiment does not waive those gates. Resolve any different training scope through an explicit versioned contract;
until then, the applicable frozen readiness and training criteria control.

Production activation requires real nominations, separately controlled identities and keys, independently authored and reviewed blind
cases, sealed custody, infrastructure evidence, native readiness evaluation, and permitted signed return evidence [S1, S7, S10].
Locally controlled fixtures or VMs may rehearse the workflow but do not create external independence.

Promotion requires predeclared improvement and no-regression gates over verified artifacts under an unchanged evaluation contract.
Candidate generation, successful training, and a promising score do not themselves authorize deployment or validator modification.

### 8.5 Applied domains and later optimization

After a stable bounded evaluation, consider small linear systems, constraints, deterministic planning, data transformations, and
protocol checking. Each needs a domain profile with its own operators, ground truth, evidence, limits, acceptance criteria, and transfer
boundary. Keyword recovery remains one test bed rather than the definition of DTA/DTE.

Then measure context selection, branch ordering, evidence acquisition, clarification frequency, tool selection, latency, memory,
storage, and energy. Historical difficulty identifies a research target; it is not evidence that a bottleneck must remain permanent.
An optimization must preserve the contract or be evaluated as a new version with a changed question.

## 9. Measurement and acceptance evidence

Report these dimensions separately before considering any aggregate score:

- **Task success:** tasks satisfying the frozen goal and required constraints divided by evaluable tasks; report unevaluable cases.
- **Unsupported-claim admission:** unsupported material claims presented as established divided by material claims assessed.
- **AITE incidence:** requirements or translation transitions adjudicated as materially distorted divided by those assessed.
- **Evidence completeness:** required evidence items present and integrity-valid divided by required items, with semantic support separate.
- **False admissions and false blocks:** admitted invalid candidates and blocked valid candidates against the declared reference set.
- **Human corrections and clarification burden:** required corrective interventions, useful clarifications, and unnecessary interruptions.
- **Deterministic replay:** canonical decision and transition equality for identical captured inputs and versions.
- **Model variability:** output and behavioral distributions across repeated inference, separate from deterministic replay.
- **Operational behavior:** failure attribution, recovery, rollback, latency, memory, storage, and other declared resource costs.

Freeze the counting unit and denominator. A model that makes fewer claims or abstains more can appear to improve a claim-based rate
while becoming less useful. Report absolute counts, task coverage, abstentions, and uncertainty alongside rates.

AIH and AITE labels may overlap, so their percentages are not automatically additive. Separate observable failure categories from
root-cause hypotheses. A transport failure may make a task unevaluable without establishing model inability.

No numerical improvement threshold is established by this document. Select and freeze thresholds before the relevant candidate
evaluation. A small synthetic result cannot establish broad real-world mitigation, universal correctness, or elimination of AIH/AITE.

## 10. Open questions, risks, and next decisions

The main unresolved theory question is how far semantic fidelity can be verified without replacing one unexamined interpretation
with another. Traceability, canonical records, and deterministic checks make this problem inspectable; they do not settle it alone.

The next decisions are the bounded problem family, minimum context and premise records, admitted inference operators, oracle authority,
rehearsal model set, repetition and resource envelope, mitigation benchmark, and improvement/no-regression thresholds.

Primary risks are specification error, missing alternatives, stale authority, correlated evaluation, contamination, excessive blocking,
scope inflation, and lost qualifications during reporting. Address them through versioned contracts, counterexamples, independent
evidence where required, explicit uncertainty, and comparisons under fixed conditions.

The grounded objective is to make a bounded decision process faithful enough to inspect, deterministic enough to replay, and evidenced
enough to evaluate and improve. The present record supports continued theory and rehearsal development; it does not yet demonstrate
that a particular local model or the combined system reduces AIH and AITE by a measured amount.

## 11. Source register and merge record

All sources below are local artifacts inspected for this synthesis. Source dates describe their own records. This document adds a
current interpretation and proposed research direction without modifying either merged original or the frozen implementation.

- **S1:** [DTA and DTE: Definitions and Current Plans][source1]. Recorded 2026-11-01 12:44 MST; updated 15:56 MST that day.
- **S2:** [DTA and DTE: General Definitions and Development Plan][source2]. Recorded 2026-11-01 12:47 MST.
- **S3:** [DTA Project Foundation][source3]. Created 2026-10-22 12:52 MDT; foundational theory and provisional formalization.
- **S4:** [DTA Terminology][source4]. Canonical vocabulary, inference modes, proposed `dtaopt` role, and theory-level statuses.
- **S5:** [DTE Core v1 Theory Contract][source5]. Frozen deterministic boundary and evidence semantics, 2026-10-23 21:38 MDT.
- **S6:** [DTE Core v1 Completion Report][source6]. Recorded 10/10 acceptance and bounded reproducibility evidence, 2026-10-23.
- **S7:** [External Custodian Post-Completion Checkpoint][source7]. Recorded implementation and production boundary, 2026-10-27.
- **S8:** [DTA/DTE Project Index][source8]. Artifact authority and historical development/evidence inventory.
- **S9:** [AIH/AITE architecture and AITE mapping][source9]. Local terminology and mitigation design, Chapter 4, 2026-10-10.
- **S10:** Current Progress Marker, created 2026-10-27 10:56 MDT, in the following directory and file:

```text
../extensions/agent-model/subprojects/external-custodian-target-group/misc/
EXTERNAL_CUSTODIAN_TARGET_GROUP_CURRENT_PROGRESS_MARKER_20261027_1056_MDT.md
```

The source hashes below identify the two merged input files as read during this review. They are provenance anchors, not assertions
that every historical implementation claim has been independently reproduced.

```text
S1 SHA-256: f05a2846f5cba3c52e0c7b29448e448ed64be850ea4603f0045227307949a59b
S2 SHA-256: c9c4bb3c7e95299238cecb96140d7612c6a60d107ec87837b4980f52f9bcabac
```

**Merge entry, 2026-11-12 15:05 MST:** Consolidated S1 and S2; grounded DTA theory in S3/S4 and DTE semantics in S5; reconciled earlier
generic phases with the recorded frozen core; preserved local-model and custody limitations; added an explicit AIH/AITE theory,
traceability, evaluation, and development program. Incorporated the user's emphasis on context as a substantive design requirement.

Future revisions should inspect controlling artifacts, update the affected definitions and decisions, distinguish new evidence from
proposed work, retain superseded history, record the effective time and source, and preserve approximately 140-character prose lines.

## 12. Signature and copyright

Samuel A. Gray III  
2026-11-12 15:10 MST

Copyright © 2026 Samuel A. Gray III. All rights reserved.

[source1]: DTA_DTE_Definitions_and_Current_Plans_2611011244MST.md
[source2]: ../../myLLC/AI/brilliance/misc/DTA_DTE_General_Definitions_and_Development_Plan_2611011247MST.md
[source3]: ../DTA_PROJECT_FOUNDATION_261022_1252_MDT.md
[source4]: ../docs/TERMINOLOGY.md
[source5]: ../docs/DTE_CORE_V1_THEORY_CONTRACT_20261023_2138_MDT.md
[source6]: ../docs/DTE_CORE_V1_COMPLETION_REPORT_20261023_2140_MDT.md
[source7]: ../docs/RESUME_CHECKPOINT_20261027_1029_MDT.md
[source8]: ../docs/PROJECT_INDEX.md
[source9]: ../../../global/misc/vibecoding-evo-20261010-1038.md
