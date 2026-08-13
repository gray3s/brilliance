# AIH v5 Local Model and Supporting Stack Candidate Plan

Date: 2026-08-03
Project: AIH v5 AIChess
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

AIH v5 should distinguish model candidates from supporting stack candidates.
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

Not recommended on this laptop as ordinary AIH v5 targets:

- Full DeepSeek R1/V3/V5 class models.
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
not as an ordinary AIH v5 local-agent candidate.

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
- Not a practical v5 tournament participant at this stage.
- Possible later AIH v5 research item if the purpose is specifically to test
  extreme streaming inference.

## NVIDIA / Google Ollama-Compatible Agentic Model Update

Date: 2026-08-13
Status: Immediate research update; no model download or active-roster change

Several newer Ollama-compatible model families are credible additions to the
AIH local-agent candidate review list.

### NVIDIA Nemotron

Recommended first NVIDIA target:

- `nemotron-3-nano:4b`

Conditional larger NVIDIA target:

- `nemotron-3-nano:30b`

Deferred NVIDIA multimodal target:

- Nemotron 3 Nano Omni / `nemotron3`-class models, pending hardware and
  multimodal-harness review.

Rationale:

NVIDIA and Ollama both present Nemotron 3 Nano as an agentic model family with
tool/thinking support. The Ollama `nemotron-3-nano:4b` tag is the most plausible
first local AIH target because it is much smaller than the 30B and 70B Nemotron
variants. The 30B tag has a large local footprint and should be treated as a
workstation or cloud/offload candidate unless the test machine capacity changes.

AIH use:

- Good candidate for local liveness registration after normal external-model
  intake.
- Good candidate for comparing reasoning-style output against Qwen, Gemma,
  Llama, Phi, and DeepSeek distilled models.
- Good candidate for future tool-calling or coding-agent-adjacent AIH tests.
- Do not pull or run until intake approval, license review, and local capacity
  limits are recorded.

### Google Gemma 4

Recommended first Google target:

- `gemma4:e2b`

Conditional second Google target:

- `gemma4:e4b`

Rationale:

Google's Gemma-on-Ollama documentation and Ollama's Gemma 4 library page list
Gemma 4 as Ollama-compatible. Ollama describes Gemma 4 as supporting reasoning,
tools/thinking, coding, multimodal use, and agentic workflows. The E2B/E4B edge
variants are the most plausible laptop candidates, but they are larger than the
currently installed `gemma3:4b` file and may be memory-sensitive.

AIH use:

- Good candidate for a future local registration-only trial before tournament
  use.
- Prefer E2B before E4B on this laptop.
- Do not add to the default run roster until the model is pulled, liveness
  tested, and benchmark latency is known.

### Google FunctionGemma

Recommended specialized Google target:

- `functiongemma:270m`

Rationale:

FunctionGemma is a specialized Google/Gemma model for function calling, and
Ollama provides a direct `functiongemma` model page. It is very small and
tool-oriented, but it is not intended as a direct dialogue model. It is
therefore a better candidate for future AIH tool-call/function-call tests than
for the current chess move prompt.

Local compatibility note:

- Ollama's model page says FunctionGemma requires Ollama v0.13.5 or later.
- This machine's Ollama client reports version 0.32.0, so the client version
  appears high enough. The server was not running at the time of this note.

AIH use:

- Add to the future function-calling/tool-calling candidate list.
- Do not use as an ordinary chess agent without a specific adapter/prompt test,
  because it is specialized for function calls rather than normal conversation.

### Google Gemma 3n

Possible Google target:

- `gemma3n:e2b`
- `gemma3n:e4b`

Rationale:

Gemma 3n is Ollama-compatible and designed for efficient execution on everyday
devices. It is a plausible local-model candidate, but the stronger agentic
claim belongs to Gemma 4 and FunctionGemma. Treat Gemma 3n as a general local
model comparison target rather than the primary new agentic candidate.

## Comparison

| Candidate | Local feasibility | First AIH action | Recommended status |
| --- | --- | --- | --- |
| DeepSeek R1 distilled 1.5B | Plausible | Intake and scan, then Ollama test | Candidate |
| DeepSeek R1 distilled 7B | Possible but memory-sensitive | Test only after 1.5B | Conditional candidate |
| Full DeepSeek models | Not practical here | Do not pursue locally now | Defer |
| Kimi K3 streaming | Technically interesting but impractical | Threat-scan and disk/runtime feasibility only | Defer |
| NVIDIA `nemotron-3-nano:4b` | Installed locally 2026-08-13; smallest practical Nemotron tag for current hardware | AIH v5 registration/liveness and CSV analyzer inclusion | Active candidate |
| NVIDIA `nemotron-3-nano:30b` | Large / memory-sensitive | Capacity review before pull | Conditional candidate |
| Google `gemma4:e2b` | Plausible but not installed | Intake, pull/liveness test later | Candidate |
| Google `gemma4:e4b` | Possible but memory-sensitive | Test only after E2B | Conditional candidate |
| Google `functiongemma:270m` | Plausible specialized tool model | Tool-call adapter review before AIH use | Specialized candidate |
| Google `gemma3n:e2b/e4b` | Possible local model | General comparison review | Conditional candidate |
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

## AIH v5 Development Decision

For AIH v5, prioritize credible local distilled model evaluation and credible
supporting-stack improvements. DeepSeek distilled models are current model
candidates. Ollama remains the current trusted supporting layer, but
alternatives may be evaluated if they pass intake and offer a concrete
advantage.

Do not spend implementation time on Kimi K3 unless the project goal changes
from ordinary local-agent testing to extreme local-inference research.

The default v5 tournament work should continue to use local-only agents and
the current 50 maxply default. DeepSeek distilled models may be added as
future local contestants after intake approval. Other model families and
supporting layers may also be added after intake approval. Kimi should remain
outside the v5 tournament path unless a later review finds that it is safe,
practical, and fast enough to produce useful chess-agent data.

## External References to Verify at Intake Time

- DeepSeek-R1 official repository: `https://github.com/deepseek-ai/DeepSeek-R1`
- Ollama DeepSeek-R1 library page: `https://ollama.com/library/deepseek-r1`
- Kimi candidate repository/source: record exact source before review
- NVIDIA Nemotron developer page: `https://developer.nvidia.com/topics/ai/nemotron`
- Ollama Nemotron 3 Nano library page: `https://ollama.com/library/nemotron-3-nano`
- Ollama Nemotron 3 Nano tags: `https://ollama.com/library/nemotron-3-nano/tags`
- NVIDIA Nemotron 3 Nano Omni inference/deployment:
  `https://docs.nvidia.com/nemotron/nightly/nemotron/omni3/inference.html`
- Google Gemma Ollama integration:
  `https://ai.google.dev/gemma/docs/integrations/ollama`
- Ollama Gemma 4 library page: `https://ollama.com/library/gemma4`
- Ollama FunctionGemma library page: `https://ollama.com/library/functiongemma`
- Google FunctionGemma page:
  `https://deepmind.google/models/gemma/functiongemma/`
- Ollama Gemma 3n library page: `https://ollama.com/library/gemma3n`
- AIH v5 no-unscanned-download rule:
  `/home/sag/RPA2/myLLC/AI/brilliance/aih/v5/project_changes/AIH_V5_PROJECT_CHANGE_NO_UNSCANNED_INTERNET_DOWNLOADS_20260803.md`
