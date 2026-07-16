#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDateTime>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QPair>
#include <QProcess>
#include <QRegularExpression>
#include <QSet>
#include <QStringList>
#include <QTextStream>

static const QString kTestId = "qwen_ollama_chess_timeout_probe_qt_20260715";
static const QString kOutDir =
    "/home/sag/RPA2/myLLC/AI/brilliance/v1/AIH/AIchess/v1/runs/"
    "qwen_ollama_chess_timeout_probe_qt_20260715";
static const QString kStockfishPath = "/usr/games/stockfish";
static const QString kOllamaPath = "/usr/local/bin/ollama";

struct OllamaResult {
    QString status;
    int exitCode = -1;
    double elapsedSeconds = 0.0;
    QString stdoutText;
    QString stderrText;
};

static QStringList splitLines(const QString &text) {
    return text.split(QRegularExpression("[\r\n]+"), Qt::SkipEmptyParts);
}

static QString runTextProcess(const QString &program, const QStringList &args, int timeoutMs, int *exitCode = nullptr) {
    QProcess proc;
    proc.start(program, args);
    if (!proc.waitForStarted(5000)) {
        if (exitCode) {
            *exitCode = -1;
        }
        return QString();
    }
    if (!proc.waitForFinished(timeoutMs)) {
        proc.kill();
        proc.waitForFinished(3000);
        if (exitCode) {
            *exitCode = -2;
        }
        return QString::fromUtf8(proc.readAllStandardOutput());
    }
    if (exitCode) {
        *exitCode = proc.exitCode();
    }
    QString output = QString::fromUtf8(proc.readAllStandardOutput());
    const QString errorOutput = QString::fromUtf8(proc.readAllStandardError());
    if (!errorOutput.isEmpty()) {
        output += "\n" + errorOutput;
    }
    return output;
}

static QStringList detectOllamaModelsSortedBySize() {
    int exitCode = 0;
    const QString output = runTextProcess(kOllamaPath, {"list"}, 10000, &exitCode);
    QList<QPair<double, QString>> sizedModels;
    if (exitCode != 0) {
        return {};
    }
    const QStringList lines = splitLines(output);
    for (int i = 1; i < lines.size(); ++i) {
        const QString simplified = lines.at(i).simplified();
        const QString name = simplified.section(' ', 0, 0);
        const QRegularExpression sizePattern("\\s(\\d+(?:\\.\\d+)?)\\s*(GB|MB)\\s");
        const QRegularExpressionMatch sizeMatch = sizePattern.match(" " + simplified + " ");
        double sizeGb = 0.0;
        if (sizeMatch.hasMatch()) {
            sizeGb = sizeMatch.captured(1).toDouble();
            if (sizeMatch.captured(2) == "MB") {
                sizeGb /= 1024.0;
            }
        }
        sizedModels.append({sizeGb, name});
    }
    std::sort(sizedModels.begin(), sizedModels.end(), [](const auto &a, const auto &b) {
        if (a.first == b.first) {
            return a.second < b.second;
        }
        return a.first < b.first;
    });
    QStringList models;
    for (const auto &entry : sizedModels) {
        models << entry.second;
    }
    return models;
}

static QMap<QString, QString> qwenAliasMap(const QStringList &availableModels) {
    QMap<QString, QString> aliases;
    int qwenIndex = 1;
    for (const QString &model : availableModels) {
        if (model.toLower().contains("qwen")) {
            aliases[QString("qwen%1").arg(qwenIndex++)] = model;
        }
    }
    return aliases;
}

static QMap<QString, QString> agentAliasMap(const QStringList &availableModels) {
    QMap<QString, QString> aliases;
    for (int i = 0; i < availableModels.size(); ++i) {
        aliases[QString("agent%1").arg(i + 1)] = availableModels.at(i);
    }
    return aliases;
}

static QString resolveAgentAlias(const QString &model, const QStringList &availableModels) {
    QMap<QString, QString> aliases = qwenAliasMap(availableModels);
    const QMap<QString, QString> agentAliases = agentAliasMap(availableModels);
    for (auto it = agentAliases.constBegin(); it != agentAliases.constEnd(); ++it) {
        aliases[it.key()] = it.value();
    }
    const QString trimmed = model.trimmed();
    static const QRegularExpression numberedAlias("^(qwen|agent)\\d+$");
    if (numberedAlias.match(trimmed).hasMatch() && !aliases.contains(trimmed)) {
        return "__INVALID_AGENT_ALIAS__:" + trimmed;
    }
    return aliases.value(trimmed, trimmed);
}

static QStringList expandAliasRangeToken(const QString &token) {
    const QString trimmed = token.trimmed();
    static const QRegularExpression rangePattern("^(qwen|agent)(\\d+):(qwen|agent)(\\d+)$");
    const QRegularExpressionMatch match = rangePattern.match(trimmed);
    if (!match.hasMatch() || match.captured(1) != match.captured(3)) {
        return {trimmed};
    }
    const QString prefix = match.captured(1);
    const int start = match.captured(2).toInt();
    const int end = match.captured(4).toInt();
    QStringList expanded;
    const int step = start <= end ? 1 : -1;
    for (int value = start; value != end + step; value += step) {
        expanded << QString("%1%2").arg(prefix).arg(value);
    }
    return expanded;
}

