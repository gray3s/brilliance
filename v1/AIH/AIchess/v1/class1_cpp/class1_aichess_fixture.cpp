#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace {

const std::string kTestId = "aih_chess_class1_cpp_fixture_v1_20260715";
const std::string kStartFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
const std::filesystem::path kRunDir =
    "/home/sag/RPA2/myLLC/AI/brilliance/v1/AIH/AIchess/v1/runs";

const std::set<std::string> kLegalStartMoves = {
    "a2a3", "a2a4", "b2b3", "b2b4", "c2c3", "c2c4", "d2d3", "d2d4",
    "e2e3", "e2e4", "f2f3", "f2f4", "g2g3", "g2g4", "h2h3", "h2h4",
    "b1a3", "b1c3", "g1f3", "g1h3",
};

std::string jsonEscape(const std::string &value) {
    std::ostringstream out;
    for (const char ch : value) {
        switch (ch) {
        case '\\':
            out << "\\\\";
            break;
        case '"':
            out << "\\\"";
            break;
        case '\n':
            out << "\\n";
            break;
        case '\r':
            out << "\\r";
            break;
        case '\t':
            out << "\\t";
            break;
        default:
            out << ch;
            break;
        }
    }
    return out.str();
}

std::string timestamp(const char *format) {
    const auto now = std::chrono::system_clock::now();
    const std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::tm localTime{};
    localtime_r(&time, &localTime);
    std::ostringstream out;
    out << std::put_time(&localTime, format);
    return out.str();
}

std::string timestampWithMilliseconds() {
    const auto now = std::chrono::system_clock::now();
    const auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    std::ostringstream out;
    out << timestamp("%Y%m%d_%H%M%S") << "_"
        << std::setw(3) << std::setfill('0') << millis.count();
    return out.str();
}

