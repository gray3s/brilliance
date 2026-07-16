# AIH Class Taxonomy Breakdown

Created: 20260715_1931MDT

## Purpose

This note records the current breakdown rule for Class 2, Class 3, and K-PhD AIH tests. It controls how new test definitions, inventories, manifests, and result records should be categorized.

## Class 2 Breakdown

Class 2 tests should be broken down by subject matter or subject field.

Class 2 remains the provenance/workflow-history hallucination class, but each Class 2 test should carry a subject-field tag so failures can be compared across domains.

Recommended Class 2 metadata:

```text
class = class_2_provenance_workflow
subject_field = <field>
failure_mode = <provenance | continuity | temporal decision | attribution | completeness | workflow disturbance | other>
source_context = <synthetic | local-project | public-safe incident | other>
```

Initial subject-field examples:

- legal/evidence workflow
- software/project workflow
- academic/research workflow
- medical/health-record workflow
- finance/accounting workflow
- publication/media workflow
- repository/version-control workflow
- personal-history/provenance workflow
- information technology tools and support workflow

Starter IT/support Class 2 candidate tests are recorded here:

```text
v1/AIH/AIhistory/v1/class2_it_tools_procedures_candidate_tests_20260715_1939MDT.md
```

AIChess-related tests can also become Class 2 tests when the target failure is
workflow/provenance rather than chess legality. Examples include false claims
about which agent selected a move, which referee verified it, whether an
artifact was written, whether a board state was preserved, or whether a
multi-agent run completed.

## Class 3 Breakdown

Class 3 tests should be broken down by the academic class that supplied the source material.

Class 3 remains the source-bound knowledge/education-ladder hallucination class, but a Class 3 test should identify the source academic class or course packet before it identifies broader level or field labels.

Recommended Class 3 metadata:

```text
class = class_3_source_bound_knowledge
source_academic_class = <institution/course/title/source-packet>
source_material_id = <packet/file/url/version>
academic_field = <field>
difficulty_band = <recognition | routine application | mixed-context application | edge-case reasoning | expert discrimination>
question_id = <id>
```

Examples:

- MIT OCW 6.036, Introduction to Machine Learning
- MIT OCW 6.034, Artificial Intelligence
- Stanford CS229
- Stanford CS221
- a local source packet derived from one or more academic classes, if the component classes are recorded.

AIChess-related tests can become Class 3 tests when the target is source-bound
chess knowledge or academic/instructional chess material, such as a defined
chess course packet, annotated game study, opening/endgame theory source, or
K-PhD-style chess education material.

## AIChess Class Movement Rule

Do not classify a test by topic alone. Classify it by the failure mode:

```text
Class 1 = chess legality, board-state fidelity, rule-following, timing, referee validation
Class 2 = workflow/provenance claims about chess runs, agents, logs, verification, artifacts
Class 3 = source-bound chess knowledge from defined academic/instructional material
```

AIChess role-count convention:

```text
Class 1 AIchess core = one board, one player agent per side, three referees
Class 2 AIchess core = two boards, one player agent per side per board, one referee per board
Class 3 AIchess core = four boards, one player agent per side per board, four referees total
```

Near-term AIChess implementations should use board assignments, player agents,
and referee teams only. Other role types are deferred until the board-runner
package is stable.

## K-PhD Breakdown

K-PhD tests should be broken down by academic year for K-12 material and by college course-year level for bachelor's and master's-level material.

K-12 material:

```text
K
grade_01
grade_02
...
grade_12
```

Bachelor's and master's-level course material:

```text
1xx = first-year college course level
2xx = second-year college course level
3xx = third-year college course level
4xx = fourth-year college course level
```

Ph.D. work:

```text
grad_year_3
grad_year_4
```

Ph.D. material is generalized to third- and fourth-year graduate work unless a later test definition has a stronger reason to use a more specific dissertation-stage label.

Recommended K-PhD metadata:

```text
class = class_3_source_bound_knowledge
test_family = k-phd
education_band = <k12 | bachelors | masters | phd>
academic_year = <K | grade_01..grade_12 | 1xx | 2xx | 3xx | 4xx | grad_year_3 | grad_year_4>
source_academic_class = <course/source packet>
academic_field = <field>
question_id = <id>
```

## Working Rule

Public summaries may still use short labels such as `Class 2 AIH test`, `Class 3 AIH test`, or `K-PhD test`. Internal records should carry the detailed subject field, source academic class, academic year/course-year level, source packet, and grading rule.
