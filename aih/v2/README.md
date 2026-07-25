# AIH v2 Evaluation Prototype

AIChess is treated as the practical AIH v1 prototype. This directory is the
general AIH v2 evaluation layer, separated from top-level Brilliance testing.

Top-level Brilliance testing is reserved for agents that demonstrate creative
behavior, not only common failure-mode exhaustion.

## Class Structure

- `Class 1:<level>:<area>:<test>`: general K-PhD capability probes.
- `Class 2.<game>...`: game tests. The current implemented Class2 probe is
  `Class 2.chess.1.1:1`.
- `Class 3...`: certification rollups derived from Class1 and Class2 component
  results.

Class1 area slots:

- `1`: language
- `2`: logic
- `3`: geography
- `4`: math
- `5`: physics
- `6`: chemistry

## Database Files

- `aih_v2_schema.sql`: SQL schema for the table files.
- `run_aih_v2_eval.sh`: shell-only table summary/listing wrapper.
- `agents.csv`: known agent catalog copied from the AIChess v2 agent table.
- `tests.csv`: Class1/Class2 test catalog.
- `results.csv` and `results.jsonl`: per-agent test rows.
- `class3_certifications.csv` and `class3_certifications.jsonl`: certification
  rollup rows.
- `run_summary.json`: latest run metadata.

The current saved database is a coherent catalog/prototype snapshot stored as
CSV/JSONL tables. Rows are marked `catalog_only` in `run_summary.json`, and
per-result rows currently use `pass_fail=skip` with
`failure_mode=catalog_only`. A live full local Ollama retest was started and
interrupted after the scope was moved from top-level Brilliance to AIH v2.

No Python runner is part of the active AIH v2 prototype.
