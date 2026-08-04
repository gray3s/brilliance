# Installed Agent Launch Manual - Standalone Bash Terminal

Date: 2026-08-03

Scope:

Installed local Ollama agents launched directly from a standalone bash
terminal.

## Basic Pattern

Open a bash terminal and run:

```bash
ollama run MODEL_NAME
```

To exit an interactive Ollama session, use:

```text
/bye
```

## Installed Local Agent Launch Commands

Use these commands to launch an installed local agent directly:

```bash
ollama run gemma3:4b
ollama run llama3.2:3b
ollama run mistral:latest
ollama run phi4-mini:latest
ollama run phi3:mini
ollama run tinyllama:latest
ollama run gemma3:1b
ollama run llama3.2:1b
ollama run gemma3:270m
ollama run smollm2:135m
ollama run qwen2.5:0.5b
ollama run granite4:3b
ollama run qwen2.5:latest
ollama run qwen2.5-coder:3b
ollama run qwen:4b
ollama run robit/qwen3.5-9b-r7-research:q4km
```

## One-Shot Prompt From Bash

To ask one question and return to the shell:

```bash
ollama run gemma3:4b "Give a one sentence status check."
```

Use the same pattern for any installed model:

```bash
ollama run qwen2.5-coder:3b "Write a bash command that lists markdown files."
```

## Save Output To A File

```bash
ollama run gemma3:4b "Summarize what an AIChess legality test measures." > gemma3_4b_test_output.txt
```

## Quick Installed-Agent Smoke Loop

This runs a short one-shot prompt through every installed local model and writes
one output file per model:

```bash
mkdir -p ollama_agent_smoke_outputs
for model in \
  gemma3:4b \
  llama3.2:3b \
  mistral:latest \
  phi4-mini:latest \
  phi3:mini \
  tinyllama:latest \
  gemma3:1b \
  llama3.2:1b \
  gemma3:270m \
  smollm2:135m \
  qwen2.5:0.5b \
  granite4:3b \
  qwen2.5:latest \
  qwen2.5-coder:3b \
  qwen:4b \
  robit/qwen3.5-9b-r7-research:q4km
do
  safe_name="$(printf '%s' "$model" | tr '/:' '__')"
  ollama run "$model" "Reply with exactly one sentence identifying yourself." \
    > "ollama_agent_smoke_outputs/${safe_name}.txt"
done
```

## List Installed Agents

```bash
ollama list
```

## AIH v4 Registry Note

AIH v4 has a separate passing-agent registry. That registry is for AIH
tournament eligibility, not for standalone terminal launch.

`/home/sag/RPA2/myLLC/AI/brilliance/aih/aichess/v3/qualification_cache/local_qualification_20260729032018.csv`

Passing local agents currently recorded:

```text
granite4:3b
qwen2.5-coder:3b
qwen2.5:0.5b
qwen2.5:latest
qwen:4b
smollm2:135m
gemma3:270m
llama3.2:1b
gemma3:1b
tinyllama:latest
phi3:mini
mistral:latest
llama3.2:3b
gemma3:4b
```

Installed but not currently passing in that registry:

```text
phi4-mini:latest
robit/qwen3.5-9b-r7-research:q4km
```