std::string parseUci(const std::string &raw) {
    static const std::regex uci("\\b[a-h][1-8][a-h][1-8][qrbn]?\\b", std::regex::icase);
    std::smatch match;
    if (!std::regex_search(raw, match, uci)) {
        return "";
    }
    std::string move = match.str(0);
    std::transform(move.begin(), move.end(), move.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return move;
}

bool isHelpFlag(const std::string &arg) {
    return arg == "--help" || arg == "-h" || arg == "/help" || arg == "/?";
}

std::string legalMovesJsonArray() {
    std::ostringstream out;
    out << "[";
    bool first = true;
    for (const std::string &move : kLegalStartMoves) {
        if (!first) {
            out << ", ";
        }
        first = false;
        out << "\"" << move << "\"";
    }
    out << "]";
    return out.str();
}

std::string refereeVoteJson(const std::string &role, const std::string &move, bool legal, int indent) {
    const std::string pad(indent, ' ');
    const std::string pad2(indent + 2, ' ');
    std::ostringstream out;
    out << pad << "{\n";
    out << pad2 << "\"role\": \"" << role << "\",\n";
    out << pad2 << "\"stack_id\": \"deterministic_referee\",\n";
    out << pad2 << "\"move\": " << (move.empty() ? "null" : "\"" + move + "\"") << ",\n";
    out << pad2 << "\"legal\": " << (legal ? "true" : "false") << ",\n";
    out << pad2 << "\"reason\": \"" << (legal ? "move_in_fixed_start_legal_set" : "missing_or_illegal_fixed_start_move") << "\"\n";
    out << pad << "}";
    return out.str();
}

std::string buildResultJson(const std::string &rawResponse, double elapsedMs) {
    const std::string parsedMove = parseUci(rawResponse);
    const bool legal = !parsedMove.empty() && kLegalStartMoves.count(parsedMove) > 0;
    const std::string created = timestamp("%Y%m%d_%H%M%S");
    const int refereeLegalVotes = legal ? 3 : 0;
    const bool refereeMajorityLegal = refereeLegalVotes >= 2;

    std::ostringstream out;
    out << "{\n";
    out << "  \"test_id\": \"" << kTestId << "\",\n";
    out << "  \"created\": \"" << created << "\",\n";
    out << "  \"test_class\": \"class_1\",\n";
    out << "  \"test_class_name\": \"rule_bound_state_game_action_hallucination\",\n";
    out << "  \"test_family\": \"AIchess\",\n";
    out << "  \"implementation\": \"cpp17_bash_class1_fixture\",\n";
    out << "  \"config_id\": \"aichess_class1_one_board_one_agent_sides_three_referees_v1_20260715_2016\",\n";
    out << "  \"board_count\": 1,\n";
    out << "  \"boards\": [\"board_1\"],\n";
    out << "  \"role_map\": {\n";
    out << "    \"board_1_white_agent_1\": \"fixture_cpp_agent\",\n";
    out << "    \"board_1_black_agent_1\": \"fixture_cpp_agent_black_placeholder\",\n";
    out << "    \"board_1_referee_1\": \"deterministic_referee\",\n";
    out << "    \"board_1_referee_2\": \"deterministic_referee\",\n";
    out << "    \"board_1_referee_3\": \"deterministic_referee\"\n";
    out << "  },\n";
    out << "  \"position\": {\n";
    out << "    \"position_id\": \"standard_start_position\",\n";
    out << "    \"fen\": \"" << kStartFen << "\",\n";
    out << "    \"side_to_move\": \"white\",\n";
    out << "    \"legal_move_count\": " << kLegalStartMoves.size() << ",\n";
    out << "    \"legal_moves\": " << legalMovesJsonArray() << "\n";
    out << "  },\n";
    out << "  \"candidate_moves\": [\n";
    out << "    {\n";
    out << "      \"board_id\": \"board_1\",\n";
    out << "      \"role\": \"board_1_white_agent_1\",\n";
    out << "      \"stack_id\": \"fixture_cpp_agent\",\n";
    out << "      \"raw_response\": \"" << jsonEscape(rawResponse) << "\",\n";
    out << "      \"parsed_uci\": " << (parsedMove.empty() ? "null" : "\"" + parsedMove + "\"") << ",\n";
    out << "      \"legal\": " << (legal ? "true" : "false") << ",\n";
    out << "      \"latency_ms\": " << std::fixed << std::setprecision(3) << elapsedMs << "\n";
    out << "    }\n";
    out << "  ],\n";
    out << "  \"final_move\": {\n";
    out << "    \"side_to_move\": \"white\",\n";
    out << "    \"selection_rule\": \"single_player_agent_per_side_per_board\",\n";
    out << "    \"selected_move_by_board\": {\n";
    out << "      \"board_1\": " << (parsedMove.empty() ? "null" : "\"" + parsedMove + "\"") << "\n";
    out << "    },\n";
    out << "    \"legal_by_harness\": " << (legal ? "true" : "false") << "\n";
    out << "  },\n";
    out << "  \"referee_votes_by_board\": {\n";
    out << "    \"board_1\": [\n";
    out << refereeVoteJson("board_1_referee_1", parsedMove, legal, 6) << ",\n";
    out << refereeVoteJson("board_1_referee_2", parsedMove, legal, 6) << ",\n";
    out << refereeVoteJson("board_1_referee_3", parsedMove, legal, 6) << "\n";
    out << "    ]\n";
    out << "  },\n";
    out << "  \"metrics\": {\n";
    out << "    \"legal_final_move\": " << (legal ? "true" : "false") << ",\n";
    out << "    \"legal_candidate_move_rate\": " << (legal ? "1.0" : "0.0") << ",\n";
    out << "    \"selected_move_by_board\": {\n";
    out << "      \"board_1\": " << (parsedMove.empty() ? "null" : "\"" + parsedMove + "\"") << "\n";
    out << "    },\n";
    out << "    \"referee_agreement_by_board\": {\n";
    out << "      \"board_1\": true\n";
    out << "    },\n";
    out << "    \"referee_majority_legal_by_board\": {\n";
    out << "      \"board_1\": " << (refereeMajorityLegal ? "true" : "false") << "\n";
    out << "    },\n";
    out << "    \"unparseable_output_count\": " << (parsedMove.empty() ? 1 : 0) << ",\n";
    out << "    \"configuration_latency_ms\": " << std::fixed << std::setprecision(3) << elapsedMs << "\n";
    out << "  },\n";
    out << "  \"score\": " << (legal && refereeMajorityLegal ? 1 : 0) << ",\n";
    out << "  \"max_score\": 1,\n";
    out << "  \"errors\": " << (legal && refereeMajorityLegal ? "[]" : "[\"illegal_or_unparseable_move\"]") << ",\n";
    out << "  \"notes\": [\n";
    out << "    \"Class 1 C++ fixture provides the durable one-board three-referee run path.\",\n";
    out << "    \"This fixture uses a fixed start-position legal move set and deterministic referee votes.\"\n";
    out << "  ]\n";
    out << "}\n";
    return out.str();
}

} // namespace

int main(int argc, char **argv) {
    std::string rawResponse = "e2e4";
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--agent-response" && i + 1 < argc) {
            rawResponse = argv[++i];
        } else if (isHelpFlag(arg)) {
            std::cout << "Usage: class1_aichess_fixture [--agent-response TEXT]\n";
            return 0;
        }
    }

    const auto start = std::chrono::steady_clock::now();
    const auto end = std::chrono::steady_clock::now();
    const double elapsedMs = std::chrono::duration<double, std::milli>(end - start).count();
    const std::string json = buildResultJson(rawResponse, elapsedMs);

    std::filesystem::create_directories(kRunDir);
    const std::filesystem::path outPath =
        kRunDir / (kTestId + "_" + timestampWithMilliseconds() + ".json");
    std::ofstream outFile(outPath);
    if (!outFile) {
        std::cerr << "Failed to open output file: " << outPath << "\n";
        return 1;
    }
    outFile << json;
    outFile.close();

    std::cout << json;
    std::cout << outPath.string() << "\n";
    return 0;
}
