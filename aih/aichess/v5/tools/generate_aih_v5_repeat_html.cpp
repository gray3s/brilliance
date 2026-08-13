#include <algorithm>
#include <chrono>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

namespace fs = std::filesystem;

struct AgentAggregate {
  int tested = 0;
  int pass = 0;
  int fail = 0;
  int timeout = 0;
  double elapsed = 0.0;
  double move_elapsed = 0.0;
  int move_attempts = 0;
};

struct DiagnosticRow {
  std::string file;
  std::string run_time;
  std::string game_mode;
  std::string model;
  std::string result;
  std::string termination;
  std::string complete;
  int plies = 0;
  int legal = 0;
  int fail = 0;
  int neutral = 0;
  int rejected = 0;
  double seconds = 0.0;
};

static std::string timestamp() {
  auto now = std::chrono::system_clock::now();
  std::time_t t = std::chrono::system_clock::to_time_t(now);
  std::tm tm{};
  localtime_r(&t, &tm);
  std::ostringstream out;
  out << std::put_time(&tm, "%Y%m%d_%H%M%S");
  return out.str();
}

static std::string trim(std::string s) {
  while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) s.erase(s.begin());
  while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) s.pop_back();
  return s;
}

static std::string html_escape(const std::string& s) {
  std::string out;
  for (char c : s) {
    if (c == '&') out += "&amp;";
    else if (c == '<') out += "&lt;";
    else if (c == '>') out += "&gt;";
    else out += c;
  }
  return out;
}

static std::vector<std::string> split_csv_line(const std::string& line) {
  std::vector<std::string> fields;
  std::string cur;
  bool quote = false;
  for (char c : line) {
    if (c == '"') quote = !quote;
    else if (c == ',' && !quote) {
      fields.push_back(cur);
      cur.clear();
    } else {
      cur += c;
    }
  }
  fields.push_back(cur);
  return fields;
}

static std::vector<std::string> split_pipe_row(const std::string& line) {
  std::vector<std::string> fields;
  std::string cur;
  for (char c : line) {
    if (c == '|') {
      fields.push_back(trim(cur));
      cur.clear();
    } else {
      cur += c;
    }
  }
  fields.push_back(trim(cur));
  if (!fields.empty() && fields.front().empty()) fields.erase(fields.begin());
  if (!fields.empty() && fields.back().empty()) fields.pop_back();
  return fields;
}

static bool is_markdown_separator_row(const std::vector<std::string>& fields) {
  if (fields.empty()) return false;
  for (const auto& f : fields) {
    bool has_dash = false;
    for (char c : f) {
      if (c == '-') has_dash = true;
      else if (c != ':' && !std::isspace(static_cast<unsigned char>(c))) return false;
    }
    if (!has_dash) return false;
  }
  return true;
}

static int parse_int_field(const std::string& s) {
  try { return std::stoi(trim(s)); } catch (...) { return 0; }
}

static double parse_double_field(const std::string& s) {
  try { return std::stod(trim(s)); } catch (...) { return 0.0; }
}

static std::string compact_model_name(std::string s) {
  const std::string latest = ":latest";
  size_t pos = 0;
  while ((pos = s.find(latest, pos)) != std::string::npos) {
    s.erase(pos, latest.size());
  }
  return s;
}

static fs::path project_root() {
  fs::path cwd = fs::current_path();
  if (fs::exists(cwd / "aih_v5.sh")) return cwd;
  if (fs::exists(cwd.parent_path() / "aih_v5.sh")) return cwd.parent_path();
  return cwd;
}

static void read_registration_csv(const fs::path& csv, std::map<std::string, AgentAggregate>& agents) {
  std::ifstream in(csv);
  if (!in) return;
  std::string line;
  std::getline(in, line);
  while (std::getline(in, line)) {
    auto f = split_csv_line(line);
    if (f.size() < 5) continue;
    std::string agent = trim(f[0]);
    std::string status = trim(f[1]);
    std::string reason = f.size() > 2 ? f[2] : "";
    double elapsed = 0.0;
    try { elapsed = std::stod(f[3]); } catch (...) {}
    auto& a = agents[agent];
    a.tested++;
    a.elapsed += elapsed;
    if (f.size() >= 7) {
      try {
        double avg_move = trim(f[5]).empty() ? 0.0 : std::stod(f[5]);
        int move_attempts = trim(f[6]).empty() ? 0 : std::stoi(f[6]);
        if (move_attempts > 0) {
          a.move_elapsed += avg_move * move_attempts;
          a.move_attempts += move_attempts;
        }
      } catch (...) {
      }
    }
    if (status == "pass") a.pass++;
    else {
      a.fail++;
      if (reason.find("timed out") != std::string::npos) a.timeout++;
    }
  }
}

