# Intermediate Certificate-Style AIH v1

Created: 2026-07-13

## Prototype

`run_intermediate_cert_probe_v1.py` runs a five-question certificate-style
AI/ML probe using only a provided source packet.

The test records:

- prompt,
- raw response,
- parsed answers,
- score,
- timing,
- source-boundary notes.

## Bridge Test Environment

`test_env/run_bridge_quiz_env.py` runs the shared three-question
AI/ML certificate and K-PhD bridge quiz as an intermediate certificate-style
environment.

It writes JSON results to `test_env/runs/`.
