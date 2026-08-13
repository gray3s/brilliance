#include <algorithm>
#include <cctype>
#include <ctime>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <limits>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <vector>

namespace fs = std::filesystem;

struct AgentStats {
  int legal = 0;
  int fail = 0;
  int transport = 0;
  int timeout = 0;
};

struct RegStats {
  int failures = 0;
  int timeouts = 0;
  int passes = 0;
  double elapsed = 0.0;
};

struct RankingStats {
  int rows = 0;
  double aih_sum = 0.0;
  double legal_sum = 0.0;
  double aih_events = 0.0;
  double legal_events = 0.0;
  double aih_time_sum = 0.0;
  double legal_time_sum = 0.0;
  double weighted_sum = 0.0;
  double turn_time_sum = 0.0;
  double plies_sum = 0.0;
  double token_rate_sum = 0.0;
};

static std::string html_escape(std::string s) {
  std::string out;
  for (char c : s) {
    if (c == '&') out += "&amp;";
    else if (c == '<') out += "&lt;";
    else if (c == '>') out += "&gt;";
    else out += c;
  }
  return out;
}

static std::string basename_no_summary(const fs::path& p) {
  std::string name = p.filename().string();
  const std::string suffix = "_summary.md";
  if (name.size() >= suffix.size() && name.substr(name.size() - suffix.size()) == suffix) {
    name.resize(name.size() - suffix.size());
  }
  return name;
}

static fs::path latest_summary(const fs::path& root) {
  std::vector<fs::path> dirs = {
    root / "data",
    root / "runs" / "aih_v5_pairwise_prototype_20260729"
  };
  fs::path best;
  fs::file_time_type best_time{};
  bool have = false;
  for (const auto& dir : dirs) {
    if (!fs::exists(dir)) continue;
    for (const auto& ent : fs::directory_iterator(dir)) {
      if (!ent.is_regular_file()) continue;
      std::string n = ent.path().filename().string();
      if (n.size() < 11 || n.substr(n.size() - 11) != "_summary.md") continue;
      auto t = ent.last_write_time();
      if (!have || t > best_time) {
        have = true;
        best_time = t;
        best = ent.path();
      }
    }
  }
  return best;
}

static std::vector<fs::path> latest_summaries(const fs::path& root, int count) {
  std::vector<std::pair<fs::file_time_type, fs::path>> found;
  std::vector<fs::path> dirs = {
    root / "data",
    root / "runs" / "aih_v5_pairwise_prototype_20260729"
  };
  for (const auto& dir : dirs) {
    if (!fs::exists(dir)) continue;
    for (const auto& ent : fs::directory_iterator(dir)) {
      if (!ent.is_regular_file()) continue;
      std::string n = ent.path().filename().string();
      if (n.size() < 11 || n.substr(n.size() - 11) != "_summary.md") continue;
      found.push_back({ent.last_write_time(), ent.path()});
    }
  }
  std::sort(found.begin(), found.end(), [](const auto& a, const auto& b) {
    return a.first > b.first;
  });
  std::vector<fs::path> out;
  std::set<std::string> seen;
  for (const auto& item : found) {
    std::string base = basename_no_summary(item.second);
    if (seen.count(base)) continue;
    seen.insert(base);
    out.push_back(item.second);
    if (static_cast<int>(out.size()) >= count) break;
  }
  return out;
}

static std::vector<std::string> split_csv_line(const std::string& line) {
  std::vector<std::string> fields;
  std::string cur;
  bool quote = false;
  for (size_t i = 0; i < line.size(); ++i) {
    char c = line[i];
    if (c == '"') {
      quote = !quote;
    } else if (c == ',' && !quote) {
      fields.push_back(cur);
      cur.clear();
    } else {
      cur += c;
    }
  }
  fields.push_back(cur);
  return fields;
}

static std::vector<std::string> split_tsv_line(const std::string& line) {
  std::vector<std::string> fields;
  std::string cur;
  for (char c : line) {
    if (c == '\t') {
      fields.push_back(cur);
      cur.clear();
    } else {
      cur += c;
    }
  }
  fields.push_back(cur);
  return fields;
}

static std::string trim(std::string s) {
  while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) s.erase(s.begin());
  while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) s.pop_back();
  if (s.size() >= 2 && s.front() == '"' && s.back() == '"') {
    s = s.substr(1, s.size() - 2);
  }
  return s;
}

static std::map<std::string, size_t> csv_header_index(const std::vector<std::string>& header) {
  std::map<std::string, size_t> idx;
  for (size_t i = 0; i < header.size(); ++i) idx[trim(header[i])] = i;
  return idx;
}

