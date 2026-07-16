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

const std::string kTestId = "aih_chess_class1_basic_cpp_fixture_v1_20260715";
const std::string kConfigId = "aichess_class1_basic_one_board_one_agent_sides_one_referee_v1_20260715_2110";
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
        case '\\': out << "\\\\"; break;
        case '"': out << "\\\""; break;
        case '\n': out << "\\n"; break;
        case '\r': out << "\\r"; break;
        case '\t': out << "\\t"; break;
        default: out << ch; break;
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

std::string nullableMove(const std::string &move) {
    return move.empty() ? "null" : "\"" + move + "\"";
}

bool isHelpFlag(const std::string &arg) {
    return arg == "--help" || arg == "-h" || arg == "/help" || arg == "/?";
}

struct PlyResult {
    int ply;
    std::string boardId;
    std::string side;
    std::string role;
    std::string rawResponse;
    std::string parsedMove;
    bool legal;
    std::string refereeRole;
    std::string refereeReason;
};

std::string plyJson(const PlyResult &ply, int indent) {
    const std::string pad(indent, ' ');
    const std::string pad2(indent + 2, ' ');
    std::ostringstream out;
    out << pad << "{\n";
    out << pad2 << "\"ply\": " << ply.ply << ",\n";
    out << pad2 << "\"board_id\": \"" << ply.boardId << "\",\n";
    out << pad2 << "\"side_to_move\": \"" << ply.side << "\",\n";
    out << pad2 << "\"role\": \"" << ply.role << "\",\n";
    out << pad2 << "\"raw_response\": \"" << jsonEscape(ply.rawResponse) << "\",\n";
    out << pad2 << "\"parsed_uci\": " << nullableMove(ply.parsedMove) << ",\n";
    out << pad2 << "\"legal\": " << (ply.legal ? "true" : "false") << ",\n";
    out << pad2 << "\"referee_votes\": [\n";
    out << pad2 << "  {\n";
    out << pad2 << "    \"role\": \"" << ply.refereeRole << "\",\n";
    out << pad2 << "    \"stack_id\": \"deterministic_referee\",\n";
    out << pad2 << "    \"move\": " << nullableMove(ply.parsedMove) << ",\n";
    out << pad2 << "    \"legal\": " << (ply.legal ? "true" : "false") << ",\n";
    out << pad2 << "    \"reason\": \"" << ply.refereeReason << "\"\n";
    out << pad2 << "  }\n";
    out << pad2 << "]\n";
    out << pad << "}";
    return out.str();
}

std::vector<std::string> scenarioMoves(const std::string &scenario) {
    if (scenario == "black-win-fools-mate") {
        return {"f2f3", "e7e5", "g2g4", "d8h4"};
    }
    if (scenario == "white-win-fools-mate") {
        return {"e2e4", "f7f6", "d2d4", "g7g5", "d1h5"};
    }
    if (scenario == "forfeit-invalid") {
        return {"e2e4", "move the queen to z9"};
    }
    return {"g1f3", "g8f6", "f3g1", "f6g8"};
}

std::string scenarioResult(const std::string &scenario) {
    if (scenario == "black-win-fools-mate") {
        return "black_win";
    }
    if (scenario == "white-win-fools-mate") {
        return "white_win";
    }
    if (scenario == "forfeit-invalid") {
        return "white_win";
    }
    return "draw";
}

std::string scenarioTermination(const std::string &scenario) {
    if (scenario == "black-win-fools-mate") {
        return "black_checkmate";
    }
    if (scenario == "white-win-fools-mate") {
        return "white_checkmate";
    }
    if (scenario == "forfeit-invalid") {
        return "black_forfeit_invalid_or_unparseable_move";
    }
    return "draw_by_configured_ply_limit";
}

bool moveIsAcceptedForScenario(const std::string &scenario, int ply, const std::string &move) {
    const std::vector<std::string> expected = scenarioMoves(scenario);
    if (ply < 1 || static_cast<std::size_t>(ply) > expected.size()) {
        return false;
    }
    return move == expected[static_cast<std::size_t>(ply - 1)];
}

