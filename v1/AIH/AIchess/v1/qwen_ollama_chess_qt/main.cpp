#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDateTime>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
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

static QStringList detectQwenModels() {
    int exitCode = 0;
    const QString output = runTextProcess(kOllamaPath, {"list"}, 10000, &exitCode);
    QStringList models;
    if (exitCode != 0) {
        return models;
    }
    const QStringList lines = splitLines(output);
    for (int i = 1; i < lines.size(); ++i) {
        const QString name = lines.at(i).simplified().section(' ', 0, 0);
        if (name.toLower().contains("qwen")) {
            models << name;
        }
    }
    return models;
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

    QCommandLineOption modeOpt("mode", "one-move, game, or both", "mode", "both");
    QCommandLineOption modelOpt("models", "Comma-separated Ollama model names", "models");
    QCommandLineOption moveTimeoutOpt("move-timeout", "Per-move timeout seconds", "seconds", "60");
    QCommandLineOption gameTimeoutOpt("game-timeout", "Per-game timeout seconds", "seconds", "600");
    QCommandLineOption maxPliesOpt("max-plies", "Maximum plies per game", "plies", "120");
    QCommandLineOption maxIllegalOpt("max-illegal", "Illegal/unparseable move limit", "count", "3");
    QCommandLineOption listModelsOpt("list-models", "List detected Qwen models and exit");
    QCommandLineOption dryRunOpt("dry-run", "Print planned run without invoking Ollama");

    parser.addOption(modeOpt);
    parser.addOption(modelOpt);
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
        models = detectQwenModels();
    }
    for (QString &model : models) {
        model = model.trimmed();
    }

    if (parser.isSet(listModelsOpt)) {
        QTextStream(stdout) << models.join('\n') << "\n";
        return 0;
    }
    if (models.isEmpty()) {
        QTextStream(stderr) << "No Qwen models found. Run ollama list or pass --models.\n";
        return 1;
    }

    const QString mode = parser.value(modeOpt);
    const int moveTimeout = parser.value(moveTimeoutOpt).toInt();
    const int gameTimeout = parser.value(gameTimeoutOpt).toInt();
    const int maxPlies = parser.value(maxPliesOpt).toInt();
    const int maxIllegal = parser.value(maxIllegalOpt).toInt();

    if (parser.isSet(dryRunOpt)) {
        QJsonObject obj;
        obj["mode"] = mode;
        obj["move_timeout_s"] = moveTimeout;
        obj["game_timeout_s"] = gameTimeout;
        obj["max_plies"] = maxPlies;
        obj["max_illegal"] = maxIllegal;
        QJsonArray modelArray;
        for (const QString &model : models) {
            modelArray.append(model);
        }
        obj["models"] = modelArray;
        QTextStream(stdout) << QJsonDocument(obj).toJson(QJsonDocument::Indented);
        return 0;
    }

    QList<QJsonObject> results;
    for (const QString &model : models) {
        if (mode == "one-move" || mode == "both") {
            results.append(runOneMove(model, moveTimeout));
        }
        if (mode == "game" || mode == "both") {
            results.append(runGame(model, moveTimeout, gameTimeout, maxPlies, maxIllegal));
        }
    }
    writeOutputs(results);
    return 0;
}