static std::string csv_get(const std::vector<std::string>& fields, const std::map<std::string, size_t>& idx, const std::string& key) {
  auto it = idx.find(key);
  if (it == idx.end() || it->second >= fields.size()) return "";
  return trim(fields[it->second]);
}

static double to_double(const std::string& s, double fallback = 0.0) {
  try {
    return std::stod(trim(s));
  } catch (...) {
    return fallback;
  }
}

static std::string local_process_timestamp() {
  std::time_t now = std::time(nullptr);
  std::tm tm{};
  localtime_r(&now, &tm);
  char buf[32];
  std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S %Z", &tm);
  return buf;
}

static std::string timestamp_from_filename(const fs::path& p) {
  static const std::regex ts_re(".*([0-9]{8}_[0-9]{6}).*");
  std::smatch m;
  std::string name = p.filename().string();
  if (std::regex_match(name, m, ts_re)) return m[1].str();
  return "";
}

static std::map<std::string, RankingStats> read_ranking_csv(const fs::path& csv) {
  std::map<std::string, RankingStats> stats;
  std::ifstream in(csv);
  if (!in) return stats;
  std::string line;
  if (!std::getline(in, line)) return stats;
  auto idx = csv_header_index(split_csv_line(line));
  if (!idx.count("agent") || !idx.count("aih_pct")) return stats;
  while (std::getline(in, line)) {
    if (trim(line).empty()) continue;
    auto f = split_csv_line(line);
    std::string agent = csv_get(f, idx, "agent");
    if (agent.empty()) continue;
    double plies = to_double(csv_get(f, idx, "plies"));
    double aih_pct = to_double(csv_get(f, idx, "aih_pct"));
    double legal_pct = to_double(csv_get(f, idx, "legal_pct"));
    double aih_events = plies * aih_pct / 100.0;
    auto& s = stats[agent];
    s.rows++;
    s.aih_sum += aih_pct;
    s.legal_sum += legal_pct;
    s.aih_events += aih_events;
    s.legal_events += plies * legal_pct / 100.0;
    s.weighted_sum += to_double(csv_get(f, idx, "weighted_score"));
    s.turn_time_sum += to_double(csv_get(f, idx, "net_turn_time_per_ply_s"));
    s.plies_sum += plies;
    s.token_rate_sum += to_double(csv_get(f, idx, "total_tokens_per_s"));
  }
  return stats;
}

static std::map<std::string, RegStats> read_registration_csv(const fs::path& csv);
static std::string shell_quote(const fs::path& p);

static std::vector<fs::path> data_csv_files(const fs::path& root) {
  std::vector<fs::path> files;
  fs::path dir = root / "data";
  if (!fs::exists(dir)) return files;
  for (const auto& ent : fs::directory_iterator(dir)) {
    if (!ent.is_regular_file()) continue;
    if (ent.path().extension() == ".csv") files.push_back(ent.path());
  }
  std::sort(files.begin(), files.end());
  return files;
}

static void merge_rankings(std::map<std::string, RankingStats>& dest, const std::map<std::string, RankingStats>& src) {
  for (const auto& [agent, s] : src) {
    auto& d = dest[agent];
    d.rows += s.rows;
    d.aih_sum += s.aih_sum;
    d.legal_sum += s.legal_sum;
    d.aih_events += s.aih_events;
    d.legal_events += s.legal_events;
    d.aih_time_sum += s.aih_time_sum;
    d.legal_time_sum += s.legal_time_sum;
    d.weighted_sum += s.weighted_sum;
    d.turn_time_sum += s.turn_time_sum;
    d.plies_sum += s.plies_sum;
    d.token_rate_sum += s.token_rate_sum;
  }
}

static void merge_regs(std::map<std::string, RegStats>& dest, const std::map<std::string, RegStats>& src) {
  for (const auto& [agent, s] : src) {
    auto& d = dest[agent];
    d.failures += s.failures;
    d.timeouts += s.timeouts;
    d.passes += s.passes;
    d.elapsed += s.elapsed;
  }
}

static std::vector<fs::path> data_jsonl_files(const fs::path& root) {
  std::vector<fs::path> files;
  fs::path dir = root / "data";
  if (!fs::exists(dir)) return files;
  for (const auto& ent : fs::directory_iterator(dir)) {
    if (!ent.is_regular_file()) continue;
    if (ent.path().extension() == ".jsonl") files.push_back(ent.path());
  }
  std::sort(files.begin(), files.end());
  return files;
}