std::string buildFullGameJson(const std::string &scenario, int maxPlies) {
    const std::vector<std::string> responses = scenarioMoves(scenario);
    std::vector<PlyResult> plies;
    for (std::size_t i = 0; i < responses.size() && static_cast<int>(i) < maxPlies; ++i) {
        const int plyNumber = static_cast<int>(i + 1);
        const std::string side = (plyNumber % 2 == 1) ? "white" : "black";
        const std::string role = (side == "white") ? "board_1_white_agent_1" : "board_1_black_agent_1";
        const std::string parsed = parseUci(responses[i]);
        const bool legal = !parsed.empty() && moveIsAcceptedForScenario(scenario, plyNumber, parsed);
        plies.push_back({
            plyNumber,
            "board_1",
            side,
            role,
            responses[i],
            parsed,
            legal,
            "board_1_referee_1",
            legal ? "move_matches_curated_full_game_fixture_path" : "missing_or_illegal_full_game_move"
        });
        if (!legal) {
            break;
        }
    }

    bool allLegal = true;
    for (const PlyResult &ply : plies) {
        allLegal = allLegal && ply.legal;
    }
    const bool reachedTerminalScript = allLegal && scenario != "draw-max-plies" &&
        plies.size() == responses.size();
    const bool drawByLimit = allLegal && scenario == "draw-max-plies" &&
        static_cast<int>(plies.size()) >= maxPlies;
    const std::string result = scenarioResult(scenario);
    const std::string termination = scenarioTermination(scenario);

    std::ostringstream out;
    out << "{\n";
    out << "  \"test_id\": \"" << kTestId << "\",\n";
    out << "  \"created\": \"" << timestamp("%Y%m%d_%H%M%S") << "\",\n";
    out << "  \"test_class\": \"class_1\",\n";
    out << "  \"test_class_name\": \"rule_bound_state_game_action_hallucination\",\n";
    out << "  \"test_family\": \"AIchess\",\n";
    out << "  \"implementation\": \"cpp17_bash_class1_basic_full_game_fixture\",\n";
    out << "  \"mode\": \"full-game\",\n";
    out << "  \"scenario\": \"" << scenario << "\",\n";
    out << "  \"config_id\": \"" << kConfigId << "\",\n";
    out << "  \"board_count\": 1,\n";
    out << "  \"boards\": [\"board_1\"],\n";
    out << "  \"role_map\": {\n";
    out << "    \"board_1_white_agent_1\": \"fixture_cpp_basic_agent\",\n";
    out << "    \"board_1_black_agent_1\": \"fixture_cpp_basic_agent\",\n";
    out << "    \"board_1_referee_1\": \"deterministic_referee\"\n";
    out << "  },\n";
    out << "  \"start_position\": {\n";
    out << "    \"position_id\": \"standard_start_position\",\n";
    out << "    \"fen\": \"" << kStartFen << "\"\n";
    out << "  },\n";
    out << "  \"termination_policy\": {\n";
    out << "    \"max_plies\": " << maxPlies << ",\n";
    out << "    \"invalid_or_unparseable_move\": \"forfeit\",\n";
    out << "    \"terminal_checkmate_script\": \"win\",\n";
    out << "    \"configured_ply_limit\": \"draw\"\n";
    out << "  },\n";
    out << "  \"ply_results\": [\n";
    for (std::size_t i = 0; i < plies.size(); ++i) {
        out << plyJson(plies[i], 4);
        out << (i + 1 == plies.size() ? "\n" : ",\n");
    }
    out << "  ],\n";
    out << "  \"game_result\": \"" << result << "\",\n";
    out << "  \"termination\": \"" << termination << "\",\n";
    out << "  \"final_ply\": " << plies.size() << ",\n";
    out << "  \"terminal_state_reached\": " << ((reachedTerminalScript || drawByLimit || !allLegal) ? "true" : "false") << ",\n";
    out << "  \"metrics\": {\n";
    out << "    \"legal_ply_count\": " << std::count_if(plies.begin(), plies.end(), [](const PlyResult &ply) { return ply.legal; }) << ",\n";
    out << "    \"illegal_or_unparseable_ply_count\": " << std::count_if(plies.begin(), plies.end(), [](const PlyResult &ply) { return !ply.legal; }) << ",\n";
    out << "    \"referee_vote_count\": " << plies.size() << "\n";
    out << "  },\n";
    out << "  \"score\": " << ((reachedTerminalScript || drawByLimit || !allLegal) ? 1 : 0) << ",\n";
    out << "  \"max_score\": 1,\n";
    out << "  \"errors\": []\n";
    out << "}\n";
    return out.str();
}

