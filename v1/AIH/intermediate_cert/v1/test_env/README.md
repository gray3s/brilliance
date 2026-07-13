# Intermediate Certificate Bridge Test Environment

Created: 2026-07-13

## Purpose

Run the shared three-question AI/ML certificate and K-PhD bridge quiz as an
intermediate certificate-style test environment.

This environment treats the quiz primarily as a compact AI/ML certification
prototype:

- source-bound vocabulary,
- validation/test-evaluation reasoning,
- structured JSON answers,
- external parser and grader.

## Runner

```bash
./run_bridge_quiz_env.py
```

Optional:

```bash
./run_bridge_quiz_env.py --agent-response '{"1":"B","2":"A","3":"B"}'
```

## Output

Results are written to:

```text
test_env/runs/
```

The output record includes:

- test family,
- quiz ID,
- prompt,
- raw response,
- parsed response,
- per-question grading,
- score,
- pass/fail,
- timing.