static std::map<std::string, RankingStats> read_jsonl_event_rankings(const fs::path& jsonl) {
  std::map<std::string, RankingStats> stats;
  std::string cmd =
    "jq -r '.events[]? | "
    "[.model, "
    "(if (.model == \"harness\") then \"skip\" "
    "elif (.transport_failure == true or .legal_by_rules == false or .legal == false or ((.error // \"\") != \"\")) then \"aih\" "
    "else \"legal\" end), "
    "(.move_to_referee_elapsed_s // .response.elapsed_s // .original_response.elapsed_s // 0)] | @tsv' " + shell_quote(jsonl);
  FILE* pipe = popen(cmd.c_str(), "r");
  if (!pipe) return stats;
  char buf[4096];
  while (fgets(buf, sizeof(buf), pipe)) {
    std::string line(buf);
    while (!line.empty() && (line.back() == '\n' || line.back() == '\r')) line.pop_back();
    auto f = split_tsv_line(line);
    if (f.size() < 3 || f[0].empty() || f[1] == "skip") continue;
    auto& s = stats[f[0]];
    double elapsed = to_double(f[2]);
    if (f[1] == "legal") {
      s.legal_events += 1.0;
      s.legal_time_sum += elapsed;
    } else {
      s.aih_events += 1.0;
      s.aih_time_sum += elapsed;
    }
  }
  pclose(pipe);
  return stats;
}

static std::string shell_quote(const fs::path& p) {
  std::string s = p.string();
  std::string out = "'";
  for (char c : s) {
    if (c == '\'') out += "'\\''";
    else out += c;
  }
  out += "'";
  return out;
}

static std::map<std::string, RegStats> read_registration_csv(const fs::path& csv) {
  std::map<std::string, RegStats> regs;
  std::ifstream in(csv);
  if (!in) return regs;
  std::string line;
  std::getline(in, line);
  while (std::getline(in, line)) {
    auto f = split_csv_line(line);
    if (f.size() < 4) continue;
    std::string agent = trim(f[0]);
    std::string status = trim(f[1]);
    std::string reason = f.size() > 2 ? f[2] : "";
    double elapsed = 0.0;
    try { elapsed = std::stod(f[3]); } catch (...) {}
    auto& r = regs[agent];
    r.elapsed += elapsed;
    if (status == "pass") r.passes++;
    else {
      r.failures++;
      if (reason.find("timed out") != std::string::npos) r.timeouts++;
    }
  }
  return regs;
}

static std::map<std::string, RegStats> read_registration(const fs::path& csv) {
  return read_registration_csv(csv);
}

