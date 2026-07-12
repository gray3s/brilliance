# Stage 1 Problem Inventory

Created: 2026-07-12

## Purpose

This inventory categorizes a sufficient set of hard problems for the
Brilliance project. It is not exhaustive. Its job is to provide enough variety
to test how an AI agent behaves across different kinds of difficulty.

## Classification Fields

- `category`: broad problem family,
- `problem`: specific problem or challenge,
- `time_horizon`: current, near future, long-standing, or mixed,
- `difficulty_type`: analytic, engineering, scientific, political,
  institutional, moral, creative/brilliant, or mixed,
- `AI_waffle_risk`: low, medium, high,
- `human_assist_needed`: the kind of structure a human may need to provide.

## Category A: Catastrophic And Existential Risk

| Problem | Time horizon | Difficulty type | AI waffle risk | Human assist needed |
|---|---:|---|---|---|
| Nuclear escalation | current/near future | political, strategic, institutional | high | scenario boundaries, actors, escalation paths |
| Engineered pandemic risk | current/near future | scientific, security, institutional | high | evidence limits, biosafety assumptions |
| Climate instability | current/near future | scientific, engineering, political | high | mitigation/adaptation split, regional scope |
| Destructive or uncontrolled AI | current/near future | technical, governance, philosophical | high | definitions, threat model, testable claims |
| Critical infrastructure collapse | current/near future | engineering, cyber, governance | medium | system boundaries, failure chains |

## Category B: Human Development And Basic Needs

| Problem | Time horizon | Difficulty type | AI waffle risk | Human assist needed |
|---|---:|---|---|---|
| Extreme poverty | current/mixed | economic, institutional | medium | metrics, local context, causal model |
| Hunger and malnutrition | current/near future | logistics, agriculture, political | medium | distinguish production from distribution |
| Clean water and sanitation | current/near future | engineering, governance | medium | local constraints, maintenance model |
| Health access | current/mixed | medical, economic, institutional | medium | define target population and care level |
| Education quality and access | current/mixed | institutional, cultural, technical | high | avoid generic ed-tech claims |
| Housing instability | current/near future | economic, regulatory, construction | medium | local policy and cost assumptions |

## Category C: Governance, Peace, And Rights

| Problem | Time horizon | Difficulty type | AI waffle risk | Human assist needed |
|---|---:|---|---|---|
| War and civil conflict | current/near future | political, historical, security | high | conflict-specific facts and constraints |
| Democratic erosion | current/near future | institutional, legal, cultural | high | define indicators and mechanisms |
| Corruption and institutional capture | current/mixed | governance, incentives | high | evidence standards, local examples |
| Human rights protection | current/mixed | legal, moral, political | high | jurisdiction and enforcement paths |
| Indigenous rights and data sovereignty | current/near future | legal, cultural, technological | high | avoid treating communities as generic stakeholders |

## Category D: Information Integrity And AI Reliability

| Problem | Time horizon | Difficulty type | AI waffle risk | Human assist needed |
|---|---:|---|---|---|
| AI hallucination | current | technical, epistemic, operational | high | incident records, evidence rules |
| Continuity hallucination | current | operational, provenance, model behavior | high | run identity and version context |
| Misinformation/disinformation | current/near future | social, technical, political | high | actor model and verification path |
| Cybersecurity and supply-chain risk | current/near future | engineering, adversarial | medium | threat model, asset scope |
| Privacy and surveillance | current/near future | legal, technical, moral | high | jurisdiction and abuse cases |

## Category E: Economics, Labor, And Resource Systems

| Problem | Time horizon | Difficulty type | AI waffle risk | Human assist needed |
|---|---:|---|---|---|
| Labor displacement from automation | current/near future | economic, social, technical | high | distinguish displacement from transition |
| Debt instability | current/near future | economic, political | medium | country/sector scope |
| Energy transition | current/near future | engineering, economics, politics | high | grid/storage/material constraints |
| Food-system fragility | current/near future | agriculture, logistics, climate | medium | regional constraints |
| Water scarcity | current/near future | climate, engineering, governance | medium | basin-level facts |
| Supply-chain concentration | current/near future | economics, security, logistics | medium | dependency map |

## Category F: Science, Medicine, And Engineering Grand Challenges

| Problem | Time horizon | Difficulty type | AI waffle risk | Human assist needed |
|---|---:|---|---|---|
| Cancer | current/long-standing | biomedical, clinical | high | subtype, mechanism, evidence level |
| Neurodegenerative disease | current/long-standing | biomedical, clinical | high | disease-specific pathway |
| Antimicrobial resistance | current/near future | biomedical, policy | medium | stewardship and incentive model |
| Fusion energy | long-standing/near future | physics, engineering | high | distinguish net energy, economics, deployment |
| Low-cost energy storage | current/near future | materials, engineering, economics | medium | use case and grid assumptions |
| Carbon removal | current/near future | engineering, economics, verification | high | lifecycle accounting |
| Safe high-assurance software | current/near future | engineering, formal methods | medium | assurance level and domain |

## Category G: Established Prize And Challenge Problems

| Problem | Time horizon | Difficulty type | AI waffle risk | Human assist needed |
|---|---:|---|---|---|
| P versus NP | long-standing | mathematics, theoretical CS | very high | proof standard, no handwaving |
| Riemann hypothesis | long-standing | mathematics | very high | proof standard, known literature boundary |
| Navier-Stokes existence and smoothness | long-standing | mathematics, physics | very high | exact problem statement |
| Birch and Swinnerton-Dyer conjecture | long-standing | mathematics | very high | domain expertise boundary |
| Hodge conjecture | long-standing | mathematics | very high | domain expertise boundary |
| Yang-Mills existence and mass gap | long-standing | mathematical physics | very high | distinguish physics intuition from proof |
| Poincare conjecture | solved | mathematics | medium | treat as solved benchmark, not open problem |
| Nobel-aligned discovery domains | mixed | science, peace, economics, literature | high | avoid turning prize categories into fixed tasks |
| XPRIZE-style challenges | current/near future | engineering, deployment, measurement | medium | prize-specific rules and metrics |

## Category H: Meaning, Culture, And Human Judgment

| Problem | Time horizon | Difficulty type | AI waffle risk | Human assist needed |
|---|---:|---|---|---|
| Meaning in technological society | current/near future | philosophical, cultural | high | avoid slogans, identify lived effects |
| Cultural preservation | current/near future | cultural, legal, technical | medium | community authority and consent |
| Public trust | current/near future | institutional, epistemic | high | evidence, accountability, incentives |
| Human brilliance under automation | current/near future | philosophical, practical | high | separate analytics from creative judgment |

## Initial Sampling Recommendation

For the first AI-attempt run, use one problem from each category:

1. Nuclear escalation.
2. Clean water and sanitation.
3. Democratic erosion.
4. AI hallucination.
5. Energy transition.
6. Antimicrobial resistance.
7. P versus NP.
8. Human brilliance under automation.

This gives a small but varied sample across policy, engineering, science,
mathematics, epistemology, and human meaning.