static QStringList splitModelSpec(const QString &spec) {
    QStringList out;
    for (const QString &token : spec.split(',', Qt::SkipEmptyParts)) {
        out << expandAliasRangeToken(token);
    }
    return out;
}

static bool isInvalidAgentAlias(const QString &model) {
    return model.startsWith("__INVALID_AGENT_ALIAS__:");
}

static QString invalidAgentAliasName(const QString &model) {
    return model.section(':', 1);
}

static QString aliasListingText(const QStringList &availableModels) {
    const QMap<QString, QString> agentAliases = agentAliasMap(availableModels);
    const QMap<QString, QString> qwenAliases = qwenAliasMap(availableModels);
    QStringList lines;
    lines << "[all installed Ollama-compatible agents, sorted by size]";
    for (auto it = agentAliases.constBegin(); it != agentAliases.constEnd(); ++it) {
        lines << QString("%1=%2").arg(it.key(), it.value());
    }
    lines << "[qwen subset, sorted by size]";
    for (auto it = qwenAliases.constBegin(); it != qwenAliases.constEnd(); ++it) {
        lines << QString("%1=%2").arg(it.key(), it.value());
    }
    return lines.join('\n');
}

class StockfishReferee {
public:
    bool start() {
        proc_.setProgram(kStockfishPath);
        proc_.start();
        if (!proc_.waitForStarted(5000)) {
            return false;
        }
        writeLine("uci");
        waitFor("uciok", 5000);
        writeLine("isready");
        return waitFor("readyok", 5000);
    }

    QString fenForMoves(const QStringList &moves) {
        setPosition(moves);
        writeLine("d");
        const QString output = readUntil(QRegularExpression("^Checkers:"), 5000);
        const QStringList lines = splitLines(output);
        for (const QString &line : lines) {
            if (line.startsWith("Fen: ")) {
                return line.mid(5).trimmed();
            }
        }
        return moves.isEmpty()
            ? "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
            : QString();
    }

    QStringList legalMoves(const QStringList &moves) {
        setPosition(moves);
        writeLine("go perft 1");
        const QString output = readUntil(QRegularExpression("^Nodes searched:"), 10000);
        QStringList legal;
        const QRegularExpression moveLine("^([a-h][1-8][a-h][1-8][qrbn]?):\\s+\\d+");
        for (const QString &line : splitLines(output)) {
            const QRegularExpressionMatch match = moveLine.match(line.trimmed());
            if (match.hasMatch()) {
                legal << match.captured(1);
            }
        }
        legal.sort();
        return legal;
    }

    bool sideToMoveInCheck(const QStringList &moves) {
        setPosition(moves);
        writeLine("d");
        const QString output = readUntil(QRegularExpression("^Checkers:"), 5000);
        for (const QString &line : splitLines(output)) {
            if (line.startsWith("Checkers:")) {
                return !line.mid(QString("Checkers:").size()).trimmed().isEmpty();
            }
        }
        return false;
    }

    void stop() {
        if (proc_.state() != QProcess::NotRunning) {
            writeLine("quit");
            proc_.waitForFinished(1000);
        }
        if (proc_.state() != QProcess::NotRunning) {
            proc_.kill();
            proc_.waitForFinished(1000);
        }
    }

private:
    QProcess proc_;

    void writeLine(const QString &line) {
        proc_.write((line + "\n").toUtf8());
        proc_.waitForBytesWritten(1000);
    }

    void setPosition(const QStringList &moves) {
        QString command = "position startpos";
        if (!moves.isEmpty()) {
            command += " moves " + moves.join(' ');
        }
        writeLine(command);
    }

    bool waitFor(const QString &needle, int timeoutMs) {
        QElapsedTimer timer;
        timer.start();
        QString buffer;
        while (timer.elapsed() < timeoutMs) {
            proc_.waitForReadyRead(100);
            buffer += QString::fromUtf8(proc_.readAllStandardOutput());
            if (buffer.contains(needle)) {
                return true;
            }
        }
        return false;
    }

    QString readUntil(const QRegularExpression &pattern, int timeoutMs) {
        QElapsedTimer timer;
        timer.start();
        QString buffer;
        while (timer.elapsed() < timeoutMs) {
            proc_.waitForReadyRead(100);
            buffer += QString::fromUtf8(proc_.readAllStandardOutput());
            for (const QString &line : splitLines(buffer)) {
                if (pattern.match(line).hasMatch()) {
                    return buffer;
                }
            }
        }
        return buffer;
    }
};