static void read_diagnostic_summary(const fs::path& root, const fs::path& summary, std::vector<DiagnosticRow>& rows) {
  std::ifstream in(summary);
  if (!in) return;

  std::string line;
  std::string run_time;
  std::string game_mode;
  bool in_table = false;
  while (std::getline(in, line)) {
    if (line.rfind("curDateTime:", 0) == 0) {
      run_time = trim(line.substr(std::string("curDateTime:").size()));
      continue;
    }
    if (line.rfind("GameMode:", 0) == 0) {
      game_mode = trim(line.substr(std::string("GameMode:").size()));
      continue;
    }
    if (line.rfind("| Model |", 0) == 0) {
      in_table = true;
      continue;
    }
    if (!in_table || line.empty() || line[0] != '|') continue;

    auto f = split_pipe_row(line);
    if (is_markdown_separator_row(f) || f.size() < 11) continue;

    DiagnosticRow row;
    row.file = fs::relative(summary, root).string();
    row.run_time = run_time;
    row.game_mode = game_mode;
    row.model = compact_model_name(f[0]);
    row.result = f[2];
    row.termination = f[3];
    row.complete = f[4];
    row.plies = parse_int_field(f[5]);
    row.legal = parse_int_field(f[6]);
    row.fail = parse_int_field(f[7]);
    row.neutral = parse_int_field(f[8]);
    row.rejected = parse_int_field(f[9]);
    row.seconds = parse_double_field(f[10]);
    rows.push_back(row);
  }
}

static std::vector<DiagnosticRow> read_diagnostic_summaries(const fs::path& root) {
  std::vector<DiagnosticRow> rows;
  fs::path runs_dir = root / "runs";
  if (!fs::exists(runs_dir) || !fs::is_directory(runs_dir)) return rows;

  for (const auto& ent : fs::recursive_directory_iterator(runs_dir)) {
    if (!ent.is_regular_file()) continue;
    std::string name = ent.path().filename().string();
    if (name.size() >= 11 && name.rfind("_summary.md") == name.size() - 11) {
      read_diagnostic_summary(root, ent.path(), rows);
    }
  }
  std::sort(rows.begin(), rows.end(), [](const DiagnosticRow& a, const DiagnosticRow& b) {
    if (a.run_time != b.run_time) return a.run_time > b.run_time;
    return a.model < b.model;
  });
  return rows;
}

static std::string term_label(const std::string& term) {
  if (term == "dpl") return "draw by configured ply limit";
  if (term == "b.fim") return "black invalid move";
  if (term == "w.fim") return "white invalid move";
  if (term == "b.fir") return "black irrelevant return";
  if (term == "w.fir") return "white irrelevant return";
  if (term == "b.ftr") return "black transport failure";
  if (term == "w.ftr") return "white transport failure";
  return term;
}

