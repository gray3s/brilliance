# AIH v5 Ollama Registration Timeout Progress - 2026-08-15

## Goal

Improve AIH v5 registration for installed local Ollama agentic agents by treating registration as a communication-profile problem, not just a fixed pass/fail smoke.

Core requirements established during the session:

- Do not truncate the raw agent response before parsing.
- Preserve full Ollama JSON for audit.
- Treat `.response` as the official returned answer.
- Preserve `.thinking` for diagnostics, but do not silently substitute it as the official answer.
- Use `num_predict=256` initially for registration because thinking models can consume low token budgets before visible output.
- Find a timeout that works per agent by starting from one global maximum timeout, not by working upward from a short timeout.

## Created / Modified Files

### `ollama_agentic_registration_smoke.sh`

Path:

```text
/home/sag/RPA2/myLLC/AI/brilliance/aih/aichess/v5/ollama_agentic_registration_smoke.sh
```

Purpose:

- Standalone AIH-v5-style Ollama registration smoke.
- Uses `/api/generate`.
- Default `num_predict=256`.
- Preserves full request and response JSON under `standalone_registration_smoke/<stamp>/`.
- Parses full visible `.response` for legal UCI candidates.
- Does not parse `.thinking` as official answer.
- Records `thinking_only_legal_uci_not_official` when applicable.

### `ollama_agentic_registration_timeout_finder.sh`

Path:

```text
/home/sag/RPA2/myLLC/AI/brilliance/aih/aichess/v5/ollama_agentic_registration_timeout_finder.sh
```

Purpose:

- Runs one standalone registration smoke per model with a single global maximum timeout.
- Default global timeout: `360s`.
- Default `num_predict=256`.
- Records elapsed time and recommended per-agent timeout.
- Recommended timeout is `max(elapsed + 30s, elapsed * 1.25)`, capped at the global timeout.
- Emits:
  - `per_agent_timeouts.csv`
  - `passes.csv`
  - `failures.csv`
  - `agent_communication_settings.csv`

Bug fixed:

- The initial version counted the wrapper process exit status rather than the per-model CSV status. It now returns pass only when the row status is `pass`.

### `nemotron_registration_test.sh`

Path:

```text
/home/sag/RPA2/myLLC/AI/brilliance/aih/aichess/v5/nemotron_registration_test.sh
```

Purpose:

- AIH v5 wrapper registration test focused on `nemotron-3-nano:4b`.
- Fixed leading pasted bullet before shebang.
- Forces one-model registration by creating a temp one-model registry.
- Uses `AIH_V5_REGISTRATION_GAME_NUM_PREDICT=256`.

## Important Environment / Execution Note

Codex sandbox network namespace cannot reliably reach the user’s real `127.0.0.1:11434` Ollama service.

Observed:

- In sandbox: `curl` to `127.0.0.1:11434` failed immediately.
- `ollama serve` inside sandbox failed with `bind: address already in use`.
- Running the smoke outside the sandbox worked.

Therefore, actual smoke/finder runs need escalated/outside-sandbox execution to hit the real local Ollama HTTP service.

## Completed Timeout Finder Run

Run directory:

```text
/home/sag/RPA2/myLLC/AI/brilliance/aih/aichess/v5/standalone_registration_timeouts/20260815_102624
```

Primary outputs:

```text
/home/sag/RPA2/myLLC/AI/brilliance/aih/aichess/v5/standalone_registration_timeouts/20260815_102624/per_agent_timeouts.csv
/home/sag/RPA2/myLLC/AI/brilliance/aih/aichess/v5/standalone_registration_timeouts/20260815_102624/agent_communication_settings.csv
/home/sag/RPA2/myLLC/AI/brilliance/aih/aichess/v5/standalone_registration_timeouts/20260815_102624/passes.csv
/home/sag/RPA2/myLLC/AI/brilliance/aih/aichess/v5/standalone_registration_timeouts/20260815_102624/failures.csv
```

Global settings:

```text
global_timeout_seconds=360
num_predict=256
api_surface=ollama /api/generate
temperature=0
num_thread=1
keep_alive=0s
```

Actual CSV status count:

```text
pass 15
fail 4
```

Note: the script's printed final `passed=19 failed=0` for that run was stale/wrong due to the counter bug fixed afterward. Trust the CSV status counts.

## Passing Agents And Recommended Timeouts

From `agent_communication_settings.csv`:

```text
llama3-groq-tool-use:8b              elapsed=136s recommended_timeout=170s
nemotron-3-nano:4b                   elapsed=154s recommended_timeout=193s
gemma3:4b                            elapsed=61s  recommended_timeout=91s
llama3.2:3b                          elapsed=41s  recommended_timeout=71s
mistral:latest                       elapsed=273s recommended_timeout=342s
phi3:mini                            elapsed=42s  recommended_timeout=72s
tinyllama:latest                     elapsed=19s  recommended_timeout=49s
gemma3:1b                            elapsed=14s  recommended_timeout=44s
gemma3:270m                          elapsed=4s   recommended_timeout=34s
smollm2:135m                         elapsed=7s   recommended_timeout=37s
qwen2.5:0.5b                         elapsed=8s   recommended_timeout=38s
granite4:3b                          elapsed=41s  recommended_timeout=71s
qwen2.5:latest                       elapsed=214s recommended_timeout=268s
qwen2.5-coder:3b                     elapsed=41s  recommended_timeout=71s
qwen:4b                              elapsed=41s  recommended_timeout=71s
```

## Failing Agents Under First Profile

From `failures.csv`:

```text
qwen3:8b
  status=fail
  reason=curl_error
  elapsed=361s
  note=curl: (28) Operation timed out after 360002 milliseconds with 0 bytes received

phi4-mini:latest
  status=fail
  reason=no_legal_visible_uci
  elapsed=45s
  visible response=e7e5
  issue=valid UCI format, illegal for White from initial FEN

llama3.2:1b
  status=fail
  reason=no_legal_visible_uci
  elapsed=17s
  visible response=e4
  issue=SAN-like response; deterministic SAN-to-UCI normalization could convert to e2e4

robit/qwen3.5-9b-r7-research:q4km
  status=fail
  reason=curl_error
  elapsed=360s
  note=curl: (28) Operation timed out after 360002 milliseconds with 0 bytes received
```

## Key Technical Findings

1. AIH v5's 30s registration timeout is too low for multiple installed models.
   - `nemotron-3-nano:4b` passed standalone at 154s in the full run; one focused run measured 186s.
   - `mistral:latest` passed at 273s.
   - `qwen2.5:latest` passed at 214s.

2. Some failures are not timeout problems.
   - `llama3.2:1b` returned `e4`, which is recoverable if the harness supports SAN pawn move normalization from the current FEN.
   - `phi4-mini:latest` returned `e7e5`, which is UCI-shaped but illegal for White. It needs correction retry or a stronger legal-list prompt/profile.

3. Some thinking/tool-capable models may need a different communication profile.
   - `qwen3:8b` and `robit/qwen3.5-9b-r7-research:q4km` timed out with no bytes under `/api/generate`, `num_predict=256`, `timeout=360s`.
   - Next profile to test: `/api/chat` with `think:false`, lower `num_predict`, and same visible-response-only parse policy.

4. Hidden thinking matters.
   - Nemotron produced visible response `a2a3`, with `.thinking` length 346.
   - Low `num_predict` values such as 8 or 16 can starve visible output on thinking models.

## Recommended AIH v5 Design Change

Registration should generate and consume per-agent communication settings:

```text
model
provider=ollama
api_surface=generate|chat
registration_timeout_seconds
registration_num_predict
num_thread
keep_alive
temperature
official_parse_policy
diagnostic_parse_policy
last_elapsed_seconds
last_status
last_reason
response_json
```

Initial procedure:

1. Run one high-ceiling registration attempt per installed Ollama model.
2. Start with global ceiling, currently `360s`, `num_predict=256`.
3. If pass, record elapsed and recommended timeout.
4. If fail with legal-format issue, try modified parse or correction profile.
5. If fail by timeout/no bytes, try modified API profile, e.g. `/api/chat` with `think:false`.
6. Save resulting communication settings and use them during AIH v5 registration/tournament calls.

## Next Work

1. Add a second-profile smoke script or extend `ollama_agentic_registration_smoke.sh` with:
   - `AIH_V5_STANDALONE_REG_API_SURFACE=generate|chat`
   - `AIH_V5_STANDALONE_REG_THINK=false`
   - optional legal-list-only prompt mode
   - optional SAN normalization mode for registration only

2. Retest the four failed agents:
   - `qwen3:8b`
   - `phi4-mini:latest`
   - `llama3.2:1b`
   - `robit/qwen3.5-9b-r7-research:q4km`

