#include <cerrno>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <algorithm>
#include <vector>

namespace fs = std::filesystem;

static std::string now_stamp() {
  const auto now = std::chrono::system_clock::now();
  const std::time_t t = std::chrono::system_clock::to_time_t(now);
  std::tm tm{};
  localtime_r(&t, &tm);
  char buf[32];
  std::strftime(buf, sizeof(buf), "%Y%m%d_%H%M%S", &tm);
  return buf;
}

static fs::path self_path() {
  std::vector<char> buf(4096);
  ssize_t n = readlink("/proc/self/exe", buf.data(), buf.size() - 1);
  if (n <= 0) return {};
  buf[static_cast<size_t>(n)] = '\0';
  return fs::path(buf.data());
}

static void usage(const char* argv0) {
  std::cerr
      << "Usage:\n"
      << "  " << argv0 << " MODEL\n"
      << "  " << argv0 << " WHITE_MODEL BLACK_MODEL\n\n"
      << "Runs bounded AIH v5 game work through the existing C++ engine.\n"
      << "One MODEL means self-play: MODEL as white and black.\n\n"
      << "Options:\n"
      << "  --nruns=N              global run count, default 1\n"
      << "  --uni-agent-play       each model plays both sides on its own board\n"
      << "  --local-agent-diagnostic local-only one-board-per-agent assumed-registration diagnostic\n"
      << "  --inter-agent-play     explicit two-agent play mode, default for two models\n"
      << "  --white-models=CSV     explicit white-side model list\n"
      << "  --black-models=CSV     explicit black-side model list\n"
      << "  --boards=N             board count, default inferred\n"
      << "  --board-concurrency=N  default 1\n"
      << "  --reference-config=ID  output reference config label\n"
      << "  --tourney-bracket      pass bracket mode to engine\n"
      << "  --board-awareness-probe pass board-awareness mode to engine\n"
      << "  --max-plies=N          default 8\n"
      << "  --move-timeout=N       default 30 seconds\n"
      << "  --stack-timeout=N      default 30 seconds\n"
      << "  --game-timeout=N       default 900 seconds\n"
      << "  --output-tokens=N      default 256\n"
      << "  --response-attempts=N  default 1\n"
      << "  --max-illegal=N        default 1\n"
      << "  --clue-mode=N          default 6\n"
      << "  --log-level=N          default 3\n"
      << "  --no-pre-reset         disable engine pre-board ollama stop\n";
}

static bool starts_with(const std::string& s, const std::string& prefix) {
  return s.rfind(prefix, 0) == 0;
}

static std::string value_after_eq(const std::string& arg) {
  const size_t pos = arg.find('=');
  return pos == std::string::npos ? "" : arg.substr(pos + 1);
}

static int csv_count(const std::string& csv) {
  if (csv.empty()) return 0;
  int count = 1;
  for (char c : csv) {
    if (c == ',') ++count;
  }
  return count;
}

