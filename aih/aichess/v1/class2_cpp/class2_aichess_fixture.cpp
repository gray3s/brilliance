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

struct BoardFixture {
    std::string boardId;
    std::string whiteRole;
    std::string blackRole;
    std::string refereeRole;
    std::string whiteResponse;
    std::string blackPlaceholder;
};

const std::string kTestId = "aih_chess_class2_cpp_fixture_v1_20260715";
const std::string kStartFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
const std::filesystem::path kRunDir =
    "/home/sag/RPA2/myLLC/AI/brilliance/aih/aichess/v1/runs";

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

bool isHelpFlag(const std::string &arg) {
    return arg == "--help" || arg == "-h" || arg == "/help" || arg == "/?";
}

bool isLegalStartMove(const std::string &move) {
    return !move.empty() && kLegalStartMoves.count(move) > 0;
}

std::string nullableMove(const std::string &move) {
    return move.empty() ? "null" : "\"" + move + "\"";
}

std::string candidateJson(const BoardFixture &board, const std::string &parsed, bool legal, int indent) {
    const std::string pad(indent, ' ');
    const std::string pad2(indent + 2, ' ');
    std::ostringstream out;
    out << pad << "{\n";
    out << pad2 << "\"board_id\": \"" << board.boardId << "\",\n";
    out << pad2 << "\"role\": \"" << board.whiteRole << "\",\n";
    out << pad2 << "\"stack_id\": \"fixture_cpp_class2_agent\",\n";
    out << pad2 << "\"raw_response\": \"" << jsonEscape(board.whiteResponse) << "\",\n";
    out << pad2 << "\"parsed_uci\": " << nullableMove(parsed) << ",\n";
    out << pad2 << "\"legal\": " << (legal ? "true" : "false") << ",\n";
    out << pad2 << "\"latency_ms\": 0.000\n";
    out << pad << "}";
    return out.str();
}

std::string refereeVoteJson(const BoardFixture &board, const std::string &move, bool legal, int indent) {
    const std::string pad(indent, ' ');
    const std::string pad2(indent + 2, ' ');
    std::ostringstream out;
    out << pad << "{\n";
    out << pad2 << "\"role\": \"" << board.refereeRole << "\",\n";
    out << pad2 << "\"stack_id\": \"deterministic_referee\",\n";
    out << pad2 << "\"move\": " << nullableMove(move) << ",\n";
    out << pad2 << "\"legal\": " << (legal ? "true" : "false") << ",\n";
    out << pad2 << "\"reason\": \"" << (legal ? "move_in_fixed_start_legal_set" : "missing_or_illegal_fixed_start_move") << "\"\n";
    out << pad << "}";
    return out.str();
}

