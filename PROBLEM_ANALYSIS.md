# Brilliance Problem Analysis

Created: 2026-07-12

## Problem

AI is increasingly useful at organizing knowledge, generating plans, and
analyzing complex questions. But the important test is not whether an AI agent
can claim to solve every hard problem. The important test is whether it can
analyze hard problems without hallucinating certainty.

## Working Question

What happens when an AI agent is given a well-bounded version of an
unreasonably large request?

The seed request is:

1. list a sufficient number of foreseeable problems facing humanity today and
   in the near future; try to cover the entire scope of all known problems, as
   the detailed list would be far too long and unwieldy,
2. ask an AI agent to solve those problems,
3. if the agent waffles, assist it.

## Analysis

The useful result may not be a solution. The useful result may be observing
how the AI analyzes the problem without actually solving it.

That observation can show whether the agent:

- distinguishes routine analytics from genuine brilliance,
- identifies missing evidence,
- avoids false certainty,
- separates known methods from unsolved breakthroughs,
- asks for better boundaries,
- accepts human assistance,
- produces testable next steps,
- admits when domain expertise or formal proof is required.

## Bounded Claim

This project does not claim that AI can solve humanity's major problems in one
pass.

It claims that, given a well-bounded problem, AI can generate useful source
material, expose weak reasoning, and help structure inquiry, even when it does
not solve the underlying problem.

## Architecture Options Under Test

The Brilliance project should treat implementation choices as test variables,
not as assumed improvements.

Candidate variables include:

- default unmodified repository/software behavior before tuning;
- agentic-AI top layers such as LangGraph, CrewAI, AutoGen, OpenHands,
  LlamaIndex/Agno, or HybridAI-native implementations;
- middle layers such as direct Ollama API, Ollama OpenAI-compatible `/v1`,
  LiteLLM, TinyProxy, custom AIH transport shims, and logging paths;
- retrieval/cache/memory layers such as source-packet only, RAG, tuned RAG,
  reranking, vector stores, semantic cache, KV/v-cache behavior, and memory
  cache;
- bottom/supporting layers such as Ollama, vLLM, and Hugging Face serving
  paths;
- model families and versions, including Qwen variants;
- Codex, Claude/Anthropic, and other cloud-agent comparison baselines,
  recorded by date and observed product surface;
- future hardware environment changes such as more CPU cores, more RAM, GPU,
  external GPU modules, Thunderbolt/advanced-I/O paths, faster storage, and
  power/thermal profiles;
- course/source-packet designs for BS-level and MS/MA-level knowledge-track
  tests.

Recommendations found in research papers, documentation, LinkedIn posts, or
other public discussion should be converted into bounded AIH test variables.
They should not be treated as established improvements until the same AIH test
has been run, parsed, scored, and compared.

Local AI/ML optimization research should be a HybridAI subproject. Its role is
to study RAG, retrieval, cache, memory, serving, quantization, and classical-ML
optimization options, then feed candidate variables into AIH tests.

One important research pattern is fixed-hardware software-switch testing. The
project should hold hardware constant where possible, change one runtime or
configuration switch at a time, and record whether the AIH result changes.

The default baseline comes first. Before modifying repository software or
enabling RAG, cache, memory, serving, or model-tuning options, the project
should record an unmodified baseline run for the same AIH test.

Codex, Claude/Anthropic, and other cloud agentic-AI stacks should be comparison
baselines, not fixed reference points. Because they can change over time, the
project should record date, product surface, model identity where available,
tool access, and prompt envelope for each cloud run.

Hardware upgrades are a future environment track, not an immediate prerequisite.
The project should first find what stack configurations run in the current
Brilliance test environment. If later tests move to a higher-core, higher-RAM,
GPU, external-GPU, or advanced-I/O system, that hardware change should be
recorded as part of the test condition.

## Future Projects To Be Named

Some local audio and transcript material may become future Brilliance
subprojects after review. That material should remain local until the source
audio, generated transcripts, transcription accuracy, publication boundary, and
research framing have been checked.

## Supporting Documents

