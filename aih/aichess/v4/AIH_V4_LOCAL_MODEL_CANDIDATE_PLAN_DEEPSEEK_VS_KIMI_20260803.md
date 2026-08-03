# AIH v4 Local Model and Supporting Stack Candidate Plan

Date: 2026-08-03
Project: AIH v4 AIChess
Status: Specialized project-development plan

## Purpose

This plan records the current AI asst opinion on local AIH candidate models
and supporting local AI stack layers. DeepSeek and Kimi are current examples,
but this plan is not restricted to those two candidates.

AIH should evaluate both:

- Model candidates, such as DeepSeek distilled models, Kimi K3, Qwen, Gemma,
  Llama, Mistral, Phi, Granite, and other local-model families.
- Supporting stack candidates, such as Ollama alternatives, direct llama.cpp
  runtimes, local API servers, model managers, prompt routers, sandboxes,
  source-triage agents, and other local execution layers.

The two current examples should not be treated as equivalent. DeepSeek has
small distilled variants that are plausible local-agent test subjects. Kimi K3
is an extreme local-inference candidate whose laptop claim depends on
streaming a very large MoE checkpoint from disk.

## Standing Safety Rule

No Internet project, model runtime, repository, checkpoint, script, or external
stack should be downloaded, cloned, built, executed, or integrated into AIH
until it passes the AIH external-project intake gate.

Minimum gate:

- Identify official source URL.
- Record exact model tag, release, commit, or checksum.
- Review license and intended-use restrictions.
- Perform source/material threat scanning where applicable.
- Record scan result and human disposition.
- Define local execution limits before first run.

## Candidate Classes

AIH v4 should distinguish model candidates from supporting stack candidates.
A model may be acceptable while its preferred runtime is not, and a runtime may
be useful even when a particular model is not.

Model candidate classes:

- Small local distilled models that can run through an already trusted runtime.
- Medium local models that may be useful if memory and latency remain
  acceptable.
- Large or MoE models that require special runtime assumptions, large disk, or
  unusual streaming behavior.
- Cloud/API-only models used as benchmark competitors, not as private-project
  assistants unless separately approved.

Supporting stack candidate classes:

- Existing trusted local runtime: currently Ollama.
- Alternative local runtimes, including llama.cpp-derived runtimes or other
  local inference servers.
- Model/package managers that download or organize model files.
- Local agent frameworks used to perform source triage, scan-output
  summarization, or repetitive project inventory.
- Isolation layers, sandboxes, or containerized runners used to protect the
  host while evaluating untrusted material.

Each supporting stack candidate must pass the same intake gate before it can
be used to evaluate other candidates.

## DeepSeek Opinion

DeepSeek should be treated as a credible local-agent candidate only in its
small/distilled forms.

Recommended first target:

- `deepseek-r1:1.5b`

Possible second target after the first pass:

- `deepseek-r1:7b`

Not recommended on this laptop as ordinary AIH v4 targets:

- Full DeepSeek R1/V3/V4 class models.
- `deepseek-r1:32b`, `70b`, or `671b`.

Rationale:

DeepSeek-R1 includes distilled Qwen/Llama-derived models that can be run in
the same manner as ordinary Qwen or Llama models. Those models fit the AIH
local-agent testing pattern because they can be evaluated through an existing
local runtime such as Ollama after intake approval. They are measured in
single-digit gigabytes for the smallest practical variants, not terabytes.

AIH use:

- Good candidate for local chess-agent testing.
- Good candidate for comparing reasoning-style output against existing local
  models.
- Good candidate for local source triage only after it has itself passed the
  AIH intake gate.
- API DeepSeek may be useful as a benchmark competitor, but should not be used
  for sensitive/private project material without a separate decision.

## Kimi Opinion

Kimi K3 should be treated as a deferred extreme-inference research candidate,
not as an ordinary AIH v4 local-agent candidate.

Recommended status:

- Hold pending threat scan and feasibility review.

Rationale:

Kimi K3's laptop claim is technically plausible only because it uses a
Mixture-of-Experts architecture and a specialized streaming inference approach.
That is materially different from running a small distilled local model. Even
if the engine can run with low RAM, the claim still appears to require very
large fast disk capacity and extremely slow token generation compared with
ordinary local models.

Expected practical blockers:

- Very large disk requirement.
- Special-purpose Internet repository or engine must be reviewed first.
- Slow token rate likely makes it unsuitable as an AIH chess agent.
- High setup cost relative to expected AIH value.

AIH use:

- Not a default local-agent target.
- Not a practical v4 tournament participant at this stage.
- Possible later AIH v5 research item if the purpose is specifically to test
  extreme streaming inference.

## Comparison

| Candidate | Local feasibility | First AIH action | Recommended status |
| --- | --- | --- | --- |
| DeepSeek R1 distilled 1.5B | Plausible | Intake and scan, then Ollama test | Candidate |
| DeepSeek R1 distilled 7B | Possible but memory-sensitive | Test only after 1.5B | Conditional candidate |
| Full DeepSeek models | Not practical here | Do not pursue locally now | Defer |
| Kimi K3 streaming | Technically interesting but impractical | Threat-scan and disk/runtime feasibility only | Defer |
| Ollama alternatives | Potentially useful | Intake, scan, sandboxed static review | Candidate class |
| llama.cpp-style runtimes | Potentially useful | Intake, scan, build/run review | Candidate class |
| Local source-triage agents | Useful only after trust is established | Intake and calibration against Codex review | Conditional candidate class |

## Supporting Layer Evaluation Criteria

When evaluating an Ollama alternative or other supporting local AI stack,
AIH should record:

- What problem it solves that Ollama does not solve.
- Whether it can run already-approved local model files.
- Whether it requires new Internet downloads, package managers, containers, or
  build scripts.
- Whether it exposes a local API surface that AIH can call predictably.
- Whether it attempts outbound network access during normal operation.
- Whether model files, prompts, logs, or private project content leave the
  machine.
- Whether its license allows the intended AIH use.
- Whether it can be scanned and inspected without executing untrusted code.
- Whether it produces better cost, latency, reliability, or automation value
  than the current Ollama-backed path.

Supporting layer candidates should be accepted only if they improve AIH's
local capability without weakening local-site safety.

## AIH v4 Development Decision

For AIH v4, prioritize credible local distilled model evaluation and credible
supporting-stack improvements. DeepSeek distilled models are current model
candidates. Ollama remains the current trusted supporting layer, but
alternatives may be evaluated if they pass intake and offer a concrete
advantage.

Do not spend implementation time on Kimi K3 unless the project goal changes
from ordinary local-agent testing to extreme local-inference research.

The default v4 tournament work should continue to use local-only agents and
the current 50 maxply default. DeepSeek distilled models may be added as
future local contestants after intake approval. Other model families and
supporting layers may also be added after intake approval. Kimi should remain
outside the v4 tournament path unless a later review finds that it is safe,
practical, and fast enough to produce useful chess-agent data.

## External References to Verify at Intake Time

- DeepSeek-R1 official repository: `https://github.com/deepseek-ai/DeepSeek-R1`
- Ollama DeepSeek-R1 library page: `https://ollama.com/library/deepseek-r1`
- Kimi candidate repository/source: record exact source before review
- AIH v5 no-unscanned-download rule:
  `/home/sag/RPA2/myLLC/AI/brilliance/aih/v5/project_changes/AIH_V5_PROJECT_CHANGE_NO_UNSCANNED_INTERNET_DOWNLOADS_20260803.md`
