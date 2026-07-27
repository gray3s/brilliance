# AIH v2 Agent List

Timestamp: 2026-07-26 2130 MT

This is the current AIH v2 agent list under consideration. Cloud model sizes
are provider-undisclosed unless the provider publishes a stable parameter count.
Local sizes use the installed Ollama artifact sizes from this host.

Test documentation:

- [AIH v2 test notes](https://github.com/gray3s/brilliance/blob/main/aih/aichess/v2/misc/aih_v2_test_documentation_20260726.md)

## Cloud Agents

Sorted by provider and intended capability tier. Parameter size is not used for
cloud ranking here because these vendors do not expose a consistent model-size
measure for these API products.

|Slot| Provider  | Model                                  | Adapter                 | Size                 |
|--- |---        |---                                     |---                      |---                   |
| 06 | Anthropic | claude-opus-4 -> claude-opus-4-8       | anthropic_messages      | provider-undisclosed |
| 07 | Anthropic | claude-sonnet-4 -> claude-sonnet-4-6   | anthropic_messages      | provider-undisclosed |
| 08 | Anthropic | claude-3-7-sonnet -> claude-sonnet-4-6 | anthropic_messages      | provider-undisclosed |
| 09 | Anthropic | claude-3-5-haiku -> claude-haiku-4-5   | anthropic_messages      | provider-undisclosed |
| 03 | OpenAI 	 | gpt-5-mini | openai_responses          | provider-undisclosed    |
| 04 | OpenAI    | gpt-5-nano | openai_responses          | provider-undisclosed    |
| 01 | OpenAI    | gpt-4.1-mini | openai_responses        | provider-undisclosed    |
| 02 | OpenAI    | gpt-4o-mini | openai_responses         | provider-undisclosed    |
| 05 | Google    | gemini-3.1-flash-lite                  | gemini_generate_content | provider-undisclosed |

Latest cloud smoke result is recorded in the AIH v2 run artifacts for:

```text
aih/aichess/v2/runs/agents_all_test_20260727025831_gg4k21/summary.tsv
```

All cloud slots `01-09` passed:

```text
preschool_liveness
aichess clue level 6
```

## Local Agents

Sorted by installed artifact size, decreasing.

| Slot | Model | Installed Size | Current Bucket | Latest Notes |
|---|---:|---:|---|---|
| 16 | robit/qwen3.5-9b-r7-research:q4km | 5.6 GB | capability_probe | no-response readiness issue in preschool sweep |
| 14 | qwen2.5:latest | 4.7 GB | chess_candidate | AIChess clue 6 pass |
| 1E | mistral:latest | 4.4 GB | chess_candidate | AIChess clue 6 timeout/fail |
| 10 | gemma3:4b | 3.3 GB | chess_candidate | AIChess clue 6 pass |
| 1D | phi4-mini:latest | 2.5 GB | chess_candidate | AIChess clue 6 intermittent/partial; needs chess admission diagnostics |
| 15 | qwen:4b | 2.3 GB | chess_candidate | AIChess clue 6 pass |
| 1C | phi3:mini | 2.2 GB | chess_candidate | AIChess clue 6 fail; needs chess admission diagnostics |
| 11 | granite4:3b | 2.1 GB | chess_candidate | AIChess clue 6 pass |
| 1F | llama3.2:3b | 2.0 GB | chess_candidate | AIChess clue 6 pass |
| 12 | qwen2.5-coder:3b | 1.9 GB | chess_candidate | AIChess clue 6 pass |
| 19 | llama3.2:1b | 1.3 GB | chess_candidate | AIChess clue 6 fail; move normalization/chess diagnostics needed |
| 1A | gemma3:1b | 815 MB | capability_probe | reduced preschool floor pass |
| 1B | tinyllama:latest | 637 MB | capability_probe | reduced preschool floor pass; copy-token promoted out of preschool |
| 13 | qwen2.5:0.5b | 397 MB | capability_probe | reduced preschool floor pass |
| 18 | gemma3:270m | 291 MB | capability_probe | reduced preschool floor pass |
| 17 | smollm2:135m | 270 MB | capability_probe | reduced preschool floor pass; copy-token promoted out of preschool |

Latest local preschool evidence is recorded in the AIH v2 run artifacts for:

```text
aih/aichess/v2/runs/agents_all_test_20260727031943_tV6cL4/summary.tsv
aih/aichess/v2/runs/agents_all_test_20260727032551_z5nnIm/summary.tsv
aih/aichess/v2/runs/agents_all_test_20260727032556_Eh0X2j/summary.tsv
```

Latest local AIChess candidate evidence is recorded in the AIH v2 run artifacts for:

```text
aih/aichess/v2/runs/agents_all_test_20260727032609_R0HPdU/summary.tsv
```

## Admission Rule

One failed qualification run is not enough to reject an agent.

```text
one pass        -> possible capability
mixed pass/fail -> reliability or test-level hallucination
repeated no-pass -> concept-level non-qualification for that level
```

Adapter, transport, timeout, key, and prompt-design failures must be separated
from model capability failures.