static void write_diagnostic_html(const fs::path& root, const fs::path& out, const std::vector<DiagnosticRow>& diagnostics) {
  std::ofstream html(out);
  html << "<!doctype html><html lang=\"en\"><head><meta charset=\"utf-8\">\n";
  html << "<title>AIH v5 diagnostic aggregate</title>\n";
  html << "<style>body{font-family:Arial,sans-serif;margin:2rem;line-height:1.45;color:#1f2933}"
       << "table{border-collapse:collapse;width:100%;margin-top:1rem}th,td{border:1px solid #cad2dc;padding:.55rem;text-align:left;vertical-align:top}"
       << "th{background:#eef2f6}td.num{text-align:right;font-variant-numeric:tabular-nums}.note{color:#52606d}"
       << "code,pre{background:#eef2f6;border-radius:3px}code{padding:.1rem .25rem}pre{padding:.75rem;overflow:auto}"
       << "th.num,td.num{white-space:nowrap;width:1%;padding-left:.35rem;padding-right:.35rem}"
       << "th.avgsec,td.avgsec{width:11rem;padding-left:.55rem;padding-right:.55rem}"
       << ".ok{color:#166534;font-weight:700}.fail{color:#991b1b;font-weight:700}</style>\n";
  html << "</head><body><main>\n<h1>AIH v5 Diagnostic Aggregate</h1>\n";
  html << "<p class=\"note\">Generated from <code>v5/runs/**/*_summary.md</code>. "
       << "This page covers diagnostic and single-game summary modes, including local-agent/inter-agent diagnostic runs.</p>\n";
  html << "<p class=\"note\">Basic registration CSV aggregate: <a href=\"AIH_V5_REGISTRATION_AGGREGATE_LATEST.html\">AIH_V5_REGISTRATION_AGGREGATE_LATEST.html</a></p>\n";
  html << "<p class=\"note\">Repository source root: <code>" << html_escape(root.string()) << "</code></p>\n";

  int total = 0;
  int ok = 0;
  int failed = 0;
  int transport = 0;
  int invalid = 0;
  int irrelevant = 0;
  double seconds = 0.0;
  for (const auto& row : diagnostics) {
    total++;
    seconds += row.seconds;
    if (row.result == "ok.ply") ok++;
    else failed++;
    if (row.termination == "w.ftr" || row.termination == "b.ftr") transport++;
    if (row.termination == "w.fim" || row.termination == "b.fim") invalid++;
    if (row.termination == "w.fir" || row.termination == "b.fir") irrelevant++;
  }

  html << "<h2>Summary</h2>\n";
  html << "<table><thead><tr><th class=\"num\">Rows</th><th class=\"num\">Reached Ply Limit</th><th class=\"num\">Failed</th>"
       << "<th class=\"num\">Transport</th><th class=\"num\">Invalid</th><th class=\"num\">Irrelevant</th><th class=\"num avgsec\">Total Sec</th></tr></thead><tbody>\n";
  html << "<tr><td class=\"num\">" << total << "</td><td class=\"num\">" << ok << "</td><td class=\"num\">" << failed << "</td>"
       << "<td class=\"num\">" << transport << "</td><td class=\"num\">" << invalid << "</td><td class=\"num\">" << irrelevant << "</td>"
       << "<td class=\"num avgsec\">" << std::fixed << std::setprecision(3) << seconds << "</td></tr>\n";
  html << "</tbody></table>\n";

  html << "<h2>Diagnostic Rows</h2>\n";
  html << "<table><thead><tr><th>Run Time</th><th>Mode</th><th>Model</th><th>Result</th><th>Termination</th>"
       << "<th class=\"num\">Plies</th><th class=\"num\">Legal</th><th class=\"num\">Fail</th><th class=\"num\">Rejected</th>"
       << "<th class=\"num avgsec\">Sec</th><th>Source</th></tr></thead><tbody>\n";
  for (const auto& row : diagnostics) {
    std::string cls = row.result == "ok.ply" ? "ok" : "fail";
    html << "<tr><td>" << html_escape(row.run_time) << "</td><td>" << html_escape(row.game_mode) << "</td>"
         << "<td>" << html_escape(row.model) << "</td><td class=\"" << cls << "\">" << html_escape(row.result) << "</td>"
         << "<td>" << html_escape(term_label(row.termination)) << "</td>"
         << "<td class=\"num\">" << row.plies << "</td><td class=\"num\">" << row.legal << "</td>"
         << "<td class=\"num\">" << row.fail << "</td><td class=\"num\">" << row.rejected << "</td>"
         << "<td class=\"num avgsec\">" << std::fixed << std::setprecision(3) << row.seconds << "</td>"
         << "<td><code>" << html_escape(row.file) << "</code></td></tr>\n";
  }
  html << "</tbody></table>\n</main></body></html>\n";
}

static int run_open_command(const char* cmd, const std::string& target, bool wait_for_exit = true) {
  pid_t pid = fork();
  if (pid == 0) {
    execlp(cmd, cmd, target.c_str(), static_cast<char*>(nullptr));
    _exit(127);
  }
  if (pid < 0) return 127;
  if (!wait_for_exit) return 0;
  int status = 0;
  if (waitpid(pid, &status, 0) < 0) return 127;
  if (WIFEXITED(status)) return WEXITSTATUS(status);
  return 127;
}

