Copyright © 2026 Samuel A. Gray III. All rights reserved.
# DTA/DTE Weekly Engineering Summary

Last modified: 2026-10-23 11:45 MDT



Reporting period: 2026-10-22 through 2026-10-23 MDT  
Privacy scope: technical project information only; personal information,
personal filesystem paths, account identifiers, and private correspondence are
excluded.

## Executive summary

DTA/DTE progressed from an early deterministic context-translation design to
a tested native evidence, conformance, and agent-control system. The current
verified baseline includes an end-to-end experiment, immutable evidence,
offline rescoring, a unified conformance entry point, canonical decision-node
hashing, a bounded persistent controller, and a candidate-promotion gate.

The DTE core is not yet declared complete. The latest unified suite passes, but
crash-safe controller resume, a versioned action-adapter registry, remaining
decision-node boundary fixtures, and the final ten-item completion audit remain.

## Experimental baseline

The verified Rev 3 keyword-recovery experiment passed all 14 precommitted
implementation criteria with complete evidence retention.

| Measure | Result |
|---|---:|
| Direct recovery | 9/10 |
| Generated-code recovery | 3/10 |
| Strict combined recovery | 12/20 |
| Evidence completeness | 100% |
| API failures | 0 |
| Compiler-infrastructure failures | 0 |
| Sandbox/runtime failures | 0 |

The historical assistant-session evaluation remains frozen at 39 percent,
`PARTIAL_FAILURE`. Later implementation success did not rewrite it.

## Evidence and rescoring

- A native offline rescoring tool hashed 171 preserved artifacts and reproduced
  the strict 12/20 result with zero unreadable cases.
- A separately named singular/plural policy produced 13/20 without modifying
  the strict score or original evidence.
- Re-evaluations write outside immutable experiment roots.
- Actual evidence and candidate artifacts are now checked against declared
  SHA-256 values and byte counts.

## Contracts and documentation

- Created a normative/historical project index.
- Audited the ten-item DTE core completion contract.
- Added versioned schemas for context, evidence, evaluation policy,
  corrections, domain transfer, and cross-record bundles.
- Created a schema-to-validator rule matrix and a native rule inventory.
- The latest rule inventory parsed ten schema files and extracted 548
  validation declarations with zero errors, including the new action-adapter
  registry schema.
- Defined an ordered integration plan for the pre-existing session,
  decision-node, and model-candidate schemas.

## Native conformance development

The core fixture suite expanded incrementally while preserving earlier runs.
Coverage now includes:

- four-state evaluation behavior;
- malformed and missing evidence;
- cross-record reference resolution;
- duplicate identities and unresolved references;
- actual content hashes and sizes;
- top-level and nested property restrictions;
- date-time, enum, uniqueness, integer, and cardinality boundaries;
- correction targets and domain gate policies;
- session-evaluation negative cases; and
- canonical decision-node hash mutations.

The latest unified run passes 48/48 core fixtures.

## Native agent controller

A minimal persistent controller now:

1. loads a frozen objective and ordered bounded actions;
2. executes authorized actions without another user prompt;
3. confines outputs to an authorized root;
4. verifies resulting artifact hashes;
5. appends hash-chained transition records; and
6. terminates in a deterministic state.

Verified terminal states and exit codes are:

| State | Exit code |
|---|---:|
| `COMPLETE` | 0 |
| `BLOCKED` | 1 |
| `BUDGET_EXHAUSTED` | 3 |
| `PAUSED` | 4 |

The positive fixture executes and verifies two actions autonomously. Negative
fixtures cover an unauthorized path and an exhausted step bound. `PAUSED`
requires an explicit plan field and cannot arise from controller discretion.

A frozen `write-evidence` adapter registry now declares its required action
fields, deterministic behavior, evidence contract, and authority restrictions.
Its SHA-256 is bound into the two-action plan, and each action declares adapter
version `1`. Compatibility conformance passes; controller-side registry loading
and unknown-adapter rejection remain next.

## Canonical decision-node identity

Decision-node IDs are recomputed from a documented semantic projection using
compact UTF-8 JSON and SHA-256. Semantic fields are included; graph placement,
traversal depth, runtime resource state, and the output ID itself are excluded.

Tests establish that:

- the baseline vector reproduces its declared ID;
- changing only excluded trace data preserves the ID; and
- changing semantic goal content invalidates the old ID.

## Candidate promotion boundary

The native promotion evaluator derives outcomes rather than trusting a
candidate's claimed decision. The unified suite currently covers six cases:

- `PROMOTE` when all frozen gates pass;
- `REJECT` for disqualifying evidence;
- `INCOMPLETE` for infrastructure or unreadable-artifact conditions;
- `CONFLICT` for materially mixed metric evidence;
- rejection of a claimed/derived decision mismatch; and
- rejection of artifact-hash tampering.

Every promoted candidate receives rollback metadata identifying the hashed
parent artifact and rollback trigger.

## Failure accountability

Failed runs and repairs were retained rather than overwritten. Material
examples include resource-limiter misclassification, a missing process start,
incorrect controller fixture hashes, compiler warnings corrected before final
acceptance, and atomic patch-context failures. Successful later runs do not
erase those records.

## Latest verified aggregate state

Authoritative run: `output/dte-unified-conformance/20261023_candidate_conflict_rollback_rev1`

| Component | Result |
|---|---:|
| Aggregate verdict | `PASS` |
| Core fixtures | 48/48 |
| Controller stopping states | 4/4 |
| Candidate cases | 6/6 |
| Session negative fixtures | 4/4 |
| Historical session reproduction | 39%, `PARTIAL_FAILURE` |
| Schema inventory | 10 files, 548 declarations, 0 errors |

## Remaining work

1. Enforce the frozen action-adapter registry in the native controller and add
   unknown-adapter rejection.
2. Implement crash-safe controller resume from a verified transition chain.
3. Complete isolated decision-node negative fixtures.
4. Reconcile the schema rule inventory with all native validators.
5. Run the clean-build, reproducibility, no-Python, and documentation audits.
6. Apply a precommitted final acceptance standard to all ten DTE core contract
   items.
7. Declare DTE core complete only if every required item is directly evidenced.

## Current decision

Continue implementation from the latest checkpoint. Do not freeze or declare
the DTE core complete solely because the current unified suite passes.