int main(int argc, char** argv) {
  std::string white_models;
  std::string black_models;
  std::string boards;
  std::string board_concurrency = "1";
  std::string reference_config = "aih_v5_single_game_core_20260813";
  std::string max_plies = "8";
  std::string move_timeout = "30";
  std::string stack_timeout = "30";
  std::string game_timeout = "900";
  std::string output_tokens = "256";
  std::string response_attempts = "1";
  std::string max_illegal = "1";
  std::string clue_mode = "6";
  std::string log_level = "3";
  std::string nruns = "1";
  bool pre_reset = true;
  bool reset_all_ollama = false;
  bool local_agent_diagnostic = false;
  bool uni_agent_play = false;
  bool tourney_bracket = false;
  bool board_awareness_probe = false;
  std::vector<std::string> models;

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    auto next_value = [&](const char* opt) -> std::string {
      if (i + 1 >= argc) {
        std::cerr << "aih_v5_single_game: missing value for " << opt << "\n";
        std::exit(2);
      }
      return argv[++i];
    };
    if (arg == "--help" || arg == "-h") {
      usage(argv[0]);
      return 0;
    } else if (arg == "--mode") {
      (void)next_value("--mode");
    } else if (starts_with(arg, "--mode=")) {
      continue;
    } else if (arg == "--white-models") {
      white_models = next_value("--white-models");
    } else if (starts_with(arg, "--white-models=")) {
      white_models = value_after_eq(arg);
    } else if (arg == "--black-models") {
      black_models = next_value("--black-models");
    } else if (starts_with(arg, "--black-models=")) {
      black_models = value_after_eq(arg);
    } else if (arg == "--boards") {
      boards = next_value("--boards");
    } else if (starts_with(arg, "--boards=")) {
      boards = value_after_eq(arg);
    } else if (arg == "--board-concurrency") {
      board_concurrency = next_value("--board-concurrency");
    } else if (starts_with(arg, "--board-concurrency=")) {
      board_concurrency = value_after_eq(arg);
    } else if (arg == "--reference-config") {
      reference_config = next_value("--reference-config");
    } else if (starts_with(arg, "--reference-config=")) {
      reference_config = value_after_eq(arg);
    } else if (arg == "--tourney-bracket" || arg == "--tournament-bracket") {
      tourney_bracket = true;
    } else if (arg == "--board-awareness-probe") {
      board_awareness_probe = true;
    } else if (arg == "--referee" || arg == "--referee-models") {
      (void)next_value(arg.c_str());
    } else if (starts_with(arg, "--referee=") || starts_with(arg, "--referee-models=")) {
      continue;
    } else if (starts_with(arg, "--nruns=") || starts_with(arg, "--n-runs=") || starts_with(arg, "--runs=")) {
      nruns = value_after_eq(arg);
    } else if (arg == "--loops") {
      nruns = next_value("--loops");
    } else if (starts_with(arg, "--loops=")) {
      nruns = value_after_eq(arg);
    } else if (arg == "--uni-agent-play" || arg == "--self-play-each" || arg == "--self-play-by-agent") {
      uni_agent_play = true;
    } else if (arg == "--local-agent-diagnostic" || arg == "--local-agent-playability" || arg == "--assume-registration-local-agents") {
      uni_agent_play = true;
      local_agent_diagnostic = true;
      reset_all_ollama = true;
      nruns = "1";
    } else if (arg == "--inter-agent-play" || arg == "--cross-agent-play") {
      uni_agent_play = false;
    } else if (arg == "--mxply" || arg == "--max-plies") {
      max_plies = next_value(arg.c_str());
    } else if (starts_with(arg, "--max-plies=") || starts_with(arg, "--mxply=")) {
      max_plies = value_after_eq(arg);
    } else if (arg == "--move-timeout") {
      move_timeout = next_value("--move-timeout");
    } else if (starts_with(arg, "--move-timeout=")) {
      move_timeout = value_after_eq(arg);
    } else if (arg == "--stack-timeout") {
      stack_timeout = next_value("--stack-timeout");
    } else if (starts_with(arg, "--stack-timeout=")) {
      stack_timeout = value_after_eq(arg);
    } else if (arg == "--gmto" || arg == "--game-timeout") {
      game_timeout = next_value(arg.c_str());
    } else if (starts_with(arg, "--game-timeout=") || starts_with(arg, "--gmto=")) {
      game_timeout = value_after_eq(arg);
    } else if (arg == "--otkns" || arg == "--output-tokens") {
      output_tokens = next_value(arg.c_str());
    } else if (starts_with(arg, "--output-tokens=") || starts_with(arg, "--otkns=")) {
      output_tokens = value_after_eq(arg);
    } else if (arg == "--cnrtlm" || arg == "--response-attempts") {
      response_attempts = next_value(arg.c_str());
    } else if (starts_with(arg, "--response-attempts=") || starts_with(arg, "--cnrtlm=")) {
      response_attempts = value_after_eq(arg);
    } else if (arg == "--max-illegal") {
      max_illegal = next_value("--max-illegal");
    } else if (starts_with(arg, "--max-illegal=")) {
      max_illegal = value_after_eq(arg);
    } else if (arg == "--clue-mode") {
      clue_mode = next_value("--clue-mode");
    } else if (starts_with(arg, "--clue-mode=")) {
      clue_mode = value_after_eq(arg);
    } else if (arg == "--loglvl" || arg == "--log-level") {
      log_level = next_value(arg.c_str());
    } else if (starts_with(arg, "--log-level=") || starts_with(arg, "--loglvl=")) {
      log_level = value_after_eq(arg);
    } else if (arg == "--auto-output-tokens") {
      continue;
    } else if (arg == "--no-pre-reset") {
      pre_reset = false;
    } else if (!arg.empty() && arg[0] == '-') {
      std::cerr << "aih_v5_single_game: unknown option: " << arg << "\n";
      usage(argv[0]);
      return 2;
    } else {
      models.push_back(arg);
    }
  }

  if ((models.empty() && white_models.empty() && black_models.empty()) || (!uni_agent_play && models.size() > 2)) {
    usage(argv[0]);
    return 2;
  }
  auto has_cloud_provider = [](const std::string& value) {
    return starts_with(value, "openai:") || starts_with(value, "google:") ||
           starts_with(value, "gemini:") || starts_with(value, "anthropic:") ||
           starts_with(value, "codex:");
  };
  if (local_agent_diagnostic) {
    for (const std::string& model : models) {
      if (has_cloud_provider(model)) {
        std::cerr << "aih_v5_single_game: local-agent diagnostic rejects cloud/provider model: " << model << "\n";
        return 2;
      }
    }
    auto reject_cloud_csv = [&](const std::string& csv, const char* label) {
      size_t start = 0;
      while (start <= csv.size()) {
        size_t end = csv.find(',', start);
        std::string item = csv.substr(start, end == std::string::npos ? std::string::npos : end - start);
        item.erase(0, item.find_first_not_of(" \t\r\n"));
        item.erase(item.find_last_not_of(" \t\r\n") + 1);
        if (has_cloud_provider(item)) {
          std::cerr << "aih_v5_single_game: local-agent diagnostic rejects cloud/provider model in "
                    << label << ": " << item << "\n";
          std::exit(2);
        }
        if (end == std::string::npos) break;
        start = end + 1;
      }
    };
    reject_cloud_csv(white_models, "--white-models");
    reject_cloud_csv(black_models, "--black-models");
  }

  auto join_csv = [](const std::vector<std::string>& values) {
    std::string out;
    for (const std::string& value : values) {
      if (!out.empty()) out += ",";
      out += value;
    }
    return out;
  };

  if (white_models.empty() && black_models.empty()) {
    white_models = uni_agent_play ? join_csv(models) : models[0];
    black_models = uni_agent_play ? join_csv(models) : (models.size() == 1 ? models[0] : models[1]);
  } else if (uni_agent_play && black_models.empty()) {
    black_models = white_models;
  }
  if (black_models.empty()) black_models = white_models;
  if (white_models.empty()) white_models = black_models;
  if (boards.empty()) {
    boards = uni_agent_play ? std::to_string(std::max(1, csv_count(white_models))) : "1";
  }
  const fs::path exe = self_path();
  const fs::path root = exe.empty() ? fs::current_path() : exe.parent_path().parent_path();
  const fs::path engine = root / "qwen_ollama_chess_qt" / "qwen_ollama_chess_qt";
  if (!fs::exists(engine)) {
    std::cerr << "aih_v5_single_game: engine not found: " << engine << "\n";
    return 127;
  }

  const fs::path log_dir = root / "logs" / "single_game";
  std::error_code ec;
  fs::create_directories(log_dir, ec);
  if (ec) {
    std::cerr << "aih_v5_single_game: cannot create log dir " << log_dir << ": " << ec.message() << "\n";
    return 1;
  }

  std::string safe_white = white_models;
  std::string safe_black = black_models;
  for (char& c : safe_white) if (c == '/' || c == ':' || c == ' ') c = '_';
  for (char& c : safe_black) if (c == '/' || c == ':' || c == ' ') c = '_';
  auto shorten = [](const std::string& value) {
    constexpr size_t kMax = 80;
    if (value.size() <= kMax) return value;
    return value.substr(0, 48) + "_plus_" + std::to_string(csv_count(value)) + "_models";
  };
  const std::string log_name = local_agent_diagnostic
      ? "aih_v5_local_agent_diagnostic_" + now_stamp() + "_models" + std::to_string(csv_count(white_models)) + "_nruns" + nruns + ".log"
      : "aih_v5_single_game_" + now_stamp() + "_" + shorten(safe_white) + "_vs_" + shorten(safe_black) + "_nruns" + nruns + ".log";
  const fs::path log_path = log_dir / log_name;

  std::vector<std::string> args = {
    engine.string(),
    "--mode", "aichess",
    "--white-models", white_models,
    "--black-models", black_models,
    "--boards", boards,
    "--board-concurrency", board_concurrency,
    "--loops", nruns,
    "--referee", "harness",
    "--mxply", max_plies,
    "--cnrtlm", response_attempts,
    "--max-illegal", max_illegal,
    "--move-timeout", move_timeout,
    "--stack-timeout", stack_timeout,
    "--gmto", game_timeout,
    "--otkns", output_tokens,
    "--auto-output-tokens",
    "--loglvl", log_level,
    "--clue-mode", clue_mode,
    "--reference-config", reference_config
  };
  if (tourney_bracket) args.push_back("--tournament-bracket");
  if (board_awareness_probe) args.push_back("--board-awareness-probe");

  std::vector<char*> exec_args;
  exec_args.reserve(args.size() + 1);
  for (std::string& arg : args) exec_args.push_back(arg.data());
  exec_args.push_back(nullptr);

  std::ofstream header(log_path);
  if (!header) {
    std::cerr << "aih_v5_single_game: cannot write log: " << log_path << "\n";
    return 1;
  }
  header << "aih_v5_single_game\n"
         << "white_models=" << white_models << "\n"
         << "black_models=" << black_models << "\n"
         << "boards=" << boards << "\n"
         << "board_concurrency=" << board_concurrency << "\n"
         << "nruns=" << nruns << "\n"
         << "uni_agent_play=" << (uni_agent_play ? "1" : "0") << "\n"
         << "local_agent_diagnostic=" << (local_agent_diagnostic ? "1" : "0") << "\n"
         << "tourney_bracket=" << (tourney_bracket ? "1" : "0") << "\n"
         << "reference_config=" << reference_config << "\n"
         << "max_plies=" << max_plies << "\n"
         << "move_timeout=" << move_timeout << "\n"
         << "stack_timeout=" << stack_timeout << "\n"
         << "game_timeout=" << game_timeout << "\n"
         << "output_tokens=" << output_tokens << "\n"
         << "response_attempts=" << response_attempts << "\n"
         << "max_illegal=" << max_illegal << "\n"
         << "clue_mode=" << clue_mode << "\n"
         << "pre_reset=" << (pre_reset ? "1" : "0") << "\n"
         << "reset_all_ollama=" << (reset_all_ollama ? "1" : "0") << "\n\n";
  header.close();

  pid_t pid = fork();
  if (pid < 0) {
    std::cerr << "aih_v5_single_game: fork failed: " << std::strerror(errno) << "\n";
    return 1;
  }
  if (pid == 0) {
    int fd = open(log_path.c_str(), O_WRONLY | O_APPEND);
    if (fd >= 0) {
      dup2(fd, STDOUT_FILENO);
      dup2(fd, STDERR_FILENO);
      close(fd);
    }
    if (!pre_reset) setenv("AIH_V5_RESET_STACK_BEFORE_BOARD", "0", 1);
    if (reset_all_ollama) setenv("AIH_V5_RESET_ALL_OLLAMA_BEFORE_BOARD", "1", 1);
    setenv("AICHESS_TRACE_STRING_CHARS", "1048576", 0);
    setenv("AICHESS_OLLAMA_NUM_THREAD", "1", 0);
    execv(engine.c_str(), exec_args.data());
    std::cerr << "aih_v5_single_game: exec failed: " << std::strerror(errno) << "\n";
    _exit(127);
  }

  int status = 0;
  if (waitpid(pid, &status, 0) < 0) {
    std::cerr << "aih_v5_single_game: wait failed: " << std::strerror(errno) << "\n";
    return 1;
  }

  int exit_code = 1;
  if (WIFEXITED(status)) {
    exit_code = WEXITSTATUS(status);
  } else if (WIFSIGNALED(status)) {
    exit_code = 128 + WTERMSIG(status);
  }

  std::ofstream footer(log_path, std::ios::app);
  footer << "\naih_v5_single_game_exit_code=" << exit_code << "\n";
  footer.close();

  std::cout << "aih_v5_single_game: log " << log_path << "\n";
  std::cout << "aih_v5_single_game: engine exit " << exit_code << "\n";
  return exit_code;
}