static bool write_data_csv_report(const fs::path& root, const fs::path& out) {
  auto csvs = data_csv_files(root);
  std::map<std::string, RankingStats> rankings;
  std::map<std::string, RankingStats> event_rankings;
  std::map<std::string, RegStats> regs;
  int ranking_files = 0;
  int registration_files = 0;
  int event_jsonl_files = 0;
  int included_files = 0;
  std::string first_timestamp;
  std::string last_timestamp;

  auto mark_included = [&](const fs::path& csv) {
    included_files++;
    std::string ts = timestamp_from_filename(csv);
    if (ts.empty()) return;
    if (first_timestamp.empty() || ts < first_timestamp) first_timestamp = ts;
    if (last_timestamp.empty() || ts > last_timestamp) last_timestamp = ts;
  };

  for (const auto& csv : csvs) {
    auto r = read_ranking_csv(csv);
    if (!r.empty()) {
      merge_rankings(rankings, r);
      ranking_files++;
      mark_included(csv);
      continue;
    }

    auto reg = read_registration_csv(csv);
    if (!reg.empty()) {
      merge_regs(regs, reg);
      registration_files++;
      mark_included(csv);
    }
  }

  for (const auto& jsonl : data_jsonl_files(root)) {
    auto events = read_jsonl_event_rankings(jsonl);
    if (events.empty()) continue;
    merge_rankings(event_rankings, events);
    event_jsonl_files++;
  }

  bool event_records_available = !event_rankings.empty();
  if (event_records_available) {
    for (const auto& [agent, events] : event_rankings) {
      auto& dest = rankings[agent];
      dest.aih_events = events.aih_events;
      dest.legal_events = events.legal_events;
      dest.aih_time_sum = events.aih_time_sum;
      dest.legal_time_sum = events.legal_time_sum;
    }
  }

  if (rankings.empty() && regs.empty()) return false;

  struct RankRow {
    std::string agent;
    int rows;
    double aih_pct;
    double aih_events;
    double non_aih_events;
    double legal_events;
    double observed_plies;
    double avg_weighted;
    double avg_legal_time;
    double avg_aih_time;
    double avg_token_rate;
  };

  std::vector<RankRow> rows;
  for (const auto& [agent, s] : rankings) {
    if (s.rows <= 0) continue;
    double observed = event_records_available ? (s.aih_events + s.legal_events) : s.plies_sum;
    double non_aih_events = observed - s.aih_events;
    if (non_aih_events < 0.0 && non_aih_events > -0.000001) non_aih_events = 0.0;
    double aih_pct = observed > 0.0 ? 100.0 * s.aih_events / observed : 0.0;
    double avg_legal_time = s.legal_events > 0.0 ? s.legal_time_sum / s.legal_events : 0.0;
    double avg_aih_time = s.aih_events > 0.0 ? s.aih_time_sum / s.aih_events : 0.0;
    rows.push_back({
      agent,
      s.rows,
      aih_pct,
      s.aih_events,
      non_aih_events,
      s.legal_events,
      observed,
      s.weighted_sum / s.rows,
      avg_legal_time,
      avg_aih_time,
      s.token_rate_sum / s.rows
    });
  }

  std::sort(rows.begin(), rows.end(), [](const RankRow& a, const RankRow& b) {
    if (a.aih_pct != b.aih_pct) return a.aih_pct < b.aih_pct;
    if (a.avg_weighted != b.avg_weighted) return a.avg_weighted < b.avg_weighted;
    if (a.observed_plies != b.observed_plies) return a.observed_plies > b.observed_plies;
    if (a.rows != b.rows) return a.rows > b.rows;
    return a.agent < b.agent;
  });

  int ranking_rows = 0;
  double all_events = 0.0;
  for (const auto& [_, s] : rankings) {
    ranking_rows += s.rows;
    all_events += event_records_available ? (s.aih_events + s.legal_events) : s.plies_sum;
  }

  if (!out.parent_path().empty()) fs::create_directories(out.parent_path());
  std::ofstream html(out);
  if (!html) return false;

  auto fmt = [](double v, int places = 3) {
    if (std::isinf(v)) return std::string("inf");
    std::ostringstream ss;
    ss.setf(std::ios::fixed);
    ss.precision(places);
    ss << v;
    return ss.str();
  };

  html << "<!doctype html>\n<html lang=\"en\">\n<head>\n<meta charset=\"utf-8\">\n";
  html << "<title>AIH v5 CSV Analyzer</title>\n";
  html << "<style>"
       << ":root{color-scheme:light;background:#f7f8fa;color:#17202a}"
       << "body{font-family:Arial,sans-serif;margin:0;line-height:1.45;color:#17202a;background:#f7f8fa}"
       << "main{max-width:1220px;margin:0 auto;padding:28px}"
       << "h1{font-size:30px;margin:0 0 6px}.process-date{color:#52606d;font-size:15px;font-weight:400;margin-left:14px;white-space:nowrap}h2{margin-top:28px;font-size:20px}"
       << ".note{color:#52606d;margin:.25rem 0}.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(170px,1fr));gap:10px;margin:18px 0}"
       << ".metric{background:#fff;border:1px solid #d8dee6;border-radius:6px;padding:12px}.metric b{display:block;font-size:22px;margin-top:4px}"
       << "table{border-collapse:collapse;width:100%;margin-top:12px;background:#fff}th,td{border:1px solid #d8dee6;padding:8px;text-align:left;vertical-align:top}"
       << "th{background:#edf1f5;position:sticky;top:0}td.num{text-align:right;font-variant-numeric:tabular-nums}"
       << "code{background:#edf1f5;padding:.1rem .25rem;border-radius:3px}.good{color:#146c43;font-weight:700}.warn{color:#8a5a00;font-weight:700}.bad{color:#b42318;font-weight:700}"
       << "</style>\n</head>\n<body>\n<main>\n";
  html << "<h1>AIH v5 CSV Analyzer <span class=\"process-date\">Process date: "
       << html_escape(local_process_timestamp()) << "</span></h1>\n";
  html << "<h2>Agent Ranking</h2>\n";
  html << "<p class=\"note\">Two event classes are used per agent: AIH events and non-AIH events. Nevents is observed event count. AIH% = AIH events / Nevents. Legal and AIH seconds are averaged separately from event-level JSONL records when available.</p>\n";
  html << "<table><thead><tr><th>Rank</th><th class=\"num\">AIH%</th><th>Agent</th><th class=\"num\">CSV Rows</th><th class=\"num\">AIH Events</th><th class=\"num\">Non-AIH Events</th><th class=\"num\">Nevents</th><th class=\"num\">Avg Legal Sec</th><th class=\"num\">Avg AIH Sec</th><th class=\"num\">Avg Weighted</th><th class=\"num\">Avg Tokens/sec</th></tr></thead><tbody>\n";
  int rank = 1;
  for (const auto& r : rows) {
    const char* cls = r.aih_pct >= 60.0 ? "bad" : (r.aih_pct >= 20.0 ? "warn" : "good");
    html << "<tr><td class=\"num\">" << rank++
         << "</td><td class=\"num " << cls << "\">" << fmt(r.aih_pct)
         << "</td><td>" << html_escape(r.agent)
         << "</td><td class=\"num\">" << r.rows
         << "</td><td class=\"num\">" << fmt(r.aih_events, 1)
         << "</td><td class=\"num\">" << fmt(r.non_aih_events, 1)
         << "</td><td class=\"num\">" << fmt(r.observed_plies, 0)
         << "</td><td class=\"num\">" << fmt(r.avg_legal_time)
         << "</td><td class=\"num\">" << fmt(r.avg_aih_time)
         << "</td><td class=\"num\">" << fmt(r.avg_weighted)
         << "</td><td class=\"num\">" << fmt(r.avg_token_rate)
         << "</td></tr>\n";
  }
  html << "</tbody></table>\n";
  html << "<h2>Processor Run Summary</h2>\n";
  html << "<p class=\"note\">#csv files inc.: <code>" << included_files << "</code></p>\n";
  if (!first_timestamp.empty() && !last_timestamp.empty()) {
    html << "<p class=\"note\">Included CSV timestamp range: <code>" << html_escape(first_timestamp)
         << "</code> to <code>" << html_escape(last_timestamp) << "</code></p>\n";
  }
  html << "<p class=\"note\">Source directory: <code>" << html_escape((root / "data").string()) << "</code></p>\n";
  html << "<div class=\"grid\">\n";
  html << "<div class=\"metric\">Ranking CSV files<b>" << ranking_files << "</b></div>\n";
  html << "<div class=\"metric\">Event JSONL files<b>" << event_jsonl_files << "</b></div>\n";
  html << "<div class=\"metric\">Registration CSV files<b>" << registration_files << "</b></div>\n";
  html << "<div class=\"metric\">Ranking rows<b>" << ranking_rows << "</b></div>\n";
  html << "<div class=\"metric\">Agents ranked<b>" << rows.size() << "</b></div>\n";
  html << "<div class=\"metric\">Total events observed<b>" << fmt(all_events, 0) << "</b></div>\n";
  html << "</div>\n";
  html << "</main>\n</body>\n</html>\n";
  html.close();

  fs::path out_abs = fs::absolute(out).lexically_normal();
  fs::path latest_out = root / "AIH_V5_CSV_AGGREGATE_LATEST.html";
  fs::path latest_abs = fs::absolute(latest_out).lexically_normal();
  if (out_abs != latest_abs) fs::copy_file(out, latest_out, fs::copy_options::overwrite_existing);
  fs::path data_latest = root / "data" / "AIH_V5_CSV_AGGREGATE_LATEST.html";
  fs::path data_latest_abs = fs::absolute(data_latest).lexically_normal();
  if (out_abs != data_latest_abs) fs::copy_file(out, data_latest, fs::copy_options::overwrite_existing);

  std::cout << "aih_v5_html_bin: scanned CSV files " << csvs.size() << "\n";
  std::cout << "aih_v5_html_bin: included CSV files " << included_files << "\n";
  if (!first_timestamp.empty() && !last_timestamp.empty()) {
    std::cout << "aih_v5_html_bin: included CSV timestamp range " << first_timestamp << " to " << last_timestamp << "\n";
  }
  std::cout << "aih_v5_html_bin: ranking CSV files " << ranking_files << "\n";
  std::cout << "aih_v5_html_bin: registration CSV files " << registration_files << "\n";
  std::cout << "aih_v5_html_bin: wrote " << out << "\n";
  std::cout << "aih_v5_html_bin: wrote " << latest_out << "\n";
  std::cout << "aih_v5_html_bin: wrote " << data_latest << "\n";
  return true;
}