- Project goals: https://github.com/gray3s/brilliance/blob/main/PROJECT_GOALS.md
- Problem description: https://github.com/gray3s/brilliance/blob/main/misc/problem_description_20260712.md
- Stage 1 problem inventory: https://github.com/gray3s/brilliance/blob/main/misc/stage1_problem_inventory_20260712.md
- Stage 2 solution-attempt taxonomy: https://github.com/gray3s/brilliance/blob/main/misc/stage2_solution_attempt_taxonomy_20260712.md
- Project development plan: https://github.com/gray3s/brilliance/blob/main/docs/project_development_plan_20260712.md
- v1 direction: https://github.com/gray3s/brilliance/blob/main/docs/v1_direction_20260712.md
- Staged test plan: https://github.com/gray3s/brilliance/blob/main/test_plans/staged_test_plan_20260712.md

## Local Addendum Draft: Audio-Derived Delta, 20260712

Publication status:

This section is a local working addendum derived from three phone WAV files.
It is not ready for publication and should not be pushed until reviewed.

Audio sources copied locally:

- `misc/audio_sources_20260712/AIDC-PfezBillionsFromCybercoin-20260704-163214.wav`
- `misc/audio_sources_20260712/recording-20260712-142048.wav`
- `misc/audio_sources_20260712/recording-20260712-142315.wav`

Transcription caveat:

The draft transcription was produced locally with `faster-whisper` and should
be manually reviewed before it is treated as authoritative.

### Delta Against Existing Analysis

Already covered by existing `brilliance/v1` notes:

- use cash-reward / prize problems as the next Brilliance test family,
- consider Clay Millennium problems, XPRIZE-style challenges, and other formal
  prize problems,
- exclude wanted-person / fugitive-capture rewards as implementation targets,
- treat such law-enforcement-adjacent reward problems only as a thought
  experiment if discussed at all.

New or underdeveloped material from the audio:

1. AI data-center economics as a Brilliance problem-analysis candidate.

The audio raises a separate research question:

```text
What is actually in an AI data center, why does building one matter, and what
is an AI data center expected to produce for its investors or owners?
```

This should become a candidate problem-analysis topic because it is a concrete
case where AI, infrastructure, capital expenditure, power demand, and market
narrative meet.

Possible Brilliance framing:

```text
AI_data_center_driver_question =
determine whether AI data-center investment is primarily justified by genuine
AI workload demand, speculative infrastructure positioning, cryptocurrency or
compute-adjacent revenue, financial engineering, or some mixture of these
drivers
```

2. Crypto / "cyber coin" as a possible hidden or competing driver.

The audio asks whether some AI data-center buildout could be entangled with
cryptocurrency economics rather than purely AI economics.

Working research question:

```text
AI_vs_crypto_capex_question =
when large data-center investments are justified publicly as AI infrastructure,
what evidence distinguishes AI workload demand from cryptocurrency mining,
token speculation, compute arbitrage, or other non-AI financial drivers?
```

This should be handled carefully. It is a hypothesis-generating question, not
a conclusion. Any future analysis would need current, cited evidence about:

- data-center customers,
- power usage,
- GPU/ASIC hardware mix,
- reported revenue streams,
- public disclosures,
- crypto-market conditions,
- ownership and related-party transactions,
- regulatory filings where applicable.

3. Disclosure / provenance standard for large financial claims.

The audio asks how a public figure could allegedly make very large sums from
crypto-related activity and where the disclosure forms or evidence would be.

Generalized Brilliance rule:

```text
large_financial_claim_provenance_rule =
when a large financial claim is made about a public actor, infrastructure
project, or technology boom, require the specific disclosure record, filing,
transaction evidence, counterparty evidence, and market context before treating
the claim as established
```

This is relevant to hallucination analysis because AI may be tempted to repeat
public narratives or rumors without tracing the underlying evidence.

4. Prioritization rule for Brilliance v1.

The audio reinforces that the primary issue is now the prize/cash-reward
problem path. Other issues can continue as secondary work, but they should not
consume the main project effort.

Working rule:

```text
brilliance_v1_priority_rule =
focus primary effort on the selected prize/cash-reward problem family; allow
secondary issues to be worked only when they support the main analysis or use
otherwise available time without distracting from the main path
```

5. Secondary issue monitoring.

The audio notes that secondary issues do not appear likely to become primary
issues immediately, while acknowledging that if they did, the project would be
in more serious trouble.

Working rule:

```text
secondary_issue_monitoring_rule =
keep secondary risks visible, but do not promote them to primary project
status unless new evidence shows that they materially threaten or redefine the
main project path
```

### Resume Note

Next time `brilliance/v1` is reopened, manually review the audio
transcription, decide whether this local addendum belongs in the published
problem-analysis file, and only then commit/push any reviewed version.
