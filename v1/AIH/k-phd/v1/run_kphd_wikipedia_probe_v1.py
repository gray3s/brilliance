#!/usr/bin/env python3
"""AIH k-phd v1 academic source-packet knowledge-ladder probe."""

from __future__ import annotations

import argparse
import json
import re
import time
from pathlib import Path


TEST_ID = "aih_kphd_academic_packet_probe_v1_20260713"
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
Search explores possible states/actions to solve a problem.
A Markov decision process is a formal model for sequential action under
uncertainty.
"""

EXAM = """
Use only the allowed academic packet. Return only JSON with keys "1",
"2", "3", "4", and "5". Values must be A, B, C, or D.

1. At the undergraduate-entry level, which term best matches learning from
labeled examples?
A. Unsupervised learning
B. Supervised learning
C. Search only
D. Logic only

2. At the undergraduate level, which concept is about performance beyond the
training cases?
A. Generalization
B. File permissions
C. Token counting only
D. Calendar sorting

3. At the upper-undergraduate AI level, which topic explores possible
states/actions to solve a problem?
A. Search
B. Audio compression
C. Spreadsheet formatting
D. Image cropping only

4. At the graduate-prep level, which topic models sequential decisions under
uncertainty?
A. Markov decision processes
B. Static README prose
C. Filename sorting
D. Plain text wrapping

5. Which answer best describes why validation/model selection and final test
evaluation should be separated?
A. To compare choices during development and reserve final assessment for a
held-out check after choices are fixed
B. To make all datasets identical
C. To remove the need for a model
D. To prevent any scoring
"""

ANSWER_KEY = {"1": "B", "2": "A", "3": "A", "4": "A", "5": "A"}


def fixture_agent(prompt: str) -> str:
    _ = prompt
    return '{"1":"B","2":"A","3":"A","4":"A","5":"A"}'


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
        "test_class": "class_3",
        "test_class_name": "source_bound_knowledge_education_ladder_hallucination",
        "test_family": "k-phd_academic_source_packet",
        "prototype_level": "v1",
        "agent_id": "fixture_agent" if agent_response is None else "provided_response",
        "source_mode": "provided_academic_source_packet",
        "prompt": prompt,
        "raw_response": raw_response,
        "parsed_response": parsed,
        "score": score,
        "max_score": len(ANSWER_KEY),
        "per_question": per_question,
        "timing_ms": elapsed_ms,
        "errors": [] if len(parsed) == len(ANSWER_KEY) else ["missing_or_unparseable_answers"],
        "notes": [
            "Prototype uses the same MIT/Stanford packet as the intermediate test, but asks ladder-style questions.",
            "Full v1 should support level tags from K-12 through graduate/PhD tasks.",
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
