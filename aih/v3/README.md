# AIH v3 K-6 Graduation Seed

Created: 2026-07-27

AIH v3 starts from a graduation model rather than a one-row-per-agent smoke
model.

## Scope

- Run adapter smoke/liveness first to confirm the agent responds.
- Run AIChess across clue levels and record plies by clue level.
- Run Class1 K-6 tests as a level-by-level promotion ladder.
- Promote only when the agent scores at least 80 percent at the current level.

## Default Agent Policy

`run_aih_v3_all_test.sh` and `bin/aih_v3_all_test` default to OpenAI cloud
slots only when no agent selection is supplied. The wrapper passes
`--openai-only --clue-mode 0` to the delegated AIChess v2 all-agent runner.

Gemini, Anthropic, Claude, and other non-OpenAI cloud agents are refused by
default for v3. Local agents remain available through explicit v3 selections
such as `--local-only` or `--selector 12`.

## K-6 Promotion Rule

For each agent and each K-6 level:

```text
attempts = N * M
pass_rate = pass_count / attempts
promote_to_next_level = pass_rate >= 0.80
```

For this seed:

```text
M = 5 tests per level
levels = 1 2 3 4 5 6
promotion_threshold = 0.80
```

`N` is the repeat count per test. A quick initial run can use `N=1`, producing
5 attempts per level. A stronger rating run should raise `N` before making
durable claims.

## Files

- `tests_k6_seed.csv`: 30 deterministic Class1 tests, five at each K-6 level.
- `aih_v3_k6_promotion_plan_20260727.md`: scoring and reporting rules.