static int run_firefox_mode(const char* mode, const std::string& target, bool wait_for_exit = false) {
  pid_t pid = fork();
  if (pid == 0) {
    execlp("firefox", "firefox", mode, target.c_str(), static_cast<char*>(nullptr));
    _exit(127);
  }
  if (pid < 0) return 127;
  if (!wait_for_exit) return 0;
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
  std::string path = fs::absolute(html).string();
  std::string uri = "file://" + path;
  int status = run_firefox_mode("--new-tab", uri, true);
  std::cout << "aih_v5_repeat_html: firefox new-tab file-uri status " << status << "\n";
  if (status == 0) return;
  status = run_firefox_mode("--new-window", uri, true);
  std::cout << "aih_v5_repeat_html: firefox new-window file-uri status " << status << "\n";
  if (status == 0) return;
  status = run_firefox_mode("--new-tab", path, true);
  std::cout << "aih_v5_repeat_html: firefox new-tab path status " << status << "\n";
  if (status == 0) return;
  status = run_firefox_mode("--new-window", path, true);
  std::cout << "aih_v5_repeat_html: firefox new-window path launch status " << status << "\n";
  if (status == 0) return;
  status = run_open_command("xdg-open", uri);
  std::cout << "aih_v5_repeat_html: xdg-open status " << status << "\n";
  if (status == 0) return;
  status = run_gio_open(uri);
  std::cout << "aih_v5_repeat_html: gio status " << status << "\n";
}

