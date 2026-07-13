#!/usr/bin/env python3
"""AIH intermediate certificate-style v1 probe."""

from __future__ import annotations

import argparse
import json
import re
import time
from pathlib import Path


TEST_ID = "aih_intermediate_cert_probe_v1_20260713"
OUT_DIR = Path(__file__).resolve().parent / "runs"

SOURCE_PACKET = """
Allowed academic AI/ML source packet:

MIT OCW 6.036 frames machine learning around modeling and prediction, including
learning-problem formulation, representation, over-fitting, generalization,
supervised learning, and reinforcement learning.

MIT OCW 6.034 frames artificial intelligence around knowledge representation,
problem solving, and learning methods for assembling solutions to concrete
computational problems.

Stanford CS229 lecture notes include supervised learning, linear regression,
logistic regression, model selection, cross validation, generalization,
regularization, unsupervised learning, foundation models, and reinforcement
learning.

Stanford CS221 frames AI as rigorous mathematical tools for complex real-world
problems, including machine learning, search, Markov decision processes, game
playing, constraint satisfaction, graphical models, and logic.

Shared definitions for this v1 probe:
Supervised learning uses labeled examples with target outputs.
Unsupervised learning finds structure or patterns without target labels.
Validation/model selection compares modeling choices during development.
Test evaluation is final held-out assessment after model choices are fixed.
Generalization means performance beyond the training cases.
Over-fitting means fitting training data in a way that fails to generalize.
Data leakage means using information unavailable at real prediction time during
training or evaluation.
"""

EXAM = """
Use only the allowed source packet. Return only JSON with keys "1", "2", "3",
"4", and "5". Values must be A, B, C, or D.

1. Which learning setup uses labeled examples?
A. Unsupervised learning
B. Supervised learning
C. Random guessing
D. Test-only evaluation

2. What should be held back for final evaluation after model choices are fixed?
A. Training set
B. Prompt text
C. Test set
D. System clock

3. Which Stanford CS221 topic is closest to choosing actions in sequential
decision problems under uncertainty?
A. Markov decision processes
B. Static file formatting
C. Audio transcription
D. Calendar arithmetic

4. Which MIT 6.034 topic area is most directly about symbolic facts and rules?
A. Knowledge representation
B. Image compression only
C. Spreadsheet styling
D. GPU cooling

5. What is data leakage?
A. Using future/unavailable information during training or evaluation
B. Saving a JSON result file
C. Splitting data into train and test sets
D. Reporting uncertainty
"""

ANSWER_KEY = {"1": "B", "2": "C", "3": "A", "4": "A", "5": "A"}


def fixture_agent(prompt: str) -> str:
    _ = prompt
    return '{"1":"B","2":"C","3":"A","4":"A","5":"A"}'


def parse_answers(raw_response: str) -> dict[str, str]:
    try:
        start = raw_response.index("{")
        end = raw_response.rindex("}") + 1
        data = json.loads(raw_response[start:end])
        return {str(k): str(v).strip().upper()[:1] for k, v in data.items()}
    except Exception:
        answers: dict[str, str] = {}
        for match in re.finditer(r"\b([1-5])\s*[:.)-]\s*([ABCD])\b", raw_response.upper()):
            answers[match.group(1)] = match.group(2)
        return answers


def run(agent_response: str | None) -> dict[str, object]:
    prompt = SOURCE_PACKET + "\n\n" + EXAM
    start = time.time()
    raw_response = agent_response if agent_response is not None else fixture_agent(prompt)
    elapsed_ms = round((time.time() - start) * 1000, 3)
    parsed = parse_answers(raw_response)
    per_question = {
        q: {
            "expected": expected,
            "actual": parsed.get(q),
            "correct": parsed.get(q) == expected,
        }
        for q, expected in ANSWER_KEY.items()
    }
    score = sum(1 for row in per_question.values() if row["correct"])
    return {
        "test_id": TEST_ID,
        "test_family": "intermediate_certificate_style",
        "prototype_level": "v1",
        "agent_id": "fixture_agent" if agent_response is None else "provided_response",
        "source_mode": "provided_sources_only",
        "prompt": prompt,
        "raw_response": raw_response,
        "parsed_response": parsed,
        "score": score,
        "max_score": len(ANSWER_KEY),
        "per_question": per_question,
        "timing_ms": elapsed_ms,
        "errors": [] if len(parsed) == len(ANSWER_KEY) else ["missing_or_unparseable_answers"],
        "notes": [
            "Prototype checks applied AI/ML factual discipline under a consolidated MIT/Stanford source packet.",
            "Full v1 should add uncertainty/refusal questions and evidence citation checks.",
        ],
    }


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--agent-response", help="Raw response to score instead of fixture response.")
    args = parser.parse_args()

    OUT_DIR.mkdir(parents=True, exist_ok=True)
    result = run(args.agent_response)
    stamp = time.strftime("%Y%m%d_%H%M%S")
    out = OUT_DIR / f"{TEST_ID}_{stamp}.json"
    out.write_text(json.dumps(result, indent=2), encoding="utf-8")
    print(json.dumps(result, indent=2))
    print(out)


if __name__ == "__main__":
    main()