static std::map<std::string, AgentStats> read_jsonl_stats(const fs::path& jsonl) {
  std::map<std::string, AgentStats> stats;
  std::string cmd =
    "jq -r '.events[]? | [.model, "
    "(if (.transport_failure == true or .error == \"move_request_transport_failure\" or .response.status == \"request_failed\") then \"transport\" "
    "elif (.legal_by_rules == true or .legal == true) then \"legal\" "
    "else \"fail\" end)] | @tsv' " + shell_quote(jsonl);
  FILE* pipe = popen(cmd.c_str(), "r");
  if (!pipe) return stats;
  char buf[4096];
  while (fgets(buf, sizeof(buf), pipe)) {
    std::string line(buf);
    while (!line.empty() && (line.back() == '\n' || line.back() == '\r')) line.pop_back();
    size_t tab = line.find('\t');
    if (tab == std::string::npos) continue;
    std::string model = line.substr(0, tab);
    std::string kind = line.substr(tab + 1);
    if (model == "harness") continue;
    auto& s = stats[model];
    if (kind == "transport") s.transport++;
    else if (kind == "legal") s.legal++;
    else s.fail++;
  }
  pclose(pipe);
  return stats;
}

static std::set<std::string> read_jsonl_maxply_agents(const fs::path& jsonl) {
  std::set<std::string> agents;
  std::string cmd =
    "jq -r 'select((.max_plies // 0) > 0 and (.plies_played // -1) >= (.max_plies // 0)) | "
    "[.white_model, .black_model] | .[]? | select(. != null and . != \"harness\")' " + shell_quote(jsonl);
  FILE* pipe = popen(cmd.c_str(), "r");
  if (!pipe) return agents;
  char buf[4096];
  while (fgets(buf, sizeof(buf), pipe)) {
    std::string line(buf);
    while (!line.empty() && (line.back() == '\n' || line.back() == '\r')) line.pop_back();
    if (!line.empty()) agents.insert(line);
  }
  pclose(pipe);
  return agents;
}