static QString parseUci(const QString &text) {
    static const QRegularExpression uci("\\b[a-h][1-8][a-h][1-8][qrbn]?\\b",
                                        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch match = uci.match(text);
    return match.hasMatch() ? match.captured(0).toLower() : QString();
}

static QString promptForMove(const QString &fen, const QStringList &legalMoves, int ply) {
    return QString(
        "You are playing a strict chess referee test. Return exactly one legal "
        "move in UCI notation and no other text.\n"
        "Ply: %1\n"
        "FEN: %2\n"
        "Legal UCI moves: %3\n"
        "Answer with one move only.")
        .arg(ply)
        .arg(fen)
        .arg(legalMoves.join(' '));
}

static QString promptForReferee(const QString &fen, const QStringList &legalMoves, const QString &candidateMove, int ply) {
    return QString(
        "You are a strict chess referee. Decide whether the candidate move is "
        "legal for the given position. Return exactly one word: legal or illegal.\n"
        "Ply: %1\n"
        "FEN: %2\n"
        "Legal UCI moves: %3\n"
        "Candidate move: %4\n"
        "Answer with legal or illegal only.")
        .arg(ply)
        .arg(fen)
        .arg(legalMoves.join(' '))
        .arg(candidateMove.isEmpty() ? "(none)" : candidateMove);
}

static bool parseRefereeLegal(const QString &text, bool *parsed) {
    const QString lower = text.trimmed().toLower();
    if (lower.contains("illegal")) {
        *parsed = true;
        return false;
    }
    if (lower.contains("legal")) {
        *parsed = true;
        return true;
    }
    *parsed = false;
    return false;
}

static OllamaResult askOllama(const QString &model, const QString &prompt, int timeoutSeconds) {
    QElapsedTimer timer;
    timer.start();
    QProcess proc;
    proc.start(kOllamaPath, {"run", model, prompt});
    OllamaResult result;
    if (!proc.waitForStarted(5000)) {
        result.status = "start_failed";
        result.elapsedSeconds = timer.elapsed() / 1000.0;
        result.stderrText = proc.errorString();
        return result;
    }
    if (!proc.waitForFinished(timeoutSeconds * 1000)) {
        proc.kill();
        proc.waitForFinished(3000);
        result.status = "timed_out";
        result.elapsedSeconds = timer.elapsed() / 1000.0;
        result.stdoutText = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
        result.stderrText = QString::fromUtf8(proc.readAllStandardError()).trimmed();
        return result;
    }
    result.status = "completed";
    result.exitCode = proc.exitCode();
    result.elapsedSeconds = timer.elapsed() / 1000.0;
    result.stdoutText = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
    result.stderrText = QString::fromUtf8(proc.readAllStandardError()).trimmed();
    return result;
}

static QJsonObject ollamaJson(const OllamaResult &result) {
    QJsonObject obj;
    obj["status"] = result.status;
    obj["exit_code"] = result.exitCode;
    obj["elapsed_s"] = result.elapsedSeconds;
    obj["stdout"] = result.stdoutText;
    obj["stderr"] = result.stderrText;
    return obj;
}

static QJsonObject runOneMove(const QString &model, int moveTimeoutSeconds) {
    StockfishReferee referee;
    QJsonObject obj;
    obj["test_id"] = kTestId;
    obj["mode"] = "one_move";
    obj["model"] = model;
    obj["move_timeout_s"] = moveTimeoutSeconds;
    if (!referee.start()) {
        obj["termination"] = "stockfish_start_failed";
        return obj;
    }
    const QStringList moves;
    const QString fen = referee.fenForMoves(moves);
    const QStringList legal = referee.legalMoves(moves);
    const OllamaResult response = askOllama(model, promptForMove(fen, legal, 1), moveTimeoutSeconds);
    const QString parsed = parseUci(response.stdoutText);
    const bool legalMove = legal.contains(parsed);
    obj["fen_before"] = fen;
    obj["response"] = ollamaJson(response);
    obj["parsed_uci"] = parsed;
    obj["legal"] = legalMove;
    obj["termination"] = response.status == "completed" ? "completed" : response.status;
    referee.stop();
    return obj;
}

static QJsonObject runGame(const QString &model,
                           int moveTimeoutSeconds,
                           int gameTimeoutSeconds,
                           int maxPlies,
                           int maxIllegal) {
    StockfishReferee referee;
    QJsonObject obj;
    obj["test_id"] = kTestId;
    obj["mode"] = "game";
    obj["model"] = model;
    obj["move_timeout_s"] = moveTimeoutSeconds;
    obj["game_timeout_s"] = gameTimeoutSeconds;
    obj["max_plies"] = maxPlies;
    obj["max_illegal"] = maxIllegal;
    if (!referee.start()) {
        obj["termination"] = "stockfish_start_failed";
        return obj;
    }

    QStringList moves;
    QJsonArray events;
    int illegalCount = 0;
    QString termination = "unknown";
    QElapsedTimer gameTimer;
    gameTimer.start();

    for (int ply = 1; ply <= maxPlies; ++ply) {
        const double elapsed = gameTimer.elapsed() / 1000.0;
        if (elapsed >= gameTimeoutSeconds) {
            termination = "game_timeout";
            break;
        }
        const QString fen = referee.fenForMoves(moves);
        const QStringList legal = referee.legalMoves(moves);
        if (legal.isEmpty()) {
            termination = "game_completed";
            break;
        }

        const int remainingSeconds = qMax(1, gameTimeoutSeconds - int(elapsed));
        const int effectiveMoveTimeout = qMin(moveTimeoutSeconds, remainingSeconds);
        const OllamaResult response = askOllama(model, promptForMove(fen, legal, ply), effectiveMoveTimeout);
        const QString parsed = parseUci(response.stdoutText);
        const bool legalMove = legal.contains(parsed);

        QJsonObject event;
        event["ply"] = ply;
        event["fen_before"] = fen;
        event["response"] = ollamaJson(response);
        event["parsed_uci"] = parsed;
        event["legal"] = legalMove;

        if (response.status == "timed_out") {
            event["error"] = "move_timeout";
            events.append(event);
            termination = "move_timeout";
            break;
        }

        if (legalMove) {
            moves << parsed;
            event["fen_after"] = referee.fenForMoves(moves);
        } else {
            illegalCount += 1;
            event["error"] = parsed.isEmpty() ? "unparseable_move" : "illegal_move";
        }
        events.append(event);

        if (illegalCount >= maxIllegal) {
            termination = "illegal_move_limit";
            break;
        }
    }

    if (termination == "unknown") {
        termination = "ply_limit";
    }
    if (referee.legalMoves(moves).isEmpty()) {
        termination = "game_completed";
    }

    obj["termination"] = termination;
    obj["completed_game"] = termination == "game_completed";
    obj["plies_played"] = moves.size();
    obj["events_recorded"] = events.size();
    obj["legal_moves_played"] = moves.size();
    obj["illegal_or_unparseable_count"] = illegalCount;
    obj["total_elapsed_s"] = gameTimer.elapsed() / 1000.0;
    obj["final_fen"] = referee.fenForMoves(moves);
    obj["move_history"] = moves.join(' ');
    obj["events"] = events;
    referee.stop();
    return obj;
}

static QString selectModelForBoard(const QStringList &models, int boardIndex) {
    if (models.isEmpty()) {
        return QString();
    }
    return models.at(boardIndex % models.size()).trimmed();
}

static QJsonObject runBoardGame(int boardIndex,
                                const QString &whiteModel,
                                const QString &blackModel,
                                const QString &requestedWhiteModel,
                                const QString &requestedBlackModel,
                                const QStringList &refereeModels,
                                const QStringList &requestedRefereeModels,
                                int refereeCount,
                                int moveTimeoutSeconds,
                                int gameTimeoutSeconds,
                                int maxPlies,
                                int maxIllegal) {
    StockfishReferee referee;
    QJsonObject obj;
    const QString boardId = QString("board_%1").arg(boardIndex + 1);
    obj["test_id"] = kTestId;
    obj["mode"] = "aichess_hallucination_game";
    obj["board_id"] = boardId;
    obj["white_model"] = whiteModel;
    obj["black_model"] = blackModel;
    obj["requested_white_model"] = requestedWhiteModel;
    obj["requested_black_model"] = requestedBlackModel;
    obj["model"] = whiteModel + " vs " + blackModel;
    obj["role_assignment"] = QString("white=%1 resolved=%2; black=%3 resolved=%4")
        .arg(requestedWhiteModel, whiteModel, requestedBlackModel, blackModel);
    obj["ground_truth_rules_engine"] = "stockfish";
    obj["referee_count"] = refereeCount;
    QJsonArray refereeModelArray;
    QJsonArray requestedRefereeModelArray;
    for (const QString &model : refereeModels) {
        refereeModelArray.append(model);
    }
    for (const QString &model : requestedRefereeModels) {
        requestedRefereeModelArray.append(model);
    }
    obj["referee_models"] = refereeModelArray;
    obj["requested_referee_models"] = requestedRefereeModelArray;
    const bool stockfishOnlyRefereeForRun = refereeModels.size() == 1 && refereeModels.first() == "stockfish";
    obj["referee_mode"] = stockfishOnlyRefereeForRun
        ? "stockfish_baseline_two_player_agents"
        : "agentic_referee_vote";
    QJsonObject agentConfiguration;
    agentConfiguration["board_id"] = boardId;
    agentConfiguration["white_role"] = QString("%1_white_agent_1").arg(boardId);
    agentConfiguration["white_requested"] = requestedWhiteModel;
    agentConfiguration["white_model"] = whiteModel;
    agentConfiguration["black_role"] = QString("%1_black_agent_1").arg(boardId);
    agentConfiguration["black_requested"] = requestedBlackModel;
    agentConfiguration["black_model"] = blackModel;
    agentConfiguration["referee_mode"] = obj["referee_mode"];
    QJsonArray refereeAssignments;
    const int assignmentRefereeCount = stockfishOnlyRefereeForRun ? 1 : refereeCount;
    for (int ref = 1; ref <= assignmentRefereeCount; ++ref) {
        QJsonObject assignment;
        assignment["role"] = QString("%1_referee_%2").arg(boardId).arg(ref);
        assignment["requested"] = selectModelForBoard(requestedRefereeModels, ref - 1);
        assignment["model"] = selectModelForBoard(refereeModels, ref - 1);
        assignment["agentic"] = !stockfishOnlyRefereeForRun;
        refereeAssignments.append(assignment);
    }
    agentConfiguration["referees"] = refereeAssignments;
    obj["agent_configuration"] = agentConfiguration;
    obj["move_timeout_s"] = moveTimeoutSeconds;
    obj["game_timeout_s"] = gameTimeoutSeconds;
    obj["max_plies"] = maxPlies;
    obj["max_illegal"] = maxIllegal;
    obj["hallucination_test"] = true;
    if (!referee.start()) {
        obj["termination"] = "stockfish_start_failed";
        obj["game_result"] = "no_result";
        obj["terminal_state_reached"] = false;
        return obj;
    }

    QStringList moves;
    QJsonArray events;
    int illegalCount = 0;
    QString termination = "unknown";
    QString gameResult = "draw";
    QElapsedTimer gameTimer;
    gameTimer.start();

    for (int ply = 1; ply <= maxPlies; ++ply) {
        const double elapsed = gameTimer.elapsed() / 1000.0;
        if (elapsed >= gameTimeoutSeconds) {
            termination = "game_timeout";
            gameResult = "draw";
            break;
        }

        const QString fen = referee.fenForMoves(moves);
        const QStringList legal = referee.legalMoves(moves);
        const bool whiteToMove = (ply % 2) == 1;
        const QString side = whiteToMove ? "white" : "black";
        const QString activeModel = whiteToMove ? whiteModel : blackModel;
        const QString role = QString("%1_%2_agent_1").arg(boardId, side);

        if (legal.isEmpty()) {
            const bool inCheck = referee.sideToMoveInCheck(moves);
            if (inCheck) {
                gameResult = whiteToMove ? "black_win" : "white_win";
                termination = whiteToMove ? "black_checkmate" : "white_checkmate";
            } else {
                gameResult = "draw";
                termination = "stalemate";
            }
            break;
        }

        const int remainingSeconds = qMax(1, gameTimeoutSeconds - int(elapsed));
        const int effectiveMoveTimeout = qMin(moveTimeoutSeconds, remainingSeconds);
        const OllamaResult response = askOllama(activeModel, promptForMove(fen, legal, ply), effectiveMoveTimeout);
        const QString parsed = parseUci(response.stdoutText);
        const bool legalMove = legal.contains(parsed);

        QJsonArray refereeVotes;
        int refereeValidVotes = 0;
        int refereeInvalidVotes = 0;
        int refereeUnparseableVotes = 0;
        const bool stockfishOnlyReferee = refereeModels.size() == 1 && refereeModels.first() == "stockfish";
        const int effectiveRefereeCount = stockfishOnlyReferee ? 1 : refereeCount;
        for (int ref = 1; ref <= effectiveRefereeCount; ++ref) {
            QJsonObject vote;
            vote["role"] = QString("%1_referee_%2").arg(boardId).arg(ref);
            const QString requestedReferee = selectModelForBoard(requestedRefereeModels, ref - 1);
            const QString resolvedReferee = selectModelForBoard(refereeModels, ref - 1);
            vote["requested_model"] = requestedReferee;
            vote["model"] = resolvedReferee;
            vote["stockfish_ground_truth_legal"] = legalMove;
            vote["move"] = parsed;
            if (stockfishOnlyReferee) {
                vote["stack_id"] = "stockfish_ground_truth_rules_engine";
                vote["referee_backend"] = "stockfish_baseline";
                vote["legal"] = legalMove;
                vote["parsed_referee_vote"] = true;
                vote["agrees_with_stockfish"] = true;
                vote["reason"] = legalMove ? "move_in_stockfish_legal_set" : "missing_or_illegal_stockfish_move";
                if (legalMove) {
                    refereeValidVotes += 1;
                } else {
                    refereeInvalidVotes += 1;
                }
            } else {
                const OllamaResult refereeResponse = askOllama(
                    resolvedReferee,
                    promptForReferee(fen, legal, parsed, ply),
                    effectiveMoveTimeout);
                bool parsedRefereeVote = false;
                const bool agentSaysLegal = parseRefereeLegal(refereeResponse.stdoutText, &parsedRefereeVote);
                vote["stack_id"] = resolvedReferee;
                vote["referee_backend"] = "ollama_agent";
                vote["response"] = ollamaJson(refereeResponse);
                vote["legal"] = agentSaysLegal;
                vote["parsed_referee_vote"] = parsedRefereeVote;
                vote["agrees_with_stockfish"] = parsedRefereeVote && agentSaysLegal == legalMove;
                if (!parsedRefereeVote) {
                    refereeUnparseableVotes += 1;
                    refereeInvalidVotes += 1;
                } else if (agentSaysLegal) {
                    refereeValidVotes += 1;
                } else {
                    refereeInvalidVotes += 1;
                }
                vote["reason"] = parsedRefereeVote
                    ? "agentic_referee_vote_compared_to_stockfish_ground_truth"
                    : "unparseable_agentic_referee_vote";
            }
            refereeVotes.append(vote);
        }

        QJsonObject event;
        event["ply"] = ply;
        event["board_id"] = boardId;
        event["side_to_move"] = side;
        event["role"] = role;
        event["model"] = activeModel;
        event["fen_before"] = fen;
        event["legal_move_count"] = legal.size();
        event["response"] = ollamaJson(response);
        event["parsed_uci"] = parsed;
        const bool refereeMajorityValid = refereeValidVotes > refereeInvalidVotes;
        event["legal_by_stockfish"] = legalMove;
        event["legal_by_referee_vote"] = refereeMajorityValid;
        event["legal"] = refereeMajorityValid;
        event["referee_valid_votes"] = refereeValidVotes;
        event["referee_invalid_votes"] = refereeInvalidVotes;
        event["referee_unparseable_votes"] = refereeUnparseableVotes;
        event["referee_vote_rule"] = "majority";
        event["referee_votes"] = refereeVotes;

        if (response.status == "timed_out") {
            event["error"] = "move_timeout_forfeit";
            events.append(event);
            termination = side + "_forfeit_move_timeout";
            gameResult = whiteToMove ? "black_win" : "white_win";
            break;
        }

        if (refereeMajorityValid && legalMove) {
            moves << parsed;
            event["fen_after"] = referee.fenForMoves(moves);
        } else {
            illegalCount += 1;
            if (refereeMajorityValid && !legalMove) {
                event["error"] = "referee_majority_accepted_illegal_move_hallucination";
            } else if (!refereeMajorityValid && legalMove) {
                event["error"] = "referee_majority_rejected_legal_move_hallucination";
            } else {
                event["error"] = parsed.isEmpty() ? "unparseable_move_hallucination" : "illegal_move_hallucination";
            }
        }
        events.append(event);

        if (illegalCount >= maxIllegal) {
            termination = side + "_forfeit_invalid_or_unparseable_move";
            gameResult = whiteToMove ? "black_win" : "white_win";
            break;
        }
    }

    if (termination == "unknown") {
        termination = "draw_by_configured_ply_limit";
        gameResult = "draw";
    }

    obj["termination"] = termination;
    obj["game_result"] = gameResult;
    obj["terminal_state_reached"] = true;
    obj["completed_game"] = termination.endsWith("checkmate") || termination == "stalemate";
    obj["plies_played"] = moves.size();
    obj["events_recorded"] = events.size();
    obj["legal_moves_played"] = moves.size();
    obj["illegal_or_unparseable_count"] = illegalCount;
    obj["total_elapsed_s"] = gameTimer.elapsed() / 1000.0;
    obj["final_fen"] = referee.fenForMoves(moves);
    obj["move_history"] = moves.join(' ');
    obj["events"] = events;
    referee.stop();
    return obj;
}

static void writeOutputs(const QList<QJsonObject> &results) {
    QDir().mkpath(kOutDir);
    const QString stamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    const QString jsonlPath = kOutDir + "/" + kTestId + "_" + stamp + ".jsonl";
    const QString summaryPath = kOutDir + "/" + kTestId + "_" + stamp + "_summary.md";

    QFile jsonl(jsonlPath);
    jsonl.open(QIODevice::WriteOnly | QIODevice::Text);
    for (const QJsonObject &result : results) {
        jsonl.write(QJsonDocument(result).toJson(QJsonDocument::Compact));
        jsonl.write("\n");
    }
    jsonl.close();

    QFile summary(summaryPath);
    summary.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream out(&summary);
    out << "# " << kTestId << " Summary\n\n";
    out << "Created: " << QDateTime::currentDateTime().toString(Qt::ISODate) << "\n\n";
    out << "| Model | Mode | Termination | Completed game | Plies | Legal moves | Illegal/unparseable | Elapsed s |\n";
    out << "| --- | --- | --- | --- | ---: | ---: | ---: | ---: |\n";
    for (const QJsonObject &r : results) {
        const QJsonObject response = r.value("response").toObject();
        const double elapsed = r.contains("total_elapsed_s")
            ? r.value("total_elapsed_s").toDouble()
            : response.value("elapsed_s").toDouble();
        const int plies = r.contains("plies_played")
            ? r.value("plies_played").toInt()
            : (r.value("legal").toBool() ? 1 : 0);
        const int legal = r.contains("legal_moves_played")
            ? r.value("legal_moves_played").toInt()
            : (r.value("legal").toBool() ? 1 : 0);
        const int illegal = r.contains("illegal_or_unparseable_count")
            ? r.value("illegal_or_unparseable_count").toInt()
            : (r.value("legal").toBool() ? 0 : 1);
        out << "| " << r.value("model").toString()
            << " | " << r.value("mode").toString()
            << " | " << r.value("termination").toString()
            << " | " << (r.value("completed_game").toBool() ? "true" : "false")
            << " | " << plies
            << " | " << legal
            << " | " << illegal
            << " | " << QString::number(elapsed, 'f', 3)
            << " |\n";
    }
    summary.close();

    QTextStream(stdout) << summaryPath << "\n" << jsonlPath << "\n";
}

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    QCommandLineParser parser;
    parser.setApplicationDescription("Qwen Ollama chess timeout probe");
    parser.addHelpOption();

    QCommandLineOption modeOpt("mode", "aichess, one-move, game, or both", "mode", "aichess");
    QCommandLineOption modelOpt("models", "Comma-separated Ollama model names", "models");
    QCommandLineOption whiteModelsOpt("white-models", "Comma-separated White player models, assigned round-robin by board", "models");
    QCommandLineOption blackModelsOpt("black-models", "Comma-separated Black player models, assigned round-robin by board", "models");
    QCommandLineOption refereeModelsOpt("referee-models", "Comma-separated referee model labels, recorded round-robin by referee role", "models");
    QCommandLineOption whiteOpt("white", "Alias for --white-models", "models");
    QCommandLineOption blackOpt("black", "Alias for --black-models", "models");
    QCommandLineOption refereeOpt("referee", "Alias for --referee-models", "models");
    QCommandLineOption boardsOpt("boards", "Number of boards to run in AIChess hallucination game mode", "count", "1");
    QCommandLineOption refereeCountOpt("referee-count", "Referee votes recorded per board", "count", "1");
    QCommandLineOption moveTimeoutOpt("move-timeout", "Per-move timeout seconds", "seconds", "60");
    QCommandLineOption gameTimeoutOpt("game-timeout", "Per-game timeout seconds", "seconds", "600");
    QCommandLineOption maxPliesOpt("max-plies", "Maximum plies per game", "plies", "120");
    QCommandLineOption maxIllegalOpt("max-illegal", "Illegal/unparseable move limit before forfeit", "count", "1");
    QCommandLineOption listModelsOpt("list-models", "List detected Qwen models and exit");
    QCommandLineOption dryRunOpt("dry-run", "Print planned run without invoking Ollama");

    parser.addOption(modeOpt);
    parser.addOption(modelOpt);
    parser.addOption(whiteModelsOpt);
    parser.addOption(blackModelsOpt);
    parser.addOption(refereeModelsOpt);
    parser.addOption(whiteOpt);
    parser.addOption(blackOpt);
    parser.addOption(refereeOpt);
    parser.addOption(boardsOpt);
    parser.addOption(refereeCountOpt);
    parser.addOption(moveTimeoutOpt);
    parser.addOption(gameTimeoutOpt);
    parser.addOption(maxPliesOpt);
    parser.addOption(maxIllegalOpt);
    parser.addOption(listModelsOpt);
    parser.addOption(dryRunOpt);
    parser.process(app);

    QStringList models;
    if (parser.isSet(modelOpt)) {
        models = parser.value(modelOpt).split(',', Qt::SkipEmptyParts);
    } else {
        models = detectOllamaModelsSortedBySize();
    }
    for (QString &model : models) {
        model = resolveAgentAlias(model, models);
    }

    QStringList whiteModels = parser.isSet(whiteOpt)
        ? splitModelSpec(parser.value(whiteOpt))
        : parser.isSet(whiteModelsOpt)
        ? splitModelSpec(parser.value(whiteModelsOpt))
        : models;
    QStringList blackModels = parser.isSet(blackOpt)
        ? splitModelSpec(parser.value(blackOpt))
        : parser.isSet(blackModelsOpt)
        ? splitModelSpec(parser.value(blackModelsOpt))
        : models;
    QStringList requestedWhiteModels = whiteModels;
    QStringList requestedBlackModels = blackModels;
    QStringList refereeModels = parser.isSet(refereeOpt)
        ? splitModelSpec(parser.value(refereeOpt))
        : parser.isSet(refereeModelsOpt)
        ? splitModelSpec(parser.value(refereeModelsOpt))
        : QStringList{"agent1"};
    QStringList requestedRefereeModels = refereeModels;
    for (QString &model : whiteModels) {
        model = resolveAgentAlias(model, models);
    }
    for (QString &model : blackModels) {
        model = resolveAgentAlias(model, models);
    }
    for (QString &model : refereeModels) {
        if (model.trimmed() == "stockfish") {
            model = "stockfish";
        } else {
            model = resolveAgentAlias(model, models);
        }
    }

    if (parser.isSet(listModelsOpt)) {
        QTextStream(stdout) << aliasListingText(models) << "\n";
        return 0;
    }
    for (const QString &model : whiteModels + blackModels + refereeModels) {
        if (isInvalidAgentAlias(model)) {
            const QString invalidName = invalidAgentAliasName(model);
            QTextStream(stderr)
                << "Invalid agent alias: " << invalidName << "\n"
                << "Available aliases:\n" << aliasListingText(models) << "\n";
            return 1;
        }
    }
    const QString requestedMode = parser.value(modeOpt);
    const bool aichessMode = requestedMode == "aichess" ||
        requestedMode == "hallucination-game" ||
        requestedMode == "class-game";

    if (models.isEmpty() && !aichessMode) {
        QTextStream(stderr) << "No local Ollama models found. Run ollama list or pass --models.\n";
        return 1;
    }

    const QString mode = requestedMode;
    const int moveTimeout = parser.value(moveTimeoutOpt).toInt();
    const int gameTimeout = parser.value(gameTimeoutOpt).toInt();
    const int maxPlies = parser.value(maxPliesOpt).toInt();
    const int maxIllegal = parser.value(maxIllegalOpt).toInt();
    const int boards = qMax(1, parser.value(boardsOpt).toInt());
    const int refereeCount = qMax(1, parser.value(refereeCountOpt).toInt());

    if (parser.isSet(dryRunOpt)) {
        QJsonObject obj;
        obj["mode"] = mode;
        obj["boards"] = boards;
        obj["referee_count"] = refereeCount;
        obj["move_timeout_s"] = moveTimeout;
        obj["game_timeout_s"] = gameTimeout;
        obj["max_plies"] = maxPlies;
        obj["max_illegal"] = maxIllegal;
        QJsonArray modelArray;
        for (const QString &model : models) {
            modelArray.append(model);
        }
        obj["models"] = modelArray;
        QJsonArray whiteArray;
        for (const QString &model : whiteModels) {
            whiteArray.append(model);
        }
        obj["white_models"] = whiteArray;
        QJsonArray blackArray;
        for (const QString &model : blackModels) {
            blackArray.append(model);
        }
        obj["black_models"] = blackArray;
        QJsonArray refereeArray;
        for (const QString &model : refereeModels) {
            refereeArray.append(model);
        }
        obj["referee_models"] = refereeArray;
        QJsonArray boardAssignments;
        for (int board = 0; board < boards; ++board) {
            QJsonObject assignment;
            const QString boardId = QString("board_%1").arg(board + 1);
            assignment["board_id"] = boardId;
            assignment["white_role"] = QString("%1_white_agent_1").arg(boardId);
            assignment["white_requested"] = selectModelForBoard(requestedWhiteModels, board);
            assignment["white_model"] = selectModelForBoard(whiteModels, board);
            assignment["black_role"] = QString("%1_black_agent_1").arg(boardId);
            assignment["black_requested"] = selectModelForBoard(requestedBlackModels, board);
            assignment["black_model"] = selectModelForBoard(blackModels, board);
            const bool stockfishOnly = refereeModels.size() == 1 && refereeModels.first() == "stockfish";
            assignment["referee_mode"] = stockfishOnly
                ? "stockfish_baseline_two_player_agents"
                : "agentic_referee_vote";
            QJsonArray refereeAssignments;
            const int assignmentRefereeCount = stockfishOnly ? 1 : refereeCount;
            for (int ref = 1; ref <= assignmentRefereeCount; ++ref) {
                QJsonObject refereeAssignment;
                refereeAssignment["role"] = QString("%1_referee_%2").arg(boardId).arg(ref);
                refereeAssignment["requested"] = selectModelForBoard(requestedRefereeModels, ref - 1);
                refereeAssignment["model"] = selectModelForBoard(refereeModels, ref - 1);
                refereeAssignment["agentic"] = !stockfishOnly;
                refereeAssignments.append(refereeAssignment);
            }
            assignment["referees"] = refereeAssignments;
            boardAssignments.append(assignment);
        }
        obj["agent_configuration_by_board"] = boardAssignments;
        QTextStream(stdout) << QJsonDocument(obj).toJson(QJsonDocument::Indented);
        return 0;
    }

    QList<QJsonObject> results;
    if (mode == "aichess" || mode == "hallucination-game" || mode == "class-game") {
        if (whiteModels.isEmpty() || blackModels.isEmpty()) {
            QTextStream(stderr) << "AIChess game mode requires --white-models/--black-models or detectable --models.\n";
            return 1;
        }
        for (int board = 0; board < boards; ++board) {
            results.append(runBoardGame(board,
                                        selectModelForBoard(whiteModels, board),
                                        selectModelForBoard(blackModels, board),
                                        selectModelForBoard(requestedWhiteModels, board),
                                        selectModelForBoard(requestedBlackModels, board),
                                        refereeModels,
                                        requestedRefereeModels,
                                        refereeCount,
                                        moveTimeout,
                                        gameTimeout,
                                        maxPlies,
                                        maxIllegal));
        }
    } else {
        for (const QString &model : models) {
            if (mode == "one-move" || mode == "both") {
            results.append(runOneMove(model, moveTimeout));
            }
            if (mode == "game" || mode == "both") {
            results.append(runGame(model, moveTimeout, gameTimeout, maxPlies, maxIllegal));
            }
        }
    }
    writeOutputs(results);
    return 0;
}