3. Integrate `agent_communication_settings.csv` consumption into `aih_v5.sh` registration:
   - lookup candidate-specific timeout
   - lookup token budget
   - lookup API surface
   - preserve full raw response/thinking JSON
   - parse full visible response only unless policy explicitly allows normalization

## Latest Checkpoint

Completed full installed-model timeout finder run:

```text
/home/sag/RPA2/myLLC/AI/brilliance/aih/aichess/v5/standalone_registration_timeouts/20260815_102624/per_agent_timeouts.csv
```

Do not rerun this full registration pass just to recover context. The useful data is already on disk.

Actual status count from CSV:

```text
pass=15
fail=4
```

The timeout finder script has been fixed after this run so future printed counters should match CSV status. The completed run's printed `passed=19 failed=0` should be ignored.

Current script syntax status:

```text
bash -n ollama_agentic_registration_smoke.sh          pass
bash -n ollama_agentic_registration_timeout_finder.sh pass
```

`ollama_agentic_registration_smoke.sh` has been extended to support the next communication profiles:

```text
AIH_V5_STANDALONE_REG_API_SURFACE=generate|chat
AIH_V5_STANDALONE_REG_THINK=true|false
AIH_V5_STANDALONE_REG_PROMPT_MODE=clue|legal-list|minimal
AIH_V5_STANDALONE_REG_NORMALIZE_SAN=0|1
```

Next targeted tests should be only the four failures, not all installed agents:

```text
qwen3:8b
phi4-mini:latest
llama3.2:1b
robit/qwen3.5-9b-r7-research:q4km
```

Suggested next profile attempts:

```text
qwen3:8b
  Try chat API with think=false, legal-list prompt, num_predict=64 or 128, timeout=360.

robit/qwen3.5-9b-r7-research:q4km
  Try chat API with think=false, legal-list prompt, num_predict=64 or 128, timeout=360.

llama3.2:1b
  Try generate API with NORMALIZE_SAN=1. Previous response was e4, so expected normalized move is e2e4.

phi4-mini:latest
  Try legal-list prompt first. Previous response was e7e5, a side-confused illegal UCI move.
```

Run targeted examples outside sandbox against the real Ollama HTTP service:

```bash
AIH_V5_STANDALONE_REG_API_SURFACE=chat \
AIH_V5_STANDALONE_REG_THINK=false \
AIH_V5_STANDALONE_REG_PROMPT_MODE=legal-list \
AIH_V5_STANDALONE_REG_NUM_PREDICT=128 \
AIH_V5_STANDALONE_REG_TIMEOUT_SECONDS=360 \
./ollama_agentic_registration_smoke.sh qwen3:8b robit/qwen3.5-9b-r7-research:q4km

AIH_V5_STANDALONE_REG_PROMPT_MODE=legal-list \
AIH_V5_STANDALONE_REG_NORMALIZE_SAN=1 \
AIH_V5_STANDALONE_REG_NUM_PREDICT=256 \
AIH_V5_STANDALONE_REG_TIMEOUT_SECONDS=120 \
./ollama_agentic_registration_smoke.sh phi4-mini:latest llama3.2:1b
```

## AIH v5 Registration Integration Checkpoint

`aih_v5.sh` has been patched to optionally consume a per-agent communication settings CSV during registration.

New environment variable:

```text
AIH_V5_REGISTRATION_COMM_SETTINGS_CSV
```

Stable settings file copied from the completed timeout run:

```text
/home/sag/RPA2/myLLC/AI/brilliance/aih/aichess/v5/AIH_V5_OLLAMA_AGENT_COMMUNICATION_SETTINGS_20260815.csv
```

Current behavior:

- For each registration candidate, `aih_v5.sh` looks up the model in `AIH_V5_REGISTRATION_COMM_SETTINGS_CSV`.
- If found, it overrides that candidate's registration timeout with column `registration_timeout_seconds`.
- If found, it overrides query/game registration `num_predict` with column `registration_num_predict`.
- It logs the applied communication settings to the registration diagnostic log.
- It resets timeout/token defaults per candidate so one agent's settings do not leak into the next.

Syntax status after patch:

```text
bash -n aih_v5.sh pass
```

Example AIH v5 registration invocation using the generated settings:

```bash
AIH_V5_REGISTRATION_MODE=game \
AIH_V5_REGISTRATION_COMM_SETTINGS_CSV=/home/sag/RPA2/myLLC/AI/brilliance/aih/aichess/v5/AIH_V5_OLLAMA_AGENT_COMMUNICATION_SETTINGS_20260815.csv \
AIH_V5_REGISTRATION_GAME_SMOKE_PLIES=1 \
AIH_V5_REGISTRATION_DYNAMIC_TIMEOUT=0 \
./aih_v5.sh --registration-only --no-open --registration-forward
```

Important limitation:

- The current `aih_v5.sh` integration only consumes timeout and `num_predict` settings for the 15 agents that passed first-profile `/api/generate`.
- The four failed agents still need second-profile work before they can be added to the stable communication settings:
  - `qwen3:8b`
  - `phi4-mini:latest`
  - `llama3.2:1b`
  - `robit/qwen3.5-9b-r7-research:q4km`

## Admin Package Checkpoint

Local registration is now represented as a separate AIH v5 admin package.

Stable files in the v5 root:

```text
AIH_V5_LOCAL_AGENT_REGISTRATION_VERIFIED.csv
AIH_V5_LOCAL_AGENT_REGISTRATION_FAILURES.csv
AIH_V5_LOCAL_AGENT_REGISTRATION_COMM_SETTINGS.csv
AIH_V5_LOCAL_AGENT_REGISTRATION_SUMMARY.csv
AIH_V5_LOCAL_AGENT_REGISTRATION_README.md
AIH_V5_OLLAMA_AGENT_COMMUNICATION_SETTINGS_20260815.csv
```

Admin scripts:

```text
aih_v5_local_agent_registration.sh
ollama_agentic_registration_smoke.sh
ollama_agentic_registration_timeout_finder.sh
```

The source ZIP refresh script now explicitly includes the stable local
registration CSV/README/checkpoint files in addition to source and shell files.
It also refreshes both stable ZIP names:

```text
AIH_V5_SOURCE-LATEST.zip
AIH_V5_SOURCE_LATEST.zip
```

2026-08-15 package refresh:

```text
AIH_V5_SOURCE_20260815.zip
AIH_V5_SOURCE-LATEST.zip
AIH_V5_SOURCE_LATEST.zip
file_count=42
```

## Restart Plan

Stop point: 2026-08-15.

Current durable state:

- Local Ollama registration package exists in the AIH v5 root.
- Stable registration database verifies `verified_count=15` and
  `failure_count=4`.
- `aih_v5.sh` can consume per-agent registration communication settings through
  `AIH_V5_REGISTRATION_COMM_SETTINGS_CSV`.
- Source ZIP latest artifacts were refreshed locally and include the local
  registration package.
- Top-level AIH/Nemotron files from `~/RPA2/incoming` were moved under:
  `reference/incoming_housekeeping_20260815/`.
- No GitHub I/O should be assumed from Codex; user owns push/update scripts.

Do not rerun the full installed-model registration sweep on restart.

Next AIH v5 work unit:

1. Work one failed local Ollama model at a time.
2. Use targeted second-profile registration only.
3. After each successful recovery, publish/verify the stable local registration
   database and refresh source latest artifacts.
4. Let the user run the local GitHub update script between successful recoveries
   if desired.

First-profile failures still requiring targeted recovery:

```text
qwen3:8b
phi4-mini:latest
llama3.2:1b
robit/qwen3.5-9b-r7-research:q4km
```

Suggested restart order:

```text
1. llama3.2:1b
   Try generate API with legal-list prompt and SAN normalization enabled.
   Previous visible response was e4, so this is likely recoverable as e2e4.

2. phi4-mini:latest
   Try generate API with legal-list prompt.
   Previous visible response was e7e5, a side-confused illegal UCI move.

3. qwen3:8b
   Try chat API with think=false, legal-list prompt, num_predict=64 or 128,
   timeout=360.

4. robit/qwen3.5-9b-r7-research:q4km
   Try chat API with think=false, legal-list prompt, num_predict=64 or 128,
   timeout=360.
```

Useful local commands for restart:

```bash
cd /home/sag/RPA2/myLLC/AI/brilliance/aih/aichess/v5

./aih_v5_local_agent_registration.sh --verify

AIH_V5_STANDALONE_REG_PROMPT_MODE=legal-list \
AIH_V5_STANDALONE_REG_NORMALIZE_SAN=1 \
AIH_V5_STANDALONE_REG_NUM_PREDICT=256 \
AIH_V5_STANDALONE_REG_TIMEOUT_SECONDS=120 \
./ollama_agentic_registration_smoke.sh llama3.2:1b
```
