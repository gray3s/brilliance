# LinkedIn Draft - AIH v2 Evaluation Prototype

Primary source links:

- AIH v2 prototype README:
  https://github.com/gray3s/brilliance/blob/main/aih/v2/README.md
- AIH v2 SQL schema:
  https://github.com/gray3s/brilliance/blob/main/aih/v2/aih_v2_schema.sql
- AIH v2 test catalog:
  https://github.com/gray3s/brilliance/blob/main/aih/v2/tests.csv
- AIChess v2 checkpoint:
  https://github.com/gray3s/brilliance/blob/main/aih/aichess/v2/project-development-logs/aichess_v2_agent_sort_checkpoint_20260725.md

Draft:

Today the AIH test work moved past being only an AIChess harness.

AIChess is still essential. It is effectively the practical AIH v1 prototype:
one hard, visible task where agents must interact with rules, state, illegal
moves, and repeatable scoring.

But the next layer needs to be more general.

I started the AIH v2 evaluation prototype as a database-backed test catalog,
not as a top-level Brilliance test. That distinction matters. I want to reserve
Brilliance testing for agents that show real creative behavior, not just for
exhausting a list of common failure modes.

The new AIH v2 structure is:

- Class1: general capability probes using `Class 1:<level>:<area>:<test>`
- Class2: games only, such as chess, checkers, go, poker, or backgammon
- Class3: certification rollups built from Class1 and Class2 components

For Class1, the current general areas are:

- language
- logic
- geography
- math
- physics
- chemistry

The current catalog has one test for every Class1 level and area combination:

- 20 levels, from level 0 through level 19
- 6 general areas
- 120 Class1 tests total

It also includes one Class2 chess entry probe:

`Class 2.chess.1.1:1`

That means one chess board, clue mode 1, and one observed legal move as the
entry threshold.

The saved AIH v2 table snapshot currently contains:

- 18 known agent rows
- 121 test definitions
- 2,178 catalog-only agent/test result rows
- 90 Class3 certification rollup rows

The current saved database form is CSV/JSONL tables plus an SQL schema. It is a
catalog/prototype snapshot, not a completed claim that every local model has
been fully retested under the new structure. A live full local Ollama retest
was started, then stopped after I moved the scope out of top-level Brilliance
and into AIH v2 where it belongs.

This gives the project a cleaner separation:

- AIChess / AIH v1: concrete game harness and compatibility evidence
- AIH v2: general capability database, game tests, and certification rollups
- Brilliance: reserved for demonstrated creative behavior

That framing should make the next round of testing less muddled. The point is
not to call every failure a lack of intelligence. The point is to locate where
the system actually breaks: language, logic, geography, math, physics,
chemistry, game interaction, or certification-level reliability.

Source:
https://github.com/gray3s/brilliance/tree/main/aih/v2
