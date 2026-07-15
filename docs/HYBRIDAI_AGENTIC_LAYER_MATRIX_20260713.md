# HybridAI Agentic Layer Matrix

Created: 2026-07-13

## Purpose

Define the first HybridAI stack-comparison axis as the agentic AI layer.

TinyProxy and Ollama should become separate infrastructure dimensions later.
This document starts with open-source agentic AI layers that can plausibly run
through Ollama directly or through an OpenAI-compatible local endpoint such as a
LiteLLM proxy.

## Working Compatibility Rule

For this matrix, "compatible with TinyProxy and Ollama" means:

1. The agent layer can call an Ollama-hosted model directly, or
2. The agent layer can call an OpenAI-compatible endpoint that is backed by
   Ollama, or
3. The agent layer can call a local HTTP model gateway that can be placed behind
   TinyProxy for traffic control and logging.

This does not mean every feature of the framework will work equally well with
every local model. Tool calling, JSON output, context length, latency, and
determinism must be tested separately.

## Initial Agentic Layer Candidates

| Agentic layer | Open-source role | Ollama path | TinyProxy path | Initial priority | Notes |
| --- | --- | --- | --- | --- | --- |
| HybridAI native v1 | Existing local implementation under test | Existing TinyProxy/Ollama stack | Native target | P0 | Baseline implementation already present in the local workbench. |
| LangGraph / LangChain agents | Stateful graph-based agent workflows | Direct ChatOllama integration | HTTP endpoint can be proxied | P0 | Strong candidate for reproducible bounded test harnesses and adapters. |
| CrewAI | Multi-agent crews and task workflows | CrewAI LLM config supports `ollama/...` via LiteLLM | LiteLLM/Ollama endpoint can be proxied | P0 | Good candidate for role/task coordination tests. |
| AutoGen | Conversable multi-agent framework | AutoGen docs show local Ollama through LiteLLM/OpenAI-compatible client | LiteLLM endpoint can be proxied | P0 | Good candidate for peer-agent and referee/worker patterns. |
| OpenHands | Software-development agent platform | Model routing can use local/OpenAI-compatible backends; verify exact current Ollama config before implementation | HTTP services can be proxied | P1 | Strong for software-task AIH tests, but heavier than first chess probes. |
| LlamaIndex agents | Data/RAG-oriented agent workflows | Ollama LLM integrations exist; verify current agent API before implementation | HTTP endpoint can be proxied | P1 | Useful for k-phd/source-boundary tests more than chess. |
| Semantic Kernel | Planner/agent orchestration SDK | Local/OpenAI-compatible paths likely; verify exact Python/C# Ollama support before implementation | HTTP endpoint can be proxied | P2 | Useful if Microsoft ecosystem or planner comparisons matter. |
| MetaGPT | Multi-agent software-team workflow | May support local/open-source models through configurable LLM providers; verify Ollama path before implementation | HTTP endpoint may be proxied | P2 | More specialized and heavier; not first target. |
| AutoGPT / Forge-style agents | Autonomous task-loop agents | Usually model-provider configurable; verify active project state and Ollama path before implementation | HTTP endpoint may be proxied | P2 | Historically important, but less controlled for bounded AIH runs. |
| Agno / phidata-style agents | Lightweight agent framework | Ollama support appears plausible; verify current API before implementation | HTTP endpoint can be proxied | P2 | Candidate for simple tool-agent comparisons after P0 adapters. |

## First Implementation Set

Start with four agent-layer variants:

1. HybridAI native v1
2. LangGraph/LangChain adapter
3. CrewAI adapter
4. AutoGen adapter

These give a useful first spread:

- native local stack,
- graph/state-machine agent,
- crew/task-based multi-agent system,
- conversable multi-agent system.

## Middle Layer Options

The middle layer should normalize model access, preserve evidence, and make
agent-layer comparisons less dependent on one backend.

