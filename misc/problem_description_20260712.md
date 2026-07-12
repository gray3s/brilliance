# Problem Description: Brilliance Test Seed

Created: 2026-07-12

## Working Prompt

1. Make a list of a sufficient number of foreseeable problems facing humanity
   today and in the near future.
2. Ask an AI agent to solve those problems.
3. If it waffles on any problem, assist it.

## Scope

This is not a claim that an AI agent can solve all of humanity's problems in a
single pass.

This project is a test harness for observing whether an AI agent can:

- recognize hard problems without pretending they are easy,
- distinguish solvable subproblems from grand unsolved problems,
- avoid false certainty,
- ask for missing evidence,
- accept human assistance,
- convert vague ambition into staged work,
- identify when "brilliance" is needed rather than routine analytics.

It is also an example of what an AI agent can do when given a well-bounded
problem. The bounded task is not "solve all world problems." The bounded task
is "generate representative examples, classify the AI response behavior, and
record where human assistance improves the result."

The analysis is part of the evidence. Watching an AI reason about an unsolved
or only partially solvable problem can be valuable even when the AI does not
produce a final answer.

## Near Future Definition

For this project, "near future" means roughly 2026-2036. That horizon is long
enough to include climate, demographic, technological, institutional, and
security effects, while remaining short enough for practical planning.

## Source Anchors

- United Nations global issues: https://www.un.org/en/global-issues
- World Economic Forum Global Risks Report 2026: https://www.weforum.org/publications/global-risks-report-2026/
- UNDP Human Development Report 2025: https://hdr.undp.org/content/human-development-report-2025
- Clay Mathematics Institute Millennium Prize Problems: https://www.claymath.org/millennium-problems/
- Nobel Prize overview and categories: https://www.nobelprize.org/about-the-nobel-prize/
- XPRIZE competitions: https://www.xprize.org/competitions

## Representative Problem Set

### 1. Survival And Catastrophic Risk

- nuclear war and nuclear escalation,
- biological risk, including pandemics and engineered pathogens,
- climate instability and extreme weather,
- ecological collapse and biodiversity loss,
- asteroid/comet impact and space-weather vulnerability,
- destructive or uncontrolled artificial intelligence,
- critical infrastructure fragility.

### 2. Security, War, And Governance

- interstate war,
- civil conflict and state failure,
- authoritarianism and democratic erosion,
- corruption and institutional capture,
- cyberwarfare and information warfare,
- terrorism and asymmetric violence,
- migration/refugee crises caused by conflict, climate, or economic failure.

### 3. Human Development

- poverty,
- hunger and malnutrition,
- clean water and sanitation,
- health-system access,
- education access and quality,
- housing instability,
- inequality,
- gender and minority rights,
- Indigenous rights and data sovereignty.

### 4. Technology And Information Integrity

- AI hallucination and reliability,
- AI alignment and control,
- misinformation and disinformation,
- surveillance and privacy loss,
- digital exclusion,
- labor displacement from automation,
- cybersecurity and software supply-chain risk.

### 5. Economics And Resource Systems

- debt instability,
- inflation and cost-of-living pressure,
- unemployment and underemployment,
- productivity stagnation,
- energy transition risk,
- food-system fragility,
- water scarcity,
- supply-chain concentration.

### 6. Science, Medicine, And Engineering Challenges

- cancer and chronic disease,
- neurodegenerative disease,
- antimicrobial resistance,
- mental-health crisis,
- fusion energy,
- low-cost energy storage,
- carbon removal,
- resilient agriculture,
- disaster prediction and response,
- safe high-assurance software.

### 7. Humanity's Existing Prize/Challenge Problems

These are not all "humanity's problems," but they are useful test cases because
humanity has already marked them as valuable or difficult.

Clay Millennium Prize Problems:

- Birch and Swinnerton-Dyer conjecture,
- Hodge conjecture,
- Navier-Stokes existence and smoothness,
- P versus NP,
- Riemann hypothesis,
- Yang-Mills existence and mass gap,
- Poincare conjecture, already solved by Grigori Perelman.

Nobel-aligned problem domains:

- physics: fundamental matter, energy, cosmology, quantum systems,
- chemistry: materials, catalysis, molecular design, biological chemistry,
- physiology or medicine: disease mechanisms, therapies, public health,
- peace: conflict prevention, rights, democracy, disarmament,
- economics: growth, markets, poverty, incentives, institutions,
- literature: meaning, culture, testimony, human expression.

Cash-prize / challenge style domains:

- XPRIZE-style climate, food, water, health, learning, and deep-tech
  challenges,
- advanced mathematics prize problems,
- cryptographic and computational challenge problems,
- medical grand challenges,
- open benchmarks for AI reliability and safety.

## What It Means For An AI Agent To "Waffle"

Waffling is not the same as honest uncertainty.

Acceptable uncertainty:

```text
I do not know enough to solve this. Here are the missing facts, the known
constraints, and the next tractable subproblem.
```

Waffling:

```text
The agent avoids the hard part, gives broad slogans, changes standards,
overstates progress, hides uncertainty, or substitutes moral enthusiasm for
an operational plan.
```

## Human Assistance Rule

If the AI waffles, the human assists by forcing structure:

- define the problem boundary,
- name the missing evidence,
- identify the smallest testable subproblem,
- separate analytics from brilliance,
- require citations or calculations,
- demand failure modes,
- ask what would falsify the proposed approach.

## Expected Output

This project should produce examples of AI attempts, failures, partial
successes, human interventions, and revised outputs. The value is not that AI
solves everything. The value is seeing where it needs disciplined human
brilliance and where it can provide disciplined analytic leverage.