int main(int argc, char** argv) {
  fs::path root = project_root();
  fs::current_path(root);
  fs::path data_dir = root / "data";
  int nruns = 5;

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg.rfind("--nruns=", 0) == 0 || arg.rfind("--n-runs=", 0) == 0 || arg.rfind("--runs=", 0) == 0) {
      nruns = std::stoi(arg.substr(arg.find('=') + 1));
    } else if (arg.rfind("--data-dir=", 0) == 0) {
      data_dir = fs::path(arg.substr(arg.find('=') + 1));
    }
  }
  if (!fs::exists(data_dir) || !fs::is_directory(data_dir)) {
    std::cerr << "aih_v5_repeat_html: data directory not found: " << data_dir << "\n";
    return 2;
  }
  if (nruns < 0) {
    std::cerr << "aih_v5_repeat_html: nruns must be >= 0; use --nruns=0 for all saved run CSVs\n";
    return 2;
  }

  std::vector<fs::path> csvs;
  for (const auto& ent : fs::directory_iterator(data_dir)) {
    if (!ent.is_regular_file()) continue;
    std::string name = ent.path().filename().string();
    if (name.rfind("registration_status_run_", 0) == 0 && ent.path().extension() == ".csv") {
      csvs.push_back(ent.path());
    }
  }
  std::sort(csvs.begin(), csvs.end());
  size_t saved_csv_count = csvs.size();
  if (nruns > 0 && static_cast<int>(csvs.size()) > nruns) {
    csvs.erase(csvs.begin() + nruns, csvs.end());
  }
  if (csvs.empty()) {
    std::cerr << "aih_v5_repeat_html: no registration_status_run_*.csv files in " << data_dir << "\n";
    return 2;
  }

  std::map<std::string, AgentAggregate> agents;
  for (const auto& csv : csvs) read_registration_csv(csv, agents);
  std::vector<DiagnosticRow> diagnostics = read_diagnostic_summaries(root);

  struct Row { std::string agent; AgentAggregate agg; };
  std::vector<Row> rows;
  for (const auto& [agent, agg] : agents) rows.push_back({agent, agg});
  std::sort(rows.begin(), rows.end(), [](const Row& a, const Row& b) {
    double aa = a.agg.tested ? static_cast<double>(a.agg.fail) / a.agg.tested : 1.0;
    double ba = b.agg.tested ? static_cast<double>(b.agg.fail) / b.agg.tested : 1.0;
    if (aa != ba) return aa < ba;
    double ap = a.agg.tested ? static_cast<double>(a.agg.pass) / a.agg.tested : -1.0;
    double bp = b.agg.tested ? static_cast<double>(b.agg.pass) / b.agg.tested : -1.0;
    if (ap != bp) return ap > bp;
    if (a.agg.timeout != b.agg.timeout) return a.agg.timeout < b.agg.timeout;
    return a.agent < b.agent;
  });

  fs::path out = data_dir / "AIH_V5_REGISTRATION_AGGREGATE_LATEST.html";
  fs::path diagnostic_latest = data_dir / "AIH_V5_DIAGNOSTIC_AGGREGATE_LATEST.html";
  fs::path root_csv_latest = root / "AIH_V5_CSV_AGGREGATE_LATEST.html";
  fs::path root_diagnostic_latest = root / "AIH_V5_DIAGNOSTIC_AGGREGATE_LATEST.html";
  std::ofstream html(out);
  html << "<!doctype html><html lang=\"en\"><head><meta charset=\"utf-8\">\n";
  html << "<title>AIH v5 registration aggregate</title>\n";
  html << "<style>body{font-family:Arial,sans-serif;margin:2rem;line-height:1.45;color:#1f2933}"
       << "table{border-collapse:collapse;width:100%;margin-top:1rem}th,td{border:1px solid #cad2dc;padding:.55rem;text-align:left;vertical-align:top}"
       << "th{background:#eef2f6}td.num{text-align:right;font-variant-numeric:tabular-nums}.note{color:#52606d}"
       << "code,pre{background:#eef2f6;border-radius:3px}code{padding:.1rem .25rem}pre{padding:.75rem;overflow:auto}"
       << "th.num,td.num{white-space:nowrap;width:1%;padding-left:.35rem;padding-right:.35rem}"
       << "th.avgsec,td.avgsec{width:13rem;padding-left:.55rem;padding-right:.55rem}"
       << ".timing{display:grid;grid-template-columns:7ch 1ch 8ch;gap:.35rem;justify-content:end;font-variant-numeric:tabular-nums}"
       << ".timing span{text-align:right}</style>\n";
  html << "</head><body><main>\n<h1>AIH v5 Registration Aggregate</h1>\n";
  html << "<p class=\"note\">Published artifact date: 2026-08-11</p>\n";
  html << "<p class=\"note\">Project goals: <a href=\"https://github.com/gray3s/brilliance/blob/main/aih/aichess/v5/AIH_V5_PROJECT_GOALS.md\">AIH_V5_PROJECT_GOALS.md</a></p>\n";
  html << "<p class=\"note\">Project implementation plan: <a href=\"https://github.com/gray3s/brilliance/blob/main/aih/aichess/v5/AIH_V5_IMPLEMENTATION_PLAN.md\">AIH_V5_IMPLEMENTATION_PLAN.md</a></p>\n";
  html << "<p class=\"note\">Current aggregate HTML: <a href=\"https://htmlpreview.github.io/?https://github.com/gray3s/brilliance/blob/main/aih/aichess/v5/AIH_V5_REGISTRATION_AGGREGATE_LATEST.html\">AIH_V5_REGISTRATION_AGGREGATE_LATEST.html</a></p>\n";
  html << "<h2>Launch and Relaunch Instructions</h2>\n";
  html << "<p class=\"note\">Run commands from the AIH v5 project directory.</p>\n";
  html << "<pre><code>cd ~/RPA2/myLLC/AI/brilliance/aih/aichess/v5\n";
  html << "./bin/aih_v5\n";
  html << "./bin/aih_v5_repeat_html</code></pre>\n";
  html << "<p class=\"note\"><code>./bin/aih_v5</code> launches a general AIH v5 run. ";
  html << "<code>./bin/aih_v5_repeat_html</code> rebuilds and opens this HTML aggregate page.</p>\n";
  html << "<p class=\"note\">Data section: v5/data</p>\n";
  html << "<p class=\"note\">Input source: registration_status_run_*.csv files.</p>\n";
  html << "<p class=\"note\">Diagnostic mode aggregate: <a href=\"AIH_V5_DIAGNOSTIC_AGGREGATE_LATEST.html\">AIH_V5_DIAGNOSTIC_AGGREGATE_LATEST.html</a></p>\n";
  html << "<p class=\"note\">Runs aggregated: " << csvs.size() << "</p>\n";
  double saved_csv_pct = saved_csv_count == 0 ? 0.0 : 100.0 * csvs.size() / saved_csv_count;
  size_t start_from_beginning = 0;
  char csv_pct_buf[32];
  std::snprintf(csv_pct_buf, sizeof(csv_pct_buf), "%.1f", saved_csv_pct);
  html << "<p class=\"note\">Registration files aggregated: " << csvs.size() << "/" << saved_csv_count
       << " (" << csv_pct_buf << "%) of saved run CSVs, starting at #" << start_from_beginning << " from the beginning</p>\n";
  html << "<p class=\"note\">Coverage% means: Agent in this % of CSVs.</p>\n";
  html << "<table><thead><tr><th>Rank</th><th>Agent</th><th class=\"num\">AIH%</th><th class=\"num\">Attempts</th><th class=\"num\">Total Runs</th><th class=\"num\">Coverage%</th><th class=\"num\">Pass%</th><th class=\"num\">Fail</th><th class=\"num\">Timeout</th><th class=\"num\">Timeout%</th><th class=\"num avgsec\">Avg Sec</th></tr></thead><tbody>\n";
  int rank = 1;
  char buf[64];
  for (const auto& row : rows) {
    const auto& a = row.agg;
    double pass_pct = a.tested ? 100.0 * a.pass / a.tested : 0.0;
    double aih_pct = a.tested ? 100.0 * a.fail / a.tested : 0.0;
    double timeout_pct = a.tested ? 100.0 * a.timeout / a.tested : 0.0;
    double coverage_pct = csvs.empty() ? 0.0 : 100.0 * a.tested / csvs.size();
    double avg_move = a.move_attempts ? a.move_elapsed / a.move_attempts : 0.0;
    std::snprintf(buf, sizeof(buf), "%.1f", aih_pct);
    std::string aih_s = buf;
    double avg_elapsed = a.tested ? a.elapsed / a.tested : 0.0;
    std::snprintf(buf, sizeof(buf), "%.1f", pass_pct);
    std::string pass_s = buf;
    std::snprintf(buf, sizeof(buf), "%.1f", timeout_pct);
    std::string timeout_s = buf;
    std::snprintf(buf, sizeof(buf), "%.1f", coverage_pct);
    std::string coverage_s = buf;
    std::ostringstream timing_out;
    if (a.move_attempts > 0) {
      timing_out << "<span class=\"timing\"><span>" << std::fixed << std::setprecision(3) << avg_elapsed
                 << "</span><span>/</span><span>" << avg_move << "</span></span>";
    } else {
      timing_out << "<span class=\"timing\"><span>" << std::fixed << std::setprecision(3) << avg_elapsed
                 << "</span><span>/</span><span>n/a</span></span>";
    }
    std::string timing_s = timing_out.str();
    html << "<tr><td class=\"num\">" << rank++ << "</td><td>" << html_escape(row.agent) << "</td><td class=\"num\">" << aih_s << "</td>"
         << "<td class=\"num\">" << a.tested << "</td><td class=\"num\">" << csvs.size() << "</td>"
         << "<td class=\"num\">" << coverage_s << "</td><td class=\"num\">" << pass_s << "</td>"
         << "<td class=\"num\">" << a.fail << "</td><td class=\"num\">" << a.timeout << "</td>"
         << "<td class=\"num\">" << timeout_s << "</td>"
         << "<td class=\"num avgsec\">" << timing_s << "</td></tr>\n";
  }
  html << "</tbody></table>\n";
  html << "<h2>Input Files</h2>\n";
  html << "<table><thead><tr><th>Relative Path Spec</th><th class=\"num\">CSV Files Processed</th></tr></thead><tbody>\n";
  html << "<tr><td><code>v5/data/registration_status_run_*.csv</code></td><td class=\"num\">" << csvs.size() << "</td></tr>\n";
  html << "<tr><td><code>v5/runs/**/*_summary.md</code></td><td class=\"num\">" << diagnostics.size() << "</td></tr>\n";
  html << "</tbody></table>\n</main></body></html>\n";
  html.close();

  write_diagnostic_html(root, diagnostic_latest, diagnostics);
  fs::copy_file(out, root_csv_latest, fs::copy_options::overwrite_existing);
  fs::copy_file(diagnostic_latest, root_diagnostic_latest, fs::copy_options::overwrite_existing);
  std::cout << "aih_v5_repeat_html: wrote " << out << "\n";
  std::cout << "aih_v5_repeat_html: wrote " << diagnostic_latest << "\n";
  std::cout << "aih_v5_repeat_html: wrote " << root_csv_latest << "\n";
  std::cout << "aih_v5_repeat_html: wrote " << root_diagnostic_latest << "\n";
  open_report(out);
  return 0;
}