| Middle layer option | Role | Strength | Concern | Priority |
| --- | --- | --- | --- | --- |
| Direct Ollama API | Agent calls Ollama directly on localhost | Simple baseline with few moving parts | Less routing, policy, and normalization | P0 baseline |
| Ollama OpenAI-compatible `/v1` API | Agent uses OpenAI client against Ollama | Broad agent-framework compatibility | Compatibility is partial and must be tested per feature | P0 |
| LiteLLM proxy | OpenAI-compatible gateway in front of Ollama or other backends | Normalizes providers, supports proxy mode, routing, JSON/tool-call paths | Adds another moving part and config surface | P0 |
| TinyProxy | HTTP proxy for traffic control/logging boundaries | Already part of HybridAI concept; useful for network mediation | Not an LLM gateway by itself; needs surrounding capture policy | P0 |
| Custom AIH transport shim | Minimal local gateway owned by the test harness | Can enforce exact logging, timing, request IDs, schemas | Must be built and maintained | P1 |
| OpenTelemetry / request logger sidecar | Observability layer | Useful for timing, errors, and replay metadata | Does not solve model compatibility alone | P1 |
| Nginx / Caddy reverse proxy | General HTTP reverse proxy | Stable, common, configurable | Less LLM-aware than LiteLLM | P2 |

Recommended first middle-layer path:

```text
agent adapter -> AIH transport shim -> LiteLLM proxy -> Ollama
```

Minimal first path:

```text
agent adapter -> Ollama OpenAI-compatible /v1 endpoint
```

HybridAI-native path:

```text
HybridAI v1 -> TinyProxy-controlled path -> Ollama
```

## Bottom Layer Options

The bottom layer is the model-serving/runtime layer. It should be treated as a
separate dimension from the agentic layer.

| Bottom layer option | Role | Strength | Concern | Priority |
| --- | --- | --- | --- | --- |
| Ollama | Local model manager and HTTP server | Current baseline; easy model pull/run loop; OpenAI-compatible API exists | Feature support depends on model and endpoint path | P0 |
| llama.cpp server | Direct GGUF runtime and server | Low-level control; OpenAI-compatible chat/completions/routes; strong for constrained local tests | More manual model/config management | P1 |
| vLLM | High-throughput model serving | OpenAI-compatible serving, batching, larger deployment path | Heavier GPU/server assumptions than first local probes | P1/P2 |
| LocalAI | OpenAI-compatible local AI server | Designed as a local OpenAI-compatible drop-in | Verify current model/tool-call behavior before using in AIH | P2 |
| LM Studio local server | Desktop/local model server | Easy interactive local testing | Licensing/open-source status and reproducibility need care | P2 |
| Hugging Face TGI | Production inference server | Useful for hosted/self-hosted HF models | Heavier than needed for first local bounded tests | P2 |
| SGLang | Structured/high-performance serving | Strong for structured generation experiments | Heavier; defer until baseline matrix exists | P2 |

Recommended first bottom-layer path:

```text
Ollama with one fixed model, fixed tag, fixed parameters, and recorded hardware.
```

Then compare:

```text
Ollama vs llama.cpp server
```

Only after that should high-throughput servers such as vLLM become important.

## Stack Dimensions

The eventual HybridAI comparison matrix should have separate dimensions:

1. Agentic layer: HybridAI native, LangGraph, CrewAI, AutoGen, etc.
2. Middle layer: direct Ollama, Ollama `/v1`, LiteLLM, TinyProxy, AIH shim.
3. Bottom layer: Ollama, llama.cpp server, vLLM, LocalAI, etc.
4. Model: Qwen, Llama, Gemma, Mistral, DeepSeek, model size, quantization.
5. AIH test: AIchess, AI/ML certificate-style test, k-phd/Wikipedia-only.
6. Run mode: one-shot probe, short game, repeated statistical run.

## Current Obstacles

1. The agent-layer axis is now started, but the exact P0 implementation set has
   not yet been reduced to local runnable adapters.

2. TinyProxy is useful as an HTTP control/logging layer, but it does not by
   itself normalize LLM provider differences. The project still needs either a
   small AIH transport shim, LiteLLM, or both.