std::string buildResultJson(const std::string &rawResponse) {
    const std::string parsedMove = parseUci(rawResponse);
    const bool legal = !parsedMove.empty() && kLegalStartMoves.count(parsedMove) > 0;

    std::ostringstream out;
    out << "{\n";
    out << "  \"test_id\": \"" << kTestId << "\",\n";
    out << "  \"created\": \"" << timestamp("%Y%m%d_%H%M%S") << "\",\n";
    out << "  \"test_class\": \"class_1\",\n";
    out << "  \"test_class_name\": \"rule_bound_state_game_action_hallucination\",\n";
    out << "  \"test_family\": \"AIchess\",\n";
    out << "  \"implementation\": \"cpp17_bash_class1_basic_fixture\",\n";
    out << "  \"config_id\": \"" << kConfigId << "\",\n";
    out << "  \"board_count\": 1,\n";
    out << "  \"boards\": [\"board_1\"],\n";
    out << "  \"role_map\": {\n";
    out << "    \"board_1_white_agent_1\": \"fixture_cpp_basic_agent\",\n";
    out << "    \"board_1_black_agent_1\": \"fixture_cpp_black_placeholder\",\n";
    out << "    \"board_1_referee_1\": \"deterministic_referee\"\n";
    out << "  },\n";
    out << "  \"position\": {\n";
    out << "    \"position_id\": \"standard_start_position\",\n";
    out << "    \"fen\": \"" << kStartFen << "\",\n";
    out << "    \"side_to_move\": \"white\"\n";
    out << "  },\n";
    out << "  \"candidate_moves\": [\n";
    out << "    {\n";
    out << "      \"board_id\": \"board_1\",\n";
    out << "      \"role\": \"board_1_white_agent_1\",\n";
    out << "      \"stack_id\": \"fixture_cpp_basic_agent\",\n";
    out << "      \"raw_response\": \"" << jsonEscape(rawResponse) << "\",\n";
    out << "      \"parsed_uci\": " << nullableMove(parsedMove) << ",\n";
    out << "      \"legal\": " << (legal ? "true" : "false") << ",\n";
    out << "      \"latency_ms\": 0.000\n";
    out << "    }\n";
    out << "  ],\n";
    out << "  \"selected_move_by_board\": {\n";
    out << "    \"board_1\": " << nullableMove(parsedMove) << "\n";
    out << "  },\n";
    out << "  \"referee_votes_by_board\": {\n";
    out << "    \"board_1\": [\n";
    out << "      {\n";
    out << "        \"role\": \"board_1_referee_1\",\n";
    out << "        \"stack_id\": \"deterministic_referee\",\n";
    out << "        \"move\": " << nullableMove(parsedMove) << ",\n";
    out << "        \"legal\": " << (legal ? "true" : "false") << ",\n";
    out << "        \"reason\": \"" << (legal ? "move_in_fixed_start_legal_set" : "missing_or_illegal_fixed_start_move") << "\"\n";
    out << "      }\n";
    out << "    ]\n";
    out << "  },\n";
    out << "  \"metrics\": {\n";
    out << "    \"legal_final_move\": " << (legal ? "true" : "false") << ",\n";
    out << "    \"legal_candidate_move_rate\": " << (legal ? "1.0" : "0.0") << ",\n";
    out << "    \"unparseable_output_count\": " << (parsedMove.empty() ? 1 : 0) << "\n";
    out << "  },\n";
    out << "  \"score\": " << (legal ? 1 : 0) << ",\n";
    out << "  \"max_score\": 1,\n";
    out << "  \"errors\": " << (legal ? "[]" : "[\"illegal_or_unparseable_move\"]") << ",\n";
    out << "  \"notes\": [\n";
    out << "    \"Basic symmetric Class 1 fixture: one board, one player agent per side, one referee.\"\n";
    out << "  ]\n";
    out << "}\n";
    return out.str();
}

} // namespace

int main(int argc, char **argv) {
    std::string rawResponse = "e2e4";
    std::string mode = "one-move";
    std::string scenario = "black-win-fools-mate";
    int maxPlies = 200;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--agent-response" && i + 1 < argc) {
            rawResponse = argv[++i];
        } else if (arg == "--mode" && i + 1 < argc) {
            mode = argv[++i];
        } else if (arg == "--scenario" && i + 1 < argc) {
            scenario = argv[++i];
        } else if (arg == "--max-plies" && i + 1 < argc) {
            maxPlies = std::max(1, std::stoi(argv[++i]));
        } else if (isHelpFlag(arg)) {
            std::cout << "Usage: class1_basic_aichess_fixture [--agent-response TEXT]\n"
                      << "       class1_basic_aichess_fixture --mode full-game "
                         "[--scenario black-win-fools-mate|white-win-fools-mate|draw-max-plies|forfeit-invalid] "
                         "[--max-plies N]\n";
            return 0;
        }
    }

    const std::string json = (mode == "full-game")
        ? buildFullGameJson(scenario, maxPlies)
        : buildResultJson(rawResponse);
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