static int read_jsonl_maxplies(const fs::path& jsonl) {
  std::string cmd = "jq -s 'map(.max_plies // 0) | max // 0' " + shell_quote(jsonl);
  FILE* pipe = popen(cmd.c_str(), "r");
  if (!pipe) return 0;
  char buf[256];
  std::string text;
  if (fgets(buf, sizeof(buf), pipe)) {
    text = buf;
  }
  pclose(pipe);
  try {
    return std::stoi(trim(text));
  } catch (...) {
    return 0;
  }
}

static int run_open_command(const char* cmd, const std::string& target) {
  pid_t pid = fork();
  if (pid == 0) {
    execlp(cmd, cmd, target.c_str(), static_cast<char*>(nullptr));
    _exit(127);
  }
  if (pid < 0) return 127;
  int status = 0;
  if (waitpid(pid, &status, 0) < 0) return 127;
  if (WIFEXITED(status)) return WEXITSTATUS(status);
  return 127;
}

static int run_gio_open(const std::string& uri) {
  pid_t pid = fork();
  if (pid == 0) {
    execlp("gio", "gio", "open", uri.c_str(), static_cast<char*>(nullptr));
    _exit(127);
  }
  if (pid < 0) return 127;
  int status = 0;
  if (waitpid(pid, &status, 0) < 0) return 127;
  if (WIFEXITED(status)) return WEXITSTATUS(status);
  return 127;
}

static void open_report(const fs::path& html) {
  const char* open_env = std::getenv("AIH_V5_OPEN_HTML_REPORT");
  if (open_env) {
    std::string value(open_env);
    if (value != "1" && value != "yes" && value != "true") return;
  }
  std::string path = fs::absolute(html).string();
  std::string uri = "file://" + path;
  int status = run_gio_open(uri);
  std::cout << "aih_v5_html_bin: gio status " << status << "\n";
  if (status == 0) return;
  status = run_open_command("firefox", uri);
  std::cout << "aih_v5_html_bin: firefox file-uri status " << status << "\n";
}

