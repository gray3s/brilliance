#!/usr/bin/env python3
"""K-PhD bridge quiz environment."""

from __future__ import annotations

import argparse
import json
import re
import time
from pathlib import Path


TEST_ID = "aih_kphd_bridge_quiz_env_v1_20260713"
TEST_FAMILY = "k-phd_academic_source_packet"


def repo_aih_root() -> Path:
    return Path(__file__).resolve().parents[3]


def load_text(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def load_quiz() -> dict[str, object]:
    quiz_path = repo_aih_root() / "shared" / "ai_ml_cert_kphd_bridge_quiz_v1_20260713.json"
    return json.loads(load_text(quiz_path))


def build_prompt(quiz: dict[str, object]) -> str:
    source_path = repo_aih_root() / "shared" / str(quiz["source_packet"])
    source_packet = load_text(source_path)
    lines = [source_packet.strip(), "", str(quiz["instructions"]).strip(), ""]
    for question in quiz["questions"]:  # type: ignore[index]
        q = question
        lines.append(f"{q['id']}. {q['prompt']}")
        for key, value in q["choices"].items():
            lines.append(f"{key}. {value}")
        lines.append("")
    return "\n".join(lines).strip()


def fixture_agent(_: str) -> str:
    return '{"1":"B","2":"A","3":"B"}'


def parse_answers(raw_response: str) -> dict[str, str]:
    try:
        start = raw_response.index("{")
        end = raw_response.rindex("}") + 1
        data = json.loads(raw_response[start:end])
        return {str(k): str(v).strip().upper()[:1] for k, v in data.items()}
    except Exception:
        answers: dict[str, str] = {}
        for match in re.finditer(r"\b([1-3])\s*[:.)-]\s*([ABCD])\b", raw_response.upper()):
            answers[match.group(1)] = match.group(2)
        return answers


def run(agent_response: str | None) -> dict[str, object]:
    quiz = load_quiz()
    prompt = build_prompt(quiz)
    started = time.time()
    raw_response = agent_response if agent_response is not None else fixture_agent(prompt)
    elapsed_ms = round((time.time() - started) * 1000, 3)
    parsed = parse_answers(raw_response)
    per_question = {}
    for question in quiz["questions"]:  # type: ignore[index]
        qid = str(question["id"])
        expected = str(question["answer"])
        actual = parsed.get(qid)
        per_question[qid] = {
            "expected": expected,
            "actual": actual,
            "correct": actual == expected,
            "skill": question.get("skill"),
        }
    score = sum(1 for item in per_question.values() if item["correct"])
    max_score = int(quiz["scoring"]["max_score"])  # type: ignore[index]
    return {
        "test_id": TEST_ID,
        "test_family": TEST_FAMILY,
        "environment": "k-phd/v1/test_env",
        "quiz_id": quiz["quiz_id"],
        "source_mode": quiz["source_mode"],
        "agent_id": "fixture_agent" if agent_response is None else "provided_response",
        "prompt": prompt,
        "raw_response": raw_response,
        "parsed_response": parsed,
        "score": score,
        "max_score": max_score,
        "passed": score >= int(quiz["scoring"]["pass_score"]),  # type: ignore[index]
        "per_question": per_question,
        "timing_ms": elapsed_ms,
        "errors": [] if len(parsed) == max_score else ["missing_or_unparseable_answers"],
        "kphd_controls": {
            "reference_mode": "provided_sources_only",
            "max_questions": 3,
            "external_sources_allowed": 0,
            "remote_ai_sources_allowed": 0
        }
    }


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--agent-response", help="Raw response to score instead of fixture response.")
    args = parser.parse_args()

    result = run(args.agent_response)
    out_dir = Path(__file__).resolve().parent / "runs"
    out_dir.mkdir(parents=True, exist_ok=True)
    out = out_dir / f"{TEST_ID}_{time.strftime('%Y%m%d_%H%M%S')}.json"
    out.write_text(json.dumps(result, indent=2), encoding="utf-8")
    print(json.dumps(result, indent=2))
    print(out)


if __name__ == "__main__":
    main()
