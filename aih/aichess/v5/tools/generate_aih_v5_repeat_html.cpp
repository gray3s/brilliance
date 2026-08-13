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

  fs::path out = data_dir / ("AIH_V5_REGISTRATION_AGGREGATE_" + timestamp() + ".html");
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
  html << "<p class=\"note\">Input source: registration_status_run_*.csv files only.</p>\n";
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
  html << "</tbody></table>\n</main></body></html>\n";
  html.close();

  fs::path latest = data_dir / "AIH_V5_REGISTRATION_AGGREGATE_LATEST.html";
  fs::copy_file(out, latest, fs::copy_options::overwrite_existing);
  std::cout << "aih_v5_repeat_html: wrote " << out << "\n";
  std::cout << "aih_v5_repeat_html: wrote " << latest << "\n";
  open_report(out);
  return 0;
}
