-- AIH v2 table schema.
-- The active database form is CSV/JSONL plus this schema; no Python runner is used.

CREATE TABLE agents (
  agent_key TEXT,
  agent_type TEXT,
  sort_order INTEGER,
  source TEXT,
  provider TEXT,
  namespace TEXT,
  model TEXT,
  label TEXT,
  available TEXT,
  timestamp TEXT
);

CREATE TABLE tests (
  test_id TEXT,
  class_id TEXT,
  class_name TEXT,
  level INTEGER,
  area_id INTEGER,
  area_name TEXT,
  test_ordinal INTEGER,
  prompt TEXT,
  expected TEXT,
  grader TEXT,
  notes TEXT
);

CREATE TABLE results (
  run_id TEXT,
  timestamp_utc TEXT,
  agent_key TEXT,
  model TEXT,
  provider TEXT,
  test_id TEXT,
  class_id TEXT,
  class_name TEXT,
  level INTEGER,
  area_id INTEGER,
  area_name TEXT,
  pass_fail TEXT,
  failure_mode TEXT,
  expected TEXT,
  selected TEXT,
  elapsed_s REAL,
  raw_reply TEXT
);

CREATE TABLE class3_certifications (
  run_id TEXT,
  timestamp_utc TEXT,
  agent_key TEXT,
  model TEXT,
  certification_id TEXT,
  class_id TEXT,
  component_query TEXT,
  component_count INTEGER,
  component_pass_count INTEGER,
  pass_rate REAL,
  pass_fail TEXT,
  threshold REAL,
  notes TEXT
);