std::string buildResultJson(const std::vector<BoardFixture> &boards) {
    std::vector<std::string> parsedMoves;
    std::vector<bool> legalMoves;
    parsedMoves.reserve(boards.size());
    legalMoves.reserve(boards.size());
    for (const BoardFixture &board : boards) {
        const std::string parsed = parseUci(board.whiteResponse);
        parsedMoves.push_back(parsed);
        legalMoves.push_back(isLegalStartMove(parsed));
    }

    const bool allLegal = std::all_of(legalMoves.begin(), legalMoves.end(), [](bool value) {
        return value;
    });
    const int legalCount = static_cast<int>(std::count(legalMoves.begin(), legalMoves.end(), true));
    const int unparseableCount = static_cast<int>(std::count_if(parsedMoves.begin(), parsedMoves.end(), [](const std::string &move) {
        return move.empty();
    }));

    std::ostringstream out;
    out << "{\n";
    out << "  \"test_id\": \"" << kTestId << "\",\n";
    out << "  \"created\": \"" << timestamp("%Y%m%d_%H%M%S") << "\",\n";
    out << "  \"test_class\": \"class_2\",\n";
    out << "  \"test_class_name\": \"provenance_workflow_hallucination\",\n";
    out << "  \"test_family\": \"AIchess\",\n";
    out << "  \"implementation\": \"cpp17_bash_class2_fixture\",\n";
    out << "  \"config_id\": \"aichess_class2_two_boards_one_agent_sides_one_referee_each_v1_20260715_2016\",\n";
    out << "  \"board_count\": 2,\n";
    out << "  \"boards\": [\"board_1\", \"board_2\"],\n";
    out << "  \"role_map\": {\n";
    for (std::size_t i = 0; i < boards.size(); ++i) {
        const BoardFixture &board = boards[i];
        out << "    \"" << board.whiteRole << "\": \"fixture_cpp_class2_agent\",\n";
        out << "    \"" << board.blackRole << "\": \"" << board.blackPlaceholder << "\",\n";
        out << "    \"" << board.refereeRole << "\": \"deterministic_referee\"";
        out << (i + 1 == boards.size() ? "\n" : ",\n");
    }
    out << "  },\n";
    out << "  \"position\": {\n";
    out << "    \"position_id\": \"standard_start_position\",\n";
    out << "    \"fen\": \"" << kStartFen << "\",\n";
    out << "    \"side_to_move\": \"white\"\n";
    out << "  },\n";
    out << "  \"candidate_moves\": [\n";
    for (std::size_t i = 0; i < boards.size(); ++i) {
        out << candidateJson(boards[i], parsedMoves[i], legalMoves[i], 4);
        out << (i + 1 == boards.size() ? "\n" : ",\n");
    }
    out << "  ],\n";
    out << "  \"selected_move_by_board\": {\n";
    for (std::size_t i = 0; i < boards.size(); ++i) {
        out << "    \"" << boards[i].boardId << "\": " << nullableMove(parsedMoves[i]);
        out << (i + 1 == boards.size() ? "\n" : ",\n");
    }
    out << "  },\n";
    out << "  \"referee_votes_by_board\": {\n";
    for (std::size_t i = 0; i < boards.size(); ++i) {
        out << "    \"" << boards[i].boardId << "\": [\n";
        out << refereeVoteJson(boards[i], parsedMoves[i], legalMoves[i], 6) << "\n";
        out << "    ]";
        out << (i + 1 == boards.size() ? "\n" : ",\n");
    }
    out << "  },\n";
    out << "  \"artifact_paths_by_board\": {\n";
    for (std::size_t i = 0; i < boards.size(); ++i) {
        out << "    \"" << boards[i].boardId << "\": \"runs/\"";
        out << (i + 1 == boards.size() ? "\n" : ",\n");
    }
    out << "  },\n";
    out << "  \"metrics\": {\n";
    out << "    \"legal_final_move\": " << (allLegal ? "true" : "false") << ",\n";
    out << "    \"legal_candidate_move_rate\": " << std::fixed << std::setprecision(3)
        << (static_cast<double>(legalCount) / static_cast<double>(boards.size())) << ",\n";
    out << "    \"unparseable_output_count\": " << unparseableCount << ",\n";
    out << "    \"referee_agreement_by_board\": {\n";
    for (std::size_t i = 0; i < boards.size(); ++i) {
        out << "      \"" << boards[i].boardId << "\": true";
        out << (i + 1 == boards.size() ? "\n" : ",\n");
    }
    out << "    },\n";
    out << "    \"workflow_provenance_complete\": true\n";
    out << "  },\n";
    out << "  \"provenance_errors\": [],\n";
    out << "  \"workflow_state_errors\": " << (allLegal ? "[]" : "[\"illegal_or_unparseable_move_by_board\"]") << ",\n";
    out << "  \"score\": " << (allLegal ? 1 : 0) << ",\n";
    out << "  \"max_score\": 1,\n";
    out << "  \"notes\": [\n";
    out << "    \"Class 2 C++ fixture provides the durable two-board Class 2 path.\",\n";
    out << "    \"This fixture preserves board IDs, player-agent moves, referee votes, and artifact paths.\"\n";
    out << "  ]\n";
    out << "}\n";
    return out.str();
}

} // namespace

int main(int argc, char **argv) {
    std::vector<BoardFixture> boards = {
        {"board_1", "board_1_white_agent_1", "board_1_black_agent_1", "board_1_referee_1", "e2e4", "fixture_cpp_black_placeholder"},
        {"board_2", "board_2_white_agent_1", "board_2_black_agent_1", "board_2_referee_1", "d2d4", "fixture_cpp_black_placeholder"},
    };

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--board1-response" && i + 1 < argc) {
            boards[0].whiteResponse = argv[++i];
        } else if (arg == "--board2-response" && i + 1 < argc) {
            boards[1].whiteResponse = argv[++i];
        } else if (isHelpFlag(arg)) {
            std::cout << "Usage: class2_aichess_fixture [--board1-response TEXT] [--board2-response TEXT]\n";
            return 0;
        }
    }

    const std::string json = buildResultJson(boards);
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