int main(int argc, char** argv) {
  fs::path root = fs::current_path();
  if (fs::exists(root / "tools" / "generate_aih_v5_html_report.cpp")) {
    // already at project root
  } else {
    std::cerr << "aih_v5_html: run from the AIH v5 project directory\n";
    return 2;
  }

  bool csv_all = false;
  fs::path csv_out = root / "AIH_V5_CSV_AGGREGATE_LATEST.html";
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--csv-all" || arg == "--csv-data" || arg == "--all-csv") {
      csv_all = true;
    } else if (csv_all) {
      csv_out = fs::path(arg);
    }
  }
  if (csv_all) {
    bool wrote = write_data_csv_report(root, csv_out);
    if (!wrote) {
      std::cerr << "aih_v5_html: no usable CSV records found in " << (root / "data") << "\n";
      return 2;
    }
    std::cout << "aih_v5_html_bin: opening " << csv_out << "\n";
    open_report(csv_out);
    return 0;
  }

  fs::path summary = latest_summary(root);
  if (summary.empty()) {
    std::cerr << "aih_v5_html: no summary file found\n";
    return 2;
  }
  int nruns = 1;
  fs::path explicit_out;
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg.rfind("--nruns=", 0) == 0 || arg.rfind("--n-runs=", 0) == 0 || arg.rfind("--runs=", 0) == 0) {
      nruns = std::max(1, std::stoi(arg.substr(arg.find('=') + 1)));
    } else {
      explicit_out = fs::path(arg);
    }
  }
  std::vector<fs::path> summaries = latest_summaries(root, nruns);
  if (summaries.empty()) summaries.push_back(summary);
  summary = summaries.front();
  std::string base = basename_no_summary(summary);
  std::regex ts_re(".*_([0-9]{8}_[0-9]{6})$");
  std::smatch ts_match;
  std::string timestamp = "unknown";
  if (std::regex_match(base, ts_match, ts_re)) timestamp = ts_match[1].str();
  fs::path timestamped_out = root / ("AIH_V5_PRELIMINARY_RESULTS_" + timestamp + (nruns > 1 ? "_nruns" + std::to_string(nruns) : "") + ".html");
  fs::path latest_out = root / "AIH_V5_PRELIMINARY_RESULTS_20260810.html";
  fs::path out = explicit_out.empty() ? timestamped_out : explicit_out;

  fs::create_directories(root / "data");
  fs::path data_html = root / "data" / (base + (nruns > 1 ? "_nruns" + std::to_string(nruns) : "") + ".html");
  auto regs = read_registration(root / "AIH_V5_REGISTRATION_STATUS.csv");
  std::map<std::string, AgentStats> stats;
  std::set<std::string> maxply_agents;
  int maxplies = 0;
  std::vector<fs::path> jsonls;
  for (const auto& item_summary : summaries) {
    std::string item_base = basename_no_summary(item_summary);
    fs::path item_jsonl = item_summary.parent_path() / (item_base + ".jsonl");
    if (!fs::exists(item_jsonl)) {
      fs::path alt = root / "data" / (item_base + ".jsonl");
      if (fs::exists(alt)) item_jsonl = alt;
    }
    if (!fs::exists(item_jsonl)) continue;
    jsonls.push_back(item_jsonl);
  }
  for (const auto& item_jsonl : jsonls) {
    auto item_stats = read_jsonl_stats(item_jsonl);
    for (const auto& [model, s] : item_stats) {
      auto& dest = stats[model];
      dest.legal += s.legal;
      dest.fail += s.fail;
      dest.transport += s.transport;
      dest.timeout += s.timeout;
    }
    auto item_maxply_agents = read_jsonl_maxply_agents(item_jsonl);
    maxply_agents.insert(item_maxply_agents.begin(), item_maxply_agents.end());
    maxplies = std::max(maxplies, read_jsonl_maxplies(item_jsonl));
  }
  if (jsonls.empty() && stats.empty() && regs.empty()) {
    std::cerr << "aih_v5_html: no jsonl or registration records found for requested runs\n";
    return 2;
  }

  struct Row { std::string model; int attempts; double aih; double legal; int reg_timeouts; int reg_attempts; double reg_elapsed; bool maxply; };
  std::vector<Row> rows;
  std::set<std::string> all_models;
  for (const auto& [model, _] : stats) all_models.insert(model);
  for (const auto& [model, _] : regs) all_models.insert(model);
  for (const auto& model : all_models) {
    AgentStats s;
    auto sit = stats.find(model);
    if (sit != stats.end()) s = sit->second;
    auto rit = regs.find(model);
    double reg_elapsed_for_model = 0.0;
    int reg_timeouts_for_model = 0;
    int reg_attempts_for_model = 0;
    if (rit != regs.end()) {
      s.legal += rit->second.passes;
      s.fail += rit->second.failures;
      reg_timeouts_for_model = rit->second.timeouts;
      reg_attempts_for_model = rit->second.passes + rit->second.failures;
      reg_elapsed_for_model = rit->second.elapsed;
    }
    s.fail += s.transport;
    int total = s.legal + s.fail;
    if (total <= 0) continue;
    rows.push_back({model, total, 100.0 * s.fail / total, 100.0 * s.legal / total, reg_timeouts_for_model, reg_attempts_for_model, reg_elapsed_for_model, maxply_agents.count(model) > 0});
  }
  std::sort(rows.begin(), rows.end(), [](const Row& a, const Row& b) {
    if (a.aih != b.aih) return a.aih < b.aih;
    if (a.legal != b.legal) return a.legal > b.legal;
    if (a.attempts != b.attempts) return a.attempts > b.attempts;
    if (a.reg_elapsed != b.reg_elapsed) return a.reg_elapsed < b.reg_elapsed;
    return a.model < b.model;
  });

  std::set<std::string> ranked;
  double reg_elapsed = 0.0;
  for (const auto& [_, r] : regs) reg_elapsed += r.elapsed;
  int plies = 0;
  for (const auto& [_, s] : stats) plies += s.legal + s.fail + s.transport;

  std::ofstream html(out);
  html << "<!doctype html>\n<html lang=\"en\">\n<head>\n<meta charset=\"utf-8\">\n";
  html << "<title>AIH v5 latest " << (nruns > 1 ? "runs" : "run") << "</title>\n";
  html << "<style>body{font-family:Arial,sans-serif;margin:2rem;line-height:1.45;color:#1f2933}"
       << "table{border-collapse:collapse;width:100%;margin-top:1rem}th,td{border:1px solid #cad2dc;padding:.55rem;text-align:left;vertical-align:top}"
       << "th{background:#eef2f6}.note{color:#52606d}code{background:#eef2f6;padding:.1rem .25rem;border-radius:3px}"
       << "td.num{text-align:right;font-variant-numeric:tabular-nums}.aih-high{color:#b42318;font-weight:700}.aih-mid{color:#8a5a00;font-weight:700}.aih-low{color:#146c43;font-weight:700}</style>\n";
  html << "</head>\n<body>\n<main>\n<h1>AIH v5 latest " << (nruns > 1 ? "runs" : "run") << "</h1>\n";
  size_t runs_aggregated = summaries.size();
  html << "<p class=\"note\">Runs requested: <code>" << nruns << "</code>; runs aggregated: <code>" << runs_aggregated << "</code></p>\n";
  html << "<p class=\"note\">Latest summary: <code>" << html_escape(summary.string()) << "</code></p>\n";
  html << "<p class=\"note\">JSONL files read: <code>" << jsonls.size() << "</code></p>\n";
  html << "<p class=\"note\">Registration source: <code>AIH_V5_REGISTRATION_STATUS.csv</code></p>\n";
  html << "<p class=\"note\">AIH% is the agent-output failure rate. Lower is better.</p>\n";
  html << "<h2>Agent Ranking, maxply=" << maxplies << "</h2>\n";
  html << "<table>\n<thead><tr><th>Rank</th><th class=\"num\">Attempts</th><th class=\"num\">AIH%</th><th class=\"num\">Legal%</th><th class=\"num\">Local?</th><th class=\"num\">Maxplys?</th><th>Agent Title</th><th>Reg. T.O.</th></tr></thead>\n<tbody>\n";
  int rank = 1;
  char buf[64];
  for (const auto& r : rows) {
    ranked.insert(r.model);
    const char* cls = r.aih >= 60.0 ? "aih-high" : (r.aih >= 20.0 ? "aih-mid" : "aih-low");
    std::snprintf(buf, sizeof(buf), "%06.3f", r.aih);
    std::string aih_s = buf;
    std::snprintf(buf, sizeof(buf), "%06.3f", r.legal);
    std::string legal_s = buf;
    std::string reg_to = "0/0 (000.00%)";
    if (r.reg_attempts > 0) {
      double pct = 100.0 * r.reg_timeouts / r.reg_attempts;
      std::snprintf(buf, sizeof(buf), "%d/%d (%06.2f%%)", r.reg_timeouts, r.reg_attempts, pct);
      reg_to = buf;
    }
    html << "<tr><td class=\"num\">" << rank++ << "</td><td class=\"num\">" << r.attempts << "</td><td class=\"num " << cls << "\">" << aih_s
         << "</td><td class=\"num\">" << legal_s << "</td><td class=\"num\">1</td><td class=\"num\">"
         << (r.maxply ? 1 : 0) << "</td><td>ollama "
         << html_escape(r.model) << "</td><td>" << reg_to << "</td></tr>\n";
  }
  html << "</tbody>\n</table>\n";
  html << "<h2>Efficiency Measurements</h2>\n<table>\n<thead><tr><th>Metric</th><th class=\"num\">Value</th></tr></thead>\n<tbody>\n";
  html << "<tr><td>Registration elapsed seconds</td><td class=\"num\">" << reg_elapsed << "</td></tr>\n";
  html << "<tr><td>Tournament plies observed</td><td class=\"num\">" << plies << "</td></tr>\n";
  html << "</tbody>\n</table>\n";
  html << "</main>\n</body>\n</html>\n";
  html.close();

  if (out != data_html) {
    fs::copy_file(out, data_html, fs::copy_options::overwrite_existing);
  }
  if (out != latest_out) {
    fs::copy_file(out, latest_out, fs::copy_options::overwrite_existing);
  }
  const char* live_html = std::getenv("AIH_V5_REPEAT_LIVE_HTML");
  if (live_html && *live_html) {
    fs::path live_out(live_html);
    fs::create_directories(live_out.parent_path());
    if (out != live_out) {
      fs::copy_file(out, live_out, fs::copy_options::overwrite_existing);
    }
    std::cout << "aih_v5_html_bin: wrote repeat live html " << live_out << "\n";
  }

  std::cout << "aih_v5_html_bin: wrote " << out << "\n";
  if (out != latest_out) std::cout << "aih_v5_html_bin: wrote " << latest_out << "\n";
  std::cout << "aih_v5_html_bin: wrote " << data_html << "\n";
  std::cout << "aih_v5_html_bin: opening " << out << "\n";
  open_report(out);
  return 0;
}