3. Ollama compatibility is not one thing. Native Ollama API, Ollama `/v1`
   OpenAI compatibility, and LiteLLM-backed Ollama calls can behave differently.

4. Tool calling, JSON output, streaming, model identity reporting, and token
   usage reporting must be verified per agent layer and per backend.

5. Local model choice is still a test variable. Qwen, Llama, Mistral, Gemma, and
   DeepSeek variants may differ more than the agent framework does on small
   tests.

6. The first AIH test adapter contract exists conceptually, but the AIchess
   one-move probe still needs a concrete runner, parser, referee call, and
   result schema for each P0 agentic layer.

7. The scoring harness must stay external to the tested agent. That means more
   plumbing up front, but it is necessary for credible results.

8. Reproducibility needs environment capture: model tag, quantization, runtime,
   CPU/GPU, context length, temperature, seed if supported, timeout, proxy path,
   and exact prompt envelope.

## First AIH Pairings

The first matrix should not attempt every AIH test at once. Use AIchess as the
first bounded test because it has an external referee and compact scoring rules.

| Agentic layer | AIchess one-move probe | AIchess short game | AI/ML certificate-style test | k-phd / Wikipedia-only test |
| --- | --- | --- | --- | --- |
| HybridAI native v1 | target first | target after probe | later | later |
| LangGraph / LangChain | target first | target after probe | later | later |
| CrewAI | target first | target after probe | later | later |
| AutoGen | target first | target after probe | later | later |
| OpenHands | defer | possible later | possible later | possible later |
| LlamaIndex agents | not first | not first | candidate | candidate |

## Adapter Contract

Each agentic layer adapter should expose the same minimal interface:

```text
input:
  test_id
  agent_id
  model_id
  prompt
  allowed_tools
  time_limit_ms
  output_schema

output:
  raw_response
  parsed_response
  parse_status
  timing_ms
  model_reported
  transport_reported
  errors
  metadata
```

The AIH harness, not the tested agent, should own:

- test definition,
- prompt envelope,
- referee/grader,
- output parsing,
- score assignment,
- result record,
- comparison statistics.

## Status Vocabulary

Use these labels in the stack-by-test matrix:

- `candidate`: agent layer appears compatible but is not implemented locally.
- `adapter planned`: a local adapter design exists.
- `adapter exists`: adapter code exists but has not completed a validated run.
- `runnable`: adapter can execute the test end-to-end.
- `result recorded`: raw output, parsed output, score, timing, and errors are archived.
- `not suitable`: pairing is intentionally excluded.

## Immediate Next Step

Create the AIchess one-move probe adapter for each P0 agentic layer. The first
result target is not chess strength. The first target is protocol compliance:

1. receives bounded prompt,
2. returns one parseable legal move,
3. preserves raw response,
4. records timing and model identity,
5. lets the external referee accept or reject the move.

## Verification Sources

- CrewAI LLM documentation includes an Ollama local-LLM configuration using
  `model="ollama/..."` and `base_url="http://localhost:11434"`.
- LangChain ChatOllama documentation describes the `langchain-ollama` package,
  model instantiation, tool calling, structured output, and streaming support.
- AutoGen documentation includes a local LLM example using Ollama through a
  LiteLLM proxy and an OpenAI-compatible client.
- Ollama documentation describes OpenAI compatibility through
  `http://localhost:11434/v1/`.
- LiteLLM documentation describes Ollama provider support, proxy mode, JSON
  mode, and tool calling paths.
- llama.cpp server documentation describes OpenAI-compatible chat completions,
  completions, responses, embeddings, timing fields, and schema-constrained JSON.
- vLLM documentation describes OpenAI-compatible online serving, but it is a
  heavier bottom-layer candidate than Ollama for the first local tests.
- OpenHands has published agent-platform documentation and papers, but the exact
  current local Ollama configuration should be verified before it becomes a P0
  implementation target.
