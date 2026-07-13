# Class 3 Candidate Program Inventory

Created: 2026-07-13

## Purpose

Seed a preliminary list of academic and professional programs that can involve
formal training and therefore can be migrated into the Class 3 AIH structure.

This is not a marketed test, study, certification, or evaluation program. It is
a planning inventory for controlled hallucination-test design.

## Class 3 Structure

Use the common Class 3 structure:

```text
track -> level/license -> field/category -> difficulty band -> question
```

This is an iterative and somewhat circular model. The classification structure
will influence the tests, and test results will expose weaknesses in the
classification structure.

The goal is not a perfect ontology. The goal is a defensible structure that can
be revised when baseline runs expose egregious misclassifications.

For each candidate program, later work should define:

- source packet,
- program or license context,
- subcategory,
- intended difficulty band,
- expected answer form,
- grading rule,
- allowed references,
- disallowed assistance,
- expected RAIH/UAIH risk.

## Academic Program Candidates

### K-12 Bands

Candidate bands:

- K-3,
- 3-6,
- 6-9,
- 9-12.

Candidate categories:

- reading comprehension,
- arithmetic and quantitative reasoning,
- science concepts,
- history/social studies source interpretation,
- writing and grammar,
- digital literacy,
- boundary/refusal judgment.

### Undergraduate Majors

Candidate undergraduate fields:

- computer science,
- electrical engineering,
- mechanical engineering,
- civil engineering,
- chemical engineering,
- mathematics,
- statistics,
- physics,
- chemistry,
- biology,
- economics,
- accounting,
- finance,
- psychology,
- sociology,
- political science,
- philosophy,
- English/literature,
- history,
- education,
- nursing,
- public health,
- criminal justice,
- architecture.

Candidate academic categories:

- vocabulary and definitions,
- core concepts,
- procedural methods,
- quantitative methods,
- source interpretation,
- lab or field-method reasoning,
- case/problem application,
- boundary/refusal judgment.

### Master's And PhD Fields

Candidate graduate fields:

- computer science,
- artificial intelligence / machine learning,
- data science,
- statistics,
- electrical engineering,
- mechanical engineering,
- biomedical engineering,
- biology,
- chemistry,
- physics,
- mathematics,
- economics,
- business administration,
- public policy,
- public health,
- education,
- psychology,
- law-adjacent policy studies,
- philosophy,
- history.

Candidate graduate categories:

- advanced vocabulary and definitions,
- method selection,
- literature/source interpretation,
- quantitative analysis,
- experimental or study design,
- research ethics,
- limitation recognition,
- thesis/dissertation framing,
- boundary/refusal judgment.

## Professional License And Certification Candidates

These are background-test candidates only. The goal is not to reproduce or
replace actual licensing exams.

### Healthcare And Clinical

Candidate programs:

- nursing licensure background,
- physician assistant background,
- pharmacy technician background,
- emergency medical technician background,
- medical coding and billing,
- clinical laboratory technician,
- radiologic technologist background,
- public health certification background.

Candidate categories:

- safety,
- ethics,
- terminology,
- procedural sequence,
- patient/privacy rules,
- dosage or quantitative checks,
- case/situation judgment,
- documentation.

### Engineering, Construction, And Trades

Candidate programs:

- professional engineering background,
- engineer-in-training fundamentals,
- electrician licensing background,
- plumbing licensing background,
- HVAC certification background,
- welding certification background,
- construction safety,
- building inspection,
- machine-shop safety and procedure,
- auto mechanic certification background.

Candidate categories:

- safety/regulatory rules,
- tools and materials,
- diagrams and measurements,
- calculations,
- procedure order,
- code-reference interpretation,
- fault diagnosis,
- refusal when safety context is missing.

### Law, Finance, And Business

Candidate programs:

- paralegal certification background,
- legal ethics background,
- accounting certification background,
- tax-preparer background,
- securities/financial representative background,
- insurance licensing background,
- project-management certification background,
- human-resources certification background.

Candidate categories:

- law/ethics,
- fiduciary or client-duty concepts,
- terminology,
- quantitative methods,
- documentation,
- case/situation analysis,
- compliance boundary recognition,
- refusal when legal/financial advice would be inappropriate.

### Education, Public Safety, And Government

Candidate programs:

- teacher certification background,
- special education background,
- school administration background,
- law-enforcement academy background,
- corrections officer background,
- fire-service certification background,
- emergency management background,
- public administration background.

Candidate categories:

- safety,
- law/regulation,
- ethics,
- procedure,
- documentation,
- scenario judgment,
- communication,
- escalation/refusal boundaries.

### Information Technology And Cybersecurity

Candidate programs:

- help-desk / IT support certification background,
- networking certification background,
- Linux administration background,
- cloud practitioner background,
- cybersecurity analyst background,
- secure software development background,
- database administration background.

Candidate categories:

- vocabulary,
- command/procedure recognition,
- configuration interpretation,
- security concepts,
- incident triage,
- log/source interpretation,
- risk classification,
- refusal when the request would be unsafe.

## Difficulty Distribution

Each category can use a normal-curve-like distribution:

```text
easy recognition
routine application
mixed-context application
edge-case reasoning
expert-level discrimination
```

The distribution should be used to place individual questions within a category,
not to imply that the whole program has been evaluated.

## Baseline Retest Plan

After migration into structured Class 3 records:

1. Build a small source packet for each selected program/category.
2. Generate a short question set across the difficulty distribution.
3. Run the baseline stack serially against each member.
4. Preserve raw response, parsed answer, grading result, and RAIH/UAIH notes.
5. Flag egregious misclassifications:
   - wrong track,
   - wrong level,
   - wrong category,
   - unsupported confidence,
   - unsafe or out-of-scope answer,
   - correct answer with invalid reasoning,
   - grader/examiner acceptance of a bad answer.

The first purpose is classification sanity-checking, not broad program
evaluation.
