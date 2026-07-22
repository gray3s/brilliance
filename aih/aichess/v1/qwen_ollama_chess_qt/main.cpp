#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QEventLoop>
#include <QMutex>
#include <QMutexLocker>
#include <QNetworkAccessManager>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPair>
#include <QProcess>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QSet>
#include <QStringList>
#include <QTextStream>
#include <QTimer>
#include <QUrl>
#include <future>
#include <vector>

static const QString kTestId = "qwen_ollama_chess_timeout_probe_qt_20260715";
static const QString kOutDir =
    "/home/sag/RPA2/myLLC/AI/brilliance/aih/aichess/v1/runs/"
    "qwen_ollama_chess_timeout_probe_qt_20260715";
static const QString kStockfishPath = "/usr/games/stockfish";
static const QString kOllamaPath = "/usr/local/bin/ollama";
static const QString kOpenAiEndpoint = "https://api.openai.com/v1/responses";
static QMutex kStderrMutex;
static int kLogLevel = 3;

struct OllamaResult {
    QString status;
    int exitCode = -1;
    double elapsedSeconds = 0.0;
    QString stdoutText;
    QString stderrText;
    bool backendDone = false;
    QString doneReason;
};

struct DetPiece {
    QString pid;
    QString side;
    QString ptype;
};

struct DetBoardState {
    QMap<QString, DetPiece> sq;
    QString side = "W";
};

static QStringList splitLines(const QString &text) {
    return text.split(QRegularExpression("[\r\n]+"), Qt::SkipEmptyParts);
}

static void writeStderr(const QString &text) {
    QMutexLocker locker(&kStderrMutex);
    QTextStream(stderr) << text;
}

static bool logEnabled(int level) {
    return kLogLevel >= level;
}

static void writeLog(int level, const QString &text) {
    if (logEnabled(level)) {
        writeStderr(text);
    }
}

static QString rawResponseLine(const QString &text) {
    QString raw = text;
    raw.replace("\\", "\\\\");
    raw.replace("\r", "\\r");
    raw.replace("\n", "\\n");
    return raw;
}

static QString traceString(const QString &text) {
    const QString raw = rawResponseLine(text);
    constexpr int maxTraceChars = 220;
    if (raw.size() <= maxTraceChars) {
        return raw;
    }
    return "<json_full_string>";
}

static QString traceSha256(const QString &text) {
    return QString::fromLatin1(QCryptographicHash::hash(text.toUtf8(), QCryptographicHash::Sha256).toHex());
}

static QString traceSha(const QString &text) {
    return traceSha256(text).left(12);
}

static QString logoff(int depth) {
    return QString(depth, '\t');
}

static bool isModelHarnessEndpoint(const QString &endpoint) {
    return endpoint.startsWith("ollama:") || endpoint.startsWith("openai:");
}

static QString level2ModelIoText(const QString &text) {
    return text;
}

static void logLevel2String(const QString &token, const QString &text) {
    const QString value = text.trimmed();
    if (value.contains('\n') || value.contains('\r')) {
        writeLog(1, QString("%1: \"\n%2\n\"\n").arg(token, value));
        return;
    }
    writeLog(1, QString("%1: \"%2\"\n").arg(token, rawResponseLine(value)));
}

static void logHarnessInput(const QString &label, const QString &inputSource, const QString &text) {
    if (kLogLevel == 2 && isModelHarnessEndpoint(inputSource)) {
        Q_UNUSED(label);
        const QString visibleText = level2ModelIoText(text);
        logLevel2String("hi", visibleText);
        return;
    }
    const int level = isModelHarnessEndpoint(inputSource)
        ? 1
        : 4;
    writeLog(level, QString("%1%2 harness hi=\"%3\" hiln=%4 hiby=%5 hisha=%6 hitr=%7 hisrc=\"%8\"\n")
        .arg(logoff(1))
        .arg(label)
        .arg(traceString(text))
        .arg(text.size())
        .arg(text.toUtf8().size())
        .arg(traceSha(text))
        .arg(rawResponseLine(text).size() > traceString(text).size() ? "true" : "false")
        .arg(inputSource));
}

static void logHarnessOutput(const QString &label, const QString &outputTarget, const QString &text) {
    if (kLogLevel == 2 && isModelHarnessEndpoint(outputTarget)) {
        Q_UNUSED(label);
        const QString visibleText = level2ModelIoText(text);
        logLevel2String("ho", visibleText);
        return;
    }
    const int level = isModelHarnessEndpoint(outputTarget)
        ? 1
        : 4;
    writeLog(level, QString("%1%2 harness ho=\"%3\" holn=%4 hoby=%5 hosha=%6 hotr=%7 hotgt=\"%8\"\n")
        .arg(logoff(1))
        .arg(label)
        .arg(traceString(text))
        .arg(text.size())
        .arg(text.toUtf8().size())
        .arg(traceSha(text))
        .arg(rawResponseLine(text).size() > traceString(text).size() ? "true" : "false")
        .arg(outputTarget));
}

static void putPiece(DetBoardState *board, const QString &square, const QString &pid, const QString &side, const QString &ptype) {
    board->sq[square] = {pid, side, ptype};
}

static DetBoardState initialDetBoardState() {
    DetBoardState board;
    board.side = "W";
    putPiece(&board, "a1", "W_R1", "W", "rook");
    putPiece(&board, "b1", "W_N1", "W", "knight");
    putPiece(&board, "c1", "W_B_dark", "W", "bishop");
    putPiece(&board, "d1", "W_Q", "W", "queen");
    putPiece(&board, "e1", "W_K", "W", "king");
    putPiece(&board, "f1", "W_B_light", "W", "bishop");
    putPiece(&board, "g1", "W_N2", "W", "knight");
    putPiece(&board, "h1", "W_R2", "W", "rook");
    for (int file = 0; file < 8; ++file) {
        const QChar f = QChar('a' + file);
        putPiece(&board, QString("%1%2").arg(f).arg(2), QString("W_P%1").arg(file + 1), "W", "pawn");
        putPiece(&board, QString("%1%2").arg(f).arg(7), QString("B_P%1").arg(file + 1), "B", "pawn");
    }
    putPiece(&board, "a8", "B_R1", "B", "rook");
    putPiece(&board, "b8", "B_N1", "B", "knight");
    putPiece(&board, "c8", "B_B_light", "B", "bishop");
    putPiece(&board, "d8", "B_Q", "B", "queen");
    putPiece(&board, "e8", "B_K", "B", "king");
    putPiece(&board, "f8", "B_B_dark", "B", "bishop");
    putPiece(&board, "g8", "B_N2", "B", "knight");
    putPiece(&board, "h8", "B_R2", "B", "rook");
    return board;
}

static QJsonObject detBoardJson(const DetBoardState &board) {
    QJsonObject obj;
    obj["side"] = board.side;
    QJsonObject squares;
    QJsonObject pieces;
    for (auto it = board.sq.constBegin(); it != board.sq.constEnd(); ++it) {
        const DetPiece piece = it.value();
        squares[it.key()] = piece.pid;
        QJsonObject pieceObj;
        pieceObj["sq"] = it.key();
        pieceObj["side"] = piece.side;
        pieceObj["ptype"] = piece.ptype;
        pieces[piece.pid] = pieceObj;
    }
    obj["sq"] = squares;
    obj["pieces"] = pieces;
    return obj;
}

static QString promotionType(QChar promotion) {
    switch (promotion.toLower().toLatin1()) {
    case 'q': return "queen";
    case 'r': return "rook";
    case 'b': return "bishop";
    case 'n': return "knight";
    default: return QString();
    }
}

static void applyDetBoardMove(DetBoardState *board, const QString &uci) {
    if (uci.size() < 4) {
        return;
    }
    const QString from = uci.left(2);
    const QString to = uci.mid(2, 2);
    if (!board->sq.contains(from)) {
        board->side = board->side == "W" ? "B" : "W";
        return;
    }
    DetPiece moving = board->sq.take(from);
    board->sq.remove(to);

    if (moving.ptype == "king" && from == "e1" && to == "g1" && board->sq.contains("h1")) {
        board->sq["f1"] = board->sq.take("h1");
    } else if (moving.ptype == "king" && from == "e1" && to == "c1" && board->sq.contains("a1")) {
        board->sq["d1"] = board->sq.take("a1");
    } else if (moving.ptype == "king" && from == "e8" && to == "g8" && board->sq.contains("h8")) {
        board->sq["f8"] = board->sq.take("h8");
    } else if (moving.ptype == "king" && from == "e8" && to == "c8" && board->sq.contains("a8")) {
        board->sq["d8"] = board->sq.take("a8");
    }

    if (uci.size() >= 5) {
        const QString promoted = promotionType(uci.at(4));
        if (!promoted.isEmpty()) {
            moving.ptype = promoted;
        }
    }
    board->sq[to] = moving;
    board->side = board->side == "W" ? "B" : "W";
}

static QString squareName(int row, int col) {
    return QString("%1%2").arg(QChar('a' + col)).arg(8 - row);
}

static int rowOfSquare(const QString &square) {
    return 8 - square.mid(1, 1).toInt();
}

static int colOfSquare(const QString &square) {
    return square.at(0).toLatin1() - 'a';
}

static bool inBounds(int row, int col) {
    return row >= 0 && row < 8 && col >= 0 && col < 8;
}

static QChar fenCharForPiece(const DetPiece &piece) {
    QChar ch = '?';
    if (piece.ptype == "pawn") {
        ch = 'p';
    } else if (piece.ptype == "rook") {
        ch = 'r';
    } else if (piece.ptype == "knight") {
        ch = 'n';
    } else if (piece.ptype == "bishop") {
        ch = 'b';
    } else if (piece.ptype == "queen") {
        ch = 'q';
    } else if (piece.ptype == "king") {
        ch = 'k';
    }
    return piece.side == "W" ? ch.toUpper() : ch;
}

static QString detBoardFen(const DetBoardState &board) {
    QStringList ranks;
    for (int row = 0; row < 8; ++row) {
        QString rank;
        int empty = 0;
        for (int col = 0; col < 8; ++col) {
            const QString sq = squareName(row, col);
            if (!board.sq.contains(sq)) {
                empty += 1;
                continue;
            }
            if (empty > 0) {
                rank += QString::number(empty);
                empty = 0;
            }
            rank += fenCharForPiece(board.sq.value(sq));
        }
        if (empty > 0) {
            rank += QString::number(empty);
        }
        ranks << rank;
    }
    return QString("%1 %2 - - 0 1").arg(ranks.join('/'), board.side == "W" ? "w" : "b");
}

static bool sameSideAt(const DetBoardState &board, int row, int col, const QString &side) {
    const QString sq = squareName(row, col);
    return board.sq.contains(sq) && board.sq.value(sq).side == side;
}

static bool enemySideAt(const DetBoardState &board, int row, int col, const QString &side) {
    const QString sq = squareName(row, col);
    return board.sq.contains(sq) && board.sq.value(sq).side != side;
}

static void addDetMove(QStringList *moves, int fromRow, int fromCol, int toRow, int toCol, QChar promotion = QChar()) {
    if (!inBounds(toRow, toCol)) {
        return;
    }
    QString move = squareName(fromRow, fromCol) + squareName(toRow, toCol);
    if (!promotion.isNull()) {
        move += promotion;
    }
    moves->append(move);
}

static void addDetSlidingMoves(const DetBoardState &board,
                               QStringList *moves,
                               int row,
                               int col,
                               const QString &side,
                               const QList<QPair<int, int>> &dirs) {
    for (const QPair<int, int> &dir : dirs) {
        int nextRow = row + dir.first;
        int nextCol = col + dir.second;
        while (inBounds(nextRow, nextCol)) {
            if (sameSideAt(board, nextRow, nextCol, side)) {
                break;
            }
            addDetMove(moves, row, col, nextRow, nextCol);
            if (enemySideAt(board, nextRow, nextCol, side)) {
                break;
            }
            nextRow += dir.first;
            nextCol += dir.second;
        }
    }
}

static QStringList pseudoLegalDetMoves(const DetBoardState &board, const QString &side) {
    QStringList moves;
    for (auto it = board.sq.constBegin(); it != board.sq.constEnd(); ++it) {
        const DetPiece piece = it.value();
        if (piece.side != side) {
            continue;
        }
        const int row = rowOfSquare(it.key());
        const int col = colOfSquare(it.key());
        if (piece.ptype == "pawn") {
            const int dir = side == "W" ? -1 : 1;
            const int startRow = side == "W" ? 6 : 1;
            const int promotionRow = side == "W" ? 0 : 7;
            const int oneRow = row + dir;
            if (inBounds(oneRow, col) && !board.sq.contains(squareName(oneRow, col))) {
                addDetMove(&moves, row, col, oneRow, col, oneRow == promotionRow ? QChar('q') : QChar());
                const int twoRow = row + 2 * dir;
                if (row == startRow && inBounds(twoRow, col) && !board.sq.contains(squareName(twoRow, col))) {
                    addDetMove(&moves, row, col, twoRow, col);
                }
            }
            for (int dc : {-1, 1}) {
                const int captureCol = col + dc;
                if (inBounds(oneRow, captureCol) && enemySideAt(board, oneRow, captureCol, side)) {
                    addDetMove(&moves, row, col, oneRow, captureCol, oneRow == promotionRow ? QChar('q') : QChar());
                }
            }
        } else if (piece.ptype == "knight") {
            const int jumps[8][2] = {{-2,-1},{-2,1},{-1,-2},{-1,2},{1,-2},{1,2},{2,-1},{2,1}};
            for (const auto &jump : jumps) {
                const int nextRow = row + jump[0];
                const int nextCol = col + jump[1];
                if (inBounds(nextRow, nextCol) && !sameSideAt(board, nextRow, nextCol, side)) {
                    addDetMove(&moves, row, col, nextRow, nextCol);
                }
            }
        } else if (piece.ptype == "bishop") {
            addDetSlidingMoves(board, &moves, row, col, side, {{-1,-1},{-1,1},{1,-1},{1,1}});
        } else if (piece.ptype == "rook") {
            addDetSlidingMoves(board, &moves, row, col, side, {{-1,0},{1,0},{0,-1},{0,1}});
        } else if (piece.ptype == "queen") {
            addDetSlidingMoves(board, &moves, row, col, side, {{-1,-1},{-1,1},{1,-1},{1,1},{-1,0},{1,0},{0,-1},{0,1}});
        } else if (piece.ptype == "king") {
            for (int dr = -1; dr <= 1; ++dr) {
                for (int dc = -1; dc <= 1; ++dc) {
                    if (dr == 0 && dc == 0) {
                        continue;
                    }
                    const int nextRow = row + dr;
                    const int nextCol = col + dc;
                    if (inBounds(nextRow, nextCol) && !sameSideAt(board, nextRow, nextCol, side)) {
                        addDetMove(&moves, row, col, nextRow, nextCol);
                    }
                }
            }
        }
    }
    return moves;
}

static bool detSquareAttacked(const DetBoardState &board, int targetRow, int targetCol, const QString &bySide) {
    for (auto it = board.sq.constBegin(); it != board.sq.constEnd(); ++it) {
        const DetPiece piece = it.value();
        if (piece.side != bySide) {
            continue;
        }
        const int row = rowOfSquare(it.key());
        const int col = colOfSquare(it.key());
        if (piece.ptype == "pawn") {
            const int dir = bySide == "W" ? -1 : 1;
            if (targetRow == row + dir && (targetCol == col - 1 || targetCol == col + 1)) {
                return true;
            }
        } else if (piece.ptype == "knight") {
            const int dr = qAbs(targetRow - row);
            const int dc = qAbs(targetCol - col);
            if ((dr == 2 && dc == 1) || (dr == 1 && dc == 2)) {
                return true;
            }
        } else if (piece.ptype == "king") {
            if (qAbs(targetRow - row) <= 1 && qAbs(targetCol - col) <= 1) {
                return true;
            }
        } else {
            QList<QPair<int, int>> dirs;
            if (piece.ptype == "bishop") {
                dirs = {{-1,-1},{-1,1},{1,-1},{1,1}};
            } else if (piece.ptype == "rook") {
                dirs = {{-1,0},{1,0},{0,-1},{0,1}};
            } else if (piece.ptype == "queen") {
                dirs = {{-1,-1},{-1,1},{1,-1},{1,1},{-1,0},{1,0},{0,-1},{0,1}};
            }
            for (const QPair<int, int> &dir : dirs) {
                int nextRow = row + dir.first;
                int nextCol = col + dir.second;
                while (inBounds(nextRow, nextCol)) {
                    if (nextRow == targetRow && nextCol == targetCol) {
                        return true;
                    }
                    if (board.sq.contains(squareName(nextRow, nextCol))) {
                        break;
                    }
                    nextRow += dir.first;
                    nextCol += dir.second;
                }
            }
        }
    }
    return false;
}

static bool detKingInCheck(const DetBoardState &board, const QString &side) {
    for (auto it = board.sq.constBegin(); it != board.sq.constEnd(); ++it) {
        const DetPiece piece = it.value();
        if (piece.side == side && piece.ptype == "king") {
            const QString enemy = side == "W" ? "B" : "W";
            return detSquareAttacked(board, rowOfSquare(it.key()), colOfSquare(it.key()), enemy);
        }
    }
    return true;
}

static QStringList legalDetMoves(const DetBoardState &board) {
    QStringList legal;
    const QString movingSide = board.side;
    for (const QString &move : pseudoLegalDetMoves(board, movingSide)) {
        DetBoardState next = board;
        applyDetBoardMove(&next, move);
        if (!detKingInCheck(next, movingSide)) {
            legal << move;
        }
    }
    legal.sort();
    return legal;
}

static QString moveFromReportedFenTransition(const DetBoardState &beforeBoard,
                                             const QStringList &legalMoves,
                                             const QString &reportedBeforeFen,
                                             const QString &reportedAfterFen,
                                             QString *reason) {
    const QString expectedBeforeFen = detBoardFen(beforeBoard);
    if (reportedBeforeFen != expectedBeforeFen) {
        if (reason) {
            *reason = "bf_mismatch";
        }
        return QString();
    }
    if (reportedAfterFen.isEmpty()) {
        if (reason) {
            *reason = "af_missing";
        }
        return QString();
    }

    QStringList matchingMoves;
    for (const QString &move : legalMoves) {
        DetBoardState afterBoard = beforeBoard;
        applyDetBoardMove(&afterBoard, move);
        if (detBoardFen(afterBoard) == reportedAfterFen) {
            matchingMoves << move;
        }
    }
    if (matchingMoves.size() == 1) {
        if (reason) {
            *reason = "bf_af_match_one_legal_move";
        }
        return matchingMoves.first();
    }
    if (reason) {
        *reason = matchingMoves.isEmpty() ? "af_matches_no_legal_move" : "af_matches_multiple_legal_moves";
    }
    return QString();
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

static QStringList normalizeWrapperCliArgs(const QStringList &args) {
    if (args.isEmpty()) {
        return args;
    }

    const QMap<QString, QString> valueFlags{
        {"-nb", "--boards"},
        {"-nl", "--loops"},
        {"-mt", "--move-timeout"},
        {"-sto", "--stack-timeout"},
        {"-otkns", "--otkns"},
        {"-gmto", "--gmto"},
        {"-mxply", "--mxply"},
        {"-cnrtlm", "--cnrtlm"},
        {"-lkahdlvl", "--lkahdlvl"},
        {"-lokahdlvl", "--lkahdlvl"}
    };
    const QMap<QString, QString> boolFlags{
        {"-aot", "--auto-output-tokens"},
        {"-bap", "--board-awareness-probe"},
        {"-avb", "--avb"},
        {"-avm", "--avm"},
        {"-ans", "--ans"}
    };

    QStringList normalized{args.first()};
    for (int i = 1; i < args.size(); ++i) {
        const QString arg = args.at(i);
        if (valueFlags.contains(arg)) {
            normalized << valueFlags.value(arg);
            if (i + 1 < args.size()) {
                normalized << args.at(++i);
            }
            continue;
        }
        if (boolFlags.contains(arg)) {
            normalized << boolFlags.value(arg);
            continue;
        }
        normalized << arg;
    }
    return normalized;
}

static bool hasAnyOpenAiAgent(const QStringList &models) {
    for (const QString &model : models) {
        if (model.startsWith("openai:", Qt::CaseInsensitive) ||
            model.startsWith("gpt-", Qt::CaseInsensitive) ||
            model.startsWith("chat-", Qt::CaseInsensitive)) {
            return true;
        }
    }
    return false;
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

    QString initialFen() {
        return fenForPosition("position startpos");
    }

    QString fenAfterMove(const QString &fen, const QString &move) {
        return fenForPosition(QString("position fen %1 moves %2").arg(fen, move));
    }

    QStringList legalMovesForFen(const QString &fen) {
        return legalMovesForPosition(QString("position fen %1").arg(fen));
    }

    bool sideToMoveInCheckForFen(const QString &fen) {
        return sideToMoveInCheckForPosition(QString("position fen %1").arg(fen));
    }

    QString fenForMoves(const QStringList &moves) {
        return fenForPosition(positionCommand(moves));
    }

    QStringList legalMoves(const QStringList &moves) {
        return legalMovesForPosition(positionCommand(moves));
    }

    bool sideToMoveInCheck(const QStringList &moves) {
        return sideToMoveInCheckForPosition(positionCommand(moves));
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

    QString fenForPosition(const QString &positionCmd) {
        const QString stockfishInput = positionCmd + "; d";
        logHarnessOutput("stockfish fen", "stockfish", stockfishInput);
        setPosition(positionCmd);
        writeLine("d");
        const QString output = readUntil(QRegularExpression("^Checkers:"), 5000);
        const QStringList lines = splitLines(output);
        for (const QString &line : lines) {
            if (line.startsWith("Fen: ")) {
                const QString fen = line.mid(5).trimmed();
                logHarnessInput("stockfish fen", "stockfish", fen);
                return fen;
            }
        }
        const QString fallback = positionCmd == "position startpos"
            ? startFen()
            : QString();
        logHarnessInput("stockfish fen", "stockfish", fallback);
        return fallback;
    }

    QStringList legalMovesForPosition(const QString &positionCmd) {
        const QString stockfishInput = positionCmd + "; go perft 1";
        logHarnessOutput("stockfish legal", "stockfish", stockfishInput);
        setPosition(positionCmd);
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
        logHarnessInput("stockfish legal", "stockfish", legal.join(' '));
        return legal;
    }

    bool sideToMoveInCheckForPosition(const QString &positionCmd) {
        const QString stockfishInput = positionCmd + "; d";
        logHarnessOutput("stockfish check", "stockfish", stockfishInput);
        setPosition(positionCmd);
        writeLine("d");
        const QString output = readUntil(QRegularExpression("^Checkers:"), 5000);
        for (const QString &line : splitLines(output)) {
            if (line.startsWith("Checkers:")) {
                const bool inCheck = !line.mid(QString("Checkers:").size()).trimmed().isEmpty();
                logHarnessInput("stockfish check", "stockfish", inCheck ? "true" : "false");
                return inCheck;
            }
        }
        logHarnessInput("stockfish check", "stockfish", "false");
        return false;
    }

    void writeLine(const QString &line) {
        proc_.write((line + "\n").toUtf8());
        proc_.waitForBytesWritten(1000);
    }

    QString startFen() const {
        return "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    }

    QString positionCommand(const QStringList &moves) const {
        QString command = "position startpos";
        if (!moves.isEmpty()) {
            command += " moves " + moves.join(' ');
        }
        return command;
    }

    void setPosition(const QString &positionCmd) {
        writeLine(positionCmd);
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

struct MoveParseResult {
    QString selected;
    QStringList candidates;
    QString reason;
};

struct BoardAwarenessResult {
    QString expectedFen;
    QString reportedFen;
    QString expectedSide;
    QString reportedSide;
    QString expectedOccupied;
    QString reportedOccupied;
    bool fenMatches = false;
    bool sideMatches = false;
    bool occupiedMatches = false;
};

static QStringList extractUciCandidates(const QString &text) {
    static const QRegularExpression uci("\\b[a-h][1-8][a-h][1-8][qrbn]?\\b",
                                        QRegularExpression::CaseInsensitiveOption);
    QStringList candidates;
    QRegularExpressionMatchIterator it = uci.globalMatch(text);
    while (it.hasNext()) {
        candidates << it.next().captured(0).toLower();
    }
    return candidates;
}

static QString fieldValue(const QString &text, const QString &field) {
    const QRegularExpression pattern(QString("(^|\\n)%1=([^\\n\\r]+)").arg(QRegularExpression::escape(field)),
                                     QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch match = pattern.match(text);
    return match.hasMatch() ? match.captured(2).trimmed().simplified() : QString();
}

static QJsonArray jsonArrayFromStrings(const QStringList &items) {
    QJsonArray array;
    for (const QString &item : items) {
        array.append(item);
    }
    return array;
}

static void logParserInput(const QString &text) {
    int nullCount = 0;
    for (const QChar ch : text) {
        if (ch.unicode() == 0) {
            nullCount += 1;
        }
    }
    writeLog(4, QString("%1parser pinstr=\"%2\" pinstrlen=%3 input_bytes=%4 input_nulls=%5 pinsha256=%6 pintrunc=%7\n")
        .arg(logoff(2))
        .arg(traceString(text))
        .arg(text.size())
        .arg(text.toUtf8().size())
        .arg(nullCount)
        .arg(traceSha256(text))
        .arg(rawResponseLine(text).size() > traceString(text).size() ? "true" : "false"));
}

static void logParserOutput(const MoveParseResult &result) {
    writeLog(4, QString("%1parser poutstr=\"%2\" poutstrlen=%3 poutsha256=%4 pouttrunc=%5\n")
        .arg(logoff(2))
        .arg(traceString(result.selected))
        .arg(result.selected.size())
        .arg(traceSha256(result.selected))
        .arg(rawResponseLine(result.selected).size() > traceString(result.selected).size() ? "true" : "false"));
}

static void logHarnessRoute(const QString &label,
                            const QString &routeTarget,
                            const QString &routeReason) {
    writeLog(3, QString("%1%2 harness houttgt=\"%3\" reason=\"%4\"\n")
        .arg(logoff(1))
        .arg(label)
        .arg(routeTarget)
        .arg(routeReason));
}

static void logHarnessParserInput(const QString &label, const QString &inputSource, const QString &text) {
    logHarnessInput(label, inputSource, text);
}

static void logHarnessParserOutput(const QString &label, const QString &outputTarget, const MoveParseResult &parse) {
    logHarnessOutput(label, outputTarget, parse.selected);
}

static void logAgentOutput(const QString &label, const OllamaResult &response) {
    writeLog(3, QString("%1%2 ollama_output status=\"%3\" done=%4 done_reason=\"%5\" output=\"%6\" outstrlen=%7 outsha256=%8 outtrunc=%9\n")
        .arg(logoff(2))
        .arg(label)
        .arg(response.status)
        .arg(response.backendDone ? "true" : "false")
        .arg(response.doneReason)
        .arg(traceString(response.stdoutText))
        .arg(response.stdoutText.size())
        .arg(traceSha256(response.stdoutText))
        .arg(rawResponseLine(response.stdoutText).size() > traceString(response.stdoutText).size() ? "true" : "false"));
    writeLog(3, QString("%1%2 agent_output status=\"%3\" done=%4 done_reason=\"%5\" output=\"%6\" outstrlen=%7 outsha256=%8 outtrunc=%9\n")
        .arg(logoff(2))
        .arg(label)
        .arg(response.status)
        .arg(response.backendDone ? "true" : "false")
        .arg(response.doneReason)
        .arg(traceString(response.stdoutText))
        .arg(response.stdoutText.size())
        .arg(traceSha256(response.stdoutText))
        .arg(rawResponseLine(response.stdoutText).size() > traceString(response.stdoutText).size() ? "true" : "false"));
}

static MoveParseResult parseMoveReply(const QString &prompt,
                                      const QString &reply,
                                      const QStringList &legalMoves = QStringList(),
                                      const QString &previousRejectedMove = QString(),
                                      const QString &source = "agent_backend",
                                      const QString &target = "move_candidate") {
    Q_UNUSED(prompt);
    Q_UNUSED(source);
    Q_UNUSED(target);

    logParserInput(reply);

    MoveParseResult result;
    const QString previous = previousRejectedMove.toLower();
    result.candidates = extractUciCandidates(reply);
    if (result.candidates.isEmpty()) {
        result.reason = "no_uci_candidate";
        logParserOutput(result);
        return result;
    }

    const bool hasLegalMoves = !legalMoves.isEmpty();

    if (hasLegalMoves) {
        for (const QString &candidate : result.candidates) {
            if (!previous.isEmpty() && candidate == previous) {
                continue;
            }
            if (legalMoves.contains(candidate)) {
                result.selected = candidate;
                result.reason = previous.isEmpty()
                    ? "selected_first_legal_candidate"
                    : "selected_first_legal_candidate_not_previous_rejection";
                logParserOutput(result);
                return result;
            }
        }
    }

    for (const QString &candidate : result.candidates) {
        if (!previous.isEmpty() && candidate == previous) {
            continue;
        }
        result.selected = candidate;
        result.reason = previous.isEmpty()
            ? "selected_first_candidate"
            : "selected_first_candidate_not_previous_rejection";
        logParserOutput(result);
        return result;
    }

    result.reason = "no_new_candidate_only_previous_rejection";
    logParserOutput(result);
    return result;
}

static MoveParseResult parseViaHarness(const QString &label,
                                       const QString &inputSource,
                                       const QString &outputTarget,
                                       const QString &prompt,
                                       const QString &reply,
                                       const QStringList &legalMoves = QStringList(),
                                       const QString &previousRejectedMove = QString()) {
    logHarnessParserInput(label, inputSource, reply);
    MoveParseResult result = parseMoveReply(prompt, reply, legalMoves, previousRejectedMove);
    logHarnessParserOutput(label, outputTarget, result);
    return result;
}

static QJsonObject moveParseJson(const MoveParseResult &parse,
                                 const QString &promptType,
                                 const QString &prompt,
                                 const QString &previousRejectedMove = QString()) {
    QJsonObject obj;
    obj["prompt_type"] = promptType;
    obj["prompt"] = prompt;
    obj["previous_rejected_move"] = previousRejectedMove;
    obj["selected_uci"] = parse.selected;
    obj["candidate_uci_moves"] = jsonArrayFromStrings(parse.candidates);
    obj["selection_reason"] = parse.reason;
    obj["parser_policy"] = "uci_candidates_prefer_deterministic_legal_move";
    return obj;
}

static int parserInputNullCount(const QString &text) {
    int nullCount = 0;
    for (const QChar ch : text) {
        if (ch.unicode() == 0) {
            nullCount += 1;
        }
    }
    return nullCount;
}

static bool doneReasonSuggestsTokenLimit(const QString &doneReason) {
    const QString lower = doneReason.toLower();
    return lower.contains("length") || lower.contains("limit");
}

static int tuneOutputTokens(const QString &label,
                            int currentOutputTokens,
                            const OllamaResult &response,
                            const MoveParseResult &parse) {
    const int maxOutputTokens = 1024;
    if (currentOutputTokens >= maxOutputTokens || response.status != "completed") {
        return currentOutputTokens;
    }

    const int byteLen = response.stdoutText.toUtf8().size();
    const int nulls = parserInputNullCount(response.stdoutText);
    const bool nearBudgetBySize = byteLen >= currentOutputTokens * 3;
    const bool noReplacementAfterCorrection = parse.reason == "no_new_candidate_only_previous_rejection";
    const bool noCandidateNearBudget = parse.selected.isEmpty() && nearBudgetBySize;
    const bool shouldIncrease = doneReasonSuggestsTokenLimit(response.doneReason) ||
        noCandidateNearBudget ||
        (noReplacementAfterCorrection && nearBudgetBySize);

    if (!shouldIncrease) {
        return currentOutputTokens;
    }

    const int nextOutputTokens = qMin(maxOutputTokens, qMax(currentOutputTokens + 64, currentOutputTokens * 2));
        writeLog(3, QString("%1 output_token_tune old=%2 new=%3 input_bytes=%4 nulls=%5 done_reason=\"%6\" parse_reason=\"%7\"\n")
        .arg(label)
        .arg(currentOutputTokens)
        .arg(nextOutputTokens)
        .arg(byteLen)
        .arg(nulls)
        .arg(response.doneReason)
        .arg(parse.reason));
    return nextOutputTokens;
}

static bool shouldRetryWithTunedOutputTokens(int oldOutputTokens, int newOutputTokens) {
    return newOutputTokens > oldOutputTokens;
}

static QJsonObject ollamaJson(const OllamaResult &result);

static QString occupiedSquaresFromFen(const QString &fen) {
    const QString board = fen.section(' ', 0, 0);
    QStringList occupied;
    int rank = 8;
    int fileIndex = 0;
    const QString files = "abcdefgh";
    for (const QChar ch : board) {
        if (ch == '/') {
            rank -= 1;
            fileIndex = 0;
            continue;
        }
        if (ch.isDigit()) {
            fileIndex += ch.digitValue();
            continue;
        }
        if (fileIndex >= 0 && fileIndex < files.size() && rank >= 1 && rank <= 8) {
            occupied << QString("%1%2=%3").arg(files.at(fileIndex)).arg(rank).arg(ch);
        }
        fileIndex += 1;
    }
    return occupied.join(' ');
}

static QString sideFromFen(const QString &fen) {
    return fen.section(' ', 1, 1).trimmed();
}

static QString normalizedFieldValue(const QString &text, const QString &field) {
    const QRegularExpression pattern(QString("(^|\\n)%1=([^\\n\\r]+)").arg(QRegularExpression::escape(field)),
                                     QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch match = pattern.match(text);
    if (!match.hasMatch()) {
        return QString();
    }
    return match.captured(2).trimmed().simplified();
}

static QString normalizedSpaceList(const QString &text) {
    return text.trimmed().simplified();
}

static QString promptForPreMoveBoardAwareness(const QString &fen,
                                              const QStringList &legalMoves,
                                              int ply) {
    return QString(
        "Pre-move board awareness check.\n"
        "Ply: %1\n"
        "Current board FEN: %2\n"
        "Current legal UCI moves: %3\n"
        "Return exactly three lines for the current board before making a move:\n"
        "fen=<the full current board FEN exactly as given>\n"
        "side=<w or b from that FEN>\n"
        "occupied=<space-separated occupied squares from a8 to h1 as square=piece, derived from the FEN>\n"
        "Do not return a move in this reply.")
        .arg(ply)
        .arg(fen)
        .arg(legalMoves.join(' '));
}

static QString promptForPostMoveBoardAwareness(const QString &movePrompt,
                                               const QString &moveReply,
                                               const QString &candidateMove,
                                               int ply) {
    return QString(
        "Post-move board awareness check.\n"
        "You are being given the transcript of the move request immediately before this check.\n"
        "Ply: %1\n"
        "Previous prompt:\n"
        "%2\n"
        "Previous reply:\n"
        "%3\n"
        "Candidate move parsed from previous reply: %4\n"
        "Return exactly three lines for the board after that candidate move:\n"
        "fen=<full FEN after the candidate move>\n"
        "side=<w or b from that FEN>\n"
        "occupied=<space-separated occupied squares from a8 to h1 as square=piece, derived from the FEN>\n"
        "Do not return a move in this reply.")
        .arg(ply)
        .arg(movePrompt)
        .arg(moveReply)
        .arg(candidateMove.isEmpty() ? "(none)" : candidateMove);
}

static BoardAwarenessResult evaluateBoardAwareness(const QString &fen, const QString &reply) {
    BoardAwarenessResult result;
    result.expectedFen = fen.trimmed();
    result.reportedFen = normalizedFieldValue(reply, "fen");
    result.expectedSide = sideFromFen(fen);
    result.reportedSide = normalizedFieldValue(reply, "side").toLower();
    result.expectedOccupied = occupiedSquaresFromFen(fen);
    result.reportedOccupied = normalizedFieldValue(reply, "occupied");
    result.fenMatches = result.reportedFen == result.expectedFen;
    result.sideMatches = result.reportedSide == result.expectedSide;
    result.occupiedMatches = normalizedSpaceList(result.reportedOccupied) == normalizedSpaceList(result.expectedOccupied);
    return result;
}

static QJsonObject boardAwarenessJson(const BoardAwarenessResult &result, const OllamaResult &response) {
    QJsonObject obj;
    obj["response"] = ollamaJson(response);
    obj["expected_fen"] = result.expectedFen;
    obj["reported_fen"] = result.reportedFen;
    obj["fen_matches"] = result.fenMatches;
    obj["expected_side"] = result.expectedSide;
    obj["reported_side"] = result.reportedSide;
    obj["side_matches"] = result.sideMatches;
    obj["expected_occupied"] = result.expectedOccupied;
    obj["reported_occupied"] = result.reportedOccupied;
    obj["occupied_matches"] = result.occupiedMatches;
    obj["passed"] = result.fenMatches && result.sideMatches && result.occupiedMatches;
    return obj;
}

static QString formatElapsed(double seconds) {
    return QString("%1s").arg(QString::number(seconds, 'E', 1), 9, QChar('0'));
}

static void countRejectedMove(const QString &parsed, bool legalMove, int *illegalTotal, int *invalidTotal) {
    if (legalMove) {
        return;
    }
    if (parsed.isEmpty()) {
        *invalidTotal += 1;
    } else {
        *illegalTotal += 1;
    }
}

static QString count3(int value) {
    return QString("%1").arg(value, 3, 10, QChar('0'));
}

static QString errorCountText(int illegalTotal, int invalidTotal) {
    return QString(" il=%1 iv=%2 nr=%3")
        .arg(count3(illegalTotal))
        .arg(count3(invalidTotal))
        .arg(count3(illegalTotal + invalidTotal));
}

static QString finalCountText(int illegalTotal, int invalidTotal, int correctionLimit) {
    return QString("nil=%1 niv=%2 ncr=%3")
        .arg(count3(illegalTotal), count3(invalidTotal), count3(correctionLimit));
}

static QString promptForMove(const QString &fen,
                             const QStringList &legalMoves,
                             int ply,
                             int lookaheadLevel = 0,
                             bool includeLegalMoveList = false) {
    Q_UNUSED(lookaheadLevel);
    QString prompt = QString(
        "Chess move request.\n"
        "Ply: %1\n"
        "FEN: %2\n")
        .arg(ply)
        .arg(fen);
    if (includeLegalMoveList) {
        prompt += QString("Legal UCI moves: %1\n").arg(legalMoves.join(' '));
    }
    prompt +=
        "Return exactly two lines:\n"
        "bf=<the full current board FEN exactly as given>\n"
        "af=<the full board FEN after your move>\n"
        "Do not include commentary.";
    return prompt;
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

static QString promptForCorrection(const QString &fen, const QStringList &legalMoves, const QString &candidateMove) {
    Q_UNUSED(legalMoves);
    const QString verdict = candidateMove.isEmpty()
        ? "Your response did not contain a valid UCI move."
        : QString("Your move %1 is illegal.").arg(candidateMove);
    return QString(
        "%1\n"
        "FEN: %2\n"
        "Return exactly two lines:\n"
        "bf=<the full current board FEN exactly as given>\n"
        "af=<the full board FEN after your move>\n"
        "Do not include commentary.")
        .arg(verdict)
        .arg(fen);
}

static QString promptForAgentOnlyMove(const QStringList &moves, const QString &side, int ply) {
    return QString(
        "Chess move request.\n"
        "Ply: %1\n"
        "Side to move: %2\n"
        "Move history: %3\n"
        "Return one UCI move.")
        .arg(ply)
        .arg(side)
        .arg(moves.isEmpty() ? "(none)" : moves.join(' '));
}

static QString promptForAgentOnlyReferee(const QStringList &moves,
                                         const QString &side,
                                         const QString &candidateMove,
                                         int ply) {
    return QString(
        "Chess referee request.\n"
        "Ply: %1\n"
        "Side to move: %2\n"
        "Move history: %3\n"
        "Candidate move: %4\n"
        "Answer legal or illegal.")
        .arg(ply)
        .arg(side)
        .arg(moves.isEmpty() ? "(none)" : moves.join(' '))
        .arg(candidateMove.isEmpty() ? "(none)" : candidateMove);
}

static QString promptForAgentOnlyCorrection(const QStringList &moves,
                                            const QString &side,
                                            const QString &candidateMove) {
    const QString verdict = candidateMove.isEmpty()
        ? "Your response did not contain a valid UCI move."
        : QString("The referee rejected your move %1 as illegal.").arg(candidateMove);
    return QString(
        "%1\n"
        "Side to move: %2\n"
        "Move history: %3\n"
        "Return one replacement UCI move.")
        .arg(verdict)
        .arg(side)
        .arg(moves.isEmpty() ? "(none)" : moves.join(' '));
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

static bool isOpenAiModel(const QString &model) {
    return model.startsWith("openai:", Qt::CaseInsensitive);
}

static QString openAiModelName(const QString &model) {
    const QString name = isOpenAiModel(model) ? model.section(':', 1) : model;
    return name.isEmpty() ? QString::fromLocal8Bit(qgetenv("AICHESS_OPENAI_MODEL")) : name;
}

static QString normalizedStackModule(const QString &stackModule) {
    const QString normalized = stackModule.trimmed().toLower();
    return normalized.isEmpty() ? "auto" : normalized;
}

static bool isAutoStackModule(const QString &stackModule) {
    const QString normalized = normalizedStackModule(stackModule);
    return normalized == "auto" || normalized == "agent_auto" || normalized == "auto_agent";
}

static bool isOpenAiStackModule(const QString &stackModule) {
    const QString normalized = normalizedStackModule(stackModule);
    return normalized == "openai_responses" || normalized == "openai" || normalized == "cloud_openai";
}

static bool isOllamaStackModule(const QString &stackModule) {
    const QString normalized = normalizedStackModule(stackModule);
    return normalized == "ollama_generate" || normalized == "ollama" || normalized == "local_ollama";
}

static QString stackModuleForAgentModel(const QString &requestedStackModule, const QString &model) {
    if (!isAutoStackModule(requestedStackModule)) {
        return normalizedStackModule(requestedStackModule);
    }
    if (isOpenAiModel(model) || model.startsWith("gpt-", Qt::CaseInsensitive) || model.startsWith("chat-", Qt::CaseInsensitive)) {
        return "openai_responses";
    }
    return "ollama_generate";
}

static QString responseOutputText(const QJsonObject &obj) {
    const QString direct = obj.value("output_text").toString().trimmed();
    if (!direct.isEmpty()) {
        return direct;
    }

    QStringList parts;
    const QJsonArray output = obj.value("output").toArray();
    for (const QJsonValue &outputValue : output) {
        const QJsonObject outputObj = outputValue.toObject();
        const QJsonArray content = outputObj.value("content").toArray();
        for (const QJsonValue &contentValue : content) {
            const QJsonObject contentObj = contentValue.toObject();
            const QString text = contentObj.value("text").toString().trimmed();
            if (!text.isEmpty()) {
                parts << text;
            }
        }
    }
    return parts.join('\n').trimmed();
}

static OllamaResult askOllama(const QString &model,
                              const QString &prompt,
                              int timeoutSeconds,
                              int numPredict,
                              const QString &startupLabel = QString()) {
    QElapsedTimer timer;
    timer.start();
    OllamaResult result;
    const QString traceLabel = startupLabel.isEmpty() ? QString("ollama") : startupLabel + " ollama";
    const QString traceEndpoint = "ollama:" + model;
    logHarnessOutput(traceLabel, traceEndpoint, prompt);

    QJsonObject options;
    options["temperature"] = 0;
    options["num_predict"] = numPredict;

    QJsonObject payload;
    payload["model"] = model;
    payload["prompt"] = prompt;
    payload["stream"] = false;
    payload["think"] = false;
    payload["options"] = options;

    QNetworkAccessManager manager;
    QNetworkRequest request(QUrl("http://127.0.0.1:11434/api/generate"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply *reply = manager.post(request, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    QEventLoop loop;
    QTimer timeout;
    QTimer startupProgress;
    QString startupDots;
    timeout.setSingleShot(true);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
    if (!startupLabel.isEmpty()) {
        writeLog(3, QString("%1: starting\n").arg(startupLabel));
        startupProgress.setInterval(1000);
        QObject::connect(&startupProgress, &QTimer::timeout, [&startupLabel, &startupDots]() {
            startupDots += ".";
            writeLog(3, QString("%1: starting%2\n").arg(startupLabel, startupDots));
        });
        startupProgress.start();
    }
    timeout.start(timeoutSeconds * 1000);
    loop.exec();
    if (startupProgress.isActive()) {
        startupProgress.stop();
    }

    result.elapsedSeconds = timer.elapsed() / 1000.0;
    if (timeout.isActive()) {
        timeout.stop();
    } else {
        reply->abort();
        reply->deleteLater();
        result.status = "timed_out";
        logHarnessInput(traceLabel, traceEndpoint, result.stdoutText);
        return result;
    }

    const QByteArray body = reply->readAll();
    if (reply->error() != QNetworkReply::NoError) {
        result.status = "request_failed";
        result.elapsedSeconds = timer.elapsed() / 1000.0;
        result.stderrText = reply->errorString();
        result.stdoutText = QString::fromUtf8(body).trimmed();
        reply->deleteLater();
        logHarnessInput(traceLabel, traceEndpoint, result.stdoutText);
        return result;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        result.status = "bad_json";
        result.stdoutText = QString::fromUtf8(body).trimmed();
        result.stderrText = parseError.errorString();
        reply->deleteLater();
        logHarnessInput(traceLabel, traceEndpoint, result.stdoutText);
        return result;
    }

    const QJsonObject obj = doc.object();
    result.status = "completed";
    result.exitCode = 0;
    result.backendDone = obj.value("done").toBool(false);
    result.doneReason = obj.value("done_reason").toString();
    result.stdoutText = obj.value("response").toString().trimmed();
    if (!result.backendDone) {
        result.status = "incomplete_response";
        result.stderrText = "Ollama response did not report done=true";
    }
    if (obj.contains("error")) {
        result.status = "request_failed";
        result.stderrText = obj.value("error").toString();
    }
    reply->deleteLater();
    logHarnessInput(traceLabel, traceEndpoint, result.stdoutText);
    return result;
}

static OllamaResult askOpenAi(const QString &model,
                              const QString &prompt,
                              int timeoutSeconds,
                              int numPredict,
                              const QString &startupLabel = QString()) {
    QElapsedTimer timer;
    timer.start();
    OllamaResult result;
    const QString traceLabel = startupLabel.isEmpty() ? QString("openai") : startupLabel + " openai";

    const QString apiKey = QString::fromLocal8Bit(qgetenv("OPENAI_API_KEY"));
    const QString resolvedModel = openAiModelName(model);
    const QString traceEndpoint = resolvedModel.isEmpty() ? QString("openai") : "openai:" + resolvedModel;
    logHarnessOutput(traceLabel, traceEndpoint, prompt);
    if (apiKey.isEmpty()) {
        result.status = "request_failed";
        result.stderrText = "OPENAI_API_KEY is not set";
        logHarnessInput(traceLabel, traceEndpoint, result.stdoutText);
        return result;
    }
    if (resolvedModel.isEmpty()) {
        result.status = "request_failed";
        result.stderrText = "OpenAI model is empty; use openai:<model> or AICHESS_OPENAI_MODEL";
        logHarnessInput(traceLabel, traceEndpoint, result.stdoutText);
        return result;
    }

    QJsonObject payload;
    payload["model"] = resolvedModel;
    payload["input"] = prompt;
    payload["max_output_tokens"] = numPredict;

    QNetworkAccessManager manager;
    QNetworkRequest request{QUrl(kOpenAiEndpoint)};
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + apiKey.toUtf8());

    QNetworkReply *reply = manager.post(request, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    QEventLoop loop;
    QTimer timeout;
    QTimer startupProgress;
    QString startupDots;
    timeout.setSingleShot(true);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
    if (!startupLabel.isEmpty()) {
        writeLog(3, QString("%1: starting cloud\n").arg(startupLabel));
        startupProgress.setInterval(1000);
        QObject::connect(&startupProgress, &QTimer::timeout, [&startupLabel, &startupDots]() {
            startupDots += ".";
            writeLog(3, QString("%1: starting cloud%2\n").arg(startupLabel, startupDots));
        });
        startupProgress.start();
    }
    timeout.start(timeoutSeconds * 1000);
    loop.exec();
    if (startupProgress.isActive()) {
        startupProgress.stop();
    }

    result.elapsedSeconds = timer.elapsed() / 1000.0;
    if (timeout.isActive()) {
        timeout.stop();
    } else {
        reply->abort();
        reply->deleteLater();
        result.status = "timed_out";
        logHarnessInput(traceLabel, traceEndpoint, result.stdoutText);
        return result;
    }

    const QByteArray body = reply->readAll();
    if (reply->error() != QNetworkReply::NoError) {
        result.status = "request_failed";
        result.exitCode = int(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt());
        result.stderrText = reply->errorString();
        result.stdoutText = QString::fromUtf8(body).trimmed();
        reply->deleteLater();
        logHarnessInput(traceLabel, traceEndpoint, result.stdoutText);
        return result;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        result.status = "bad_json";
        result.stdoutText = QString::fromUtf8(body).trimmed();
        result.stderrText = parseError.errorString();
        reply->deleteLater();
        logHarnessInput(traceLabel, traceEndpoint, result.stdoutText);
        return result;
    }

    const QJsonObject obj = doc.object();
    result.status = "completed";
    result.exitCode = 0;
    result.backendDone = true;
    result.doneReason = obj.value("status").toString();
    result.stdoutText = responseOutputText(obj);
    if (obj.contains("error")) {
        result.status = "request_failed";
        const QJsonObject error = obj.value("error").toObject();
        result.stderrText = error.value("message").toString();
    }
    reply->deleteLater();
    logHarnessInput(traceLabel, traceEndpoint, result.stdoutText);
    return result;
}

static OllamaResult askStackModule(const QString &stackModule,
                                   const QString &model,
                                   const QString &prompt,
                                   int timeoutSeconds,
                                   int numPredict,
                                   const QString &startupLabel = QString()) {
    if (isOpenAiStackModule(stackModule)) {
        return askOpenAi(model, prompt, timeoutSeconds, numPredict, startupLabel);
    }
    if (isOllamaStackModule(stackModule)) {
        return askOllama(model, prompt, timeoutSeconds, numPredict, startupLabel);
    }

    OllamaResult result;
    result.status = "request_failed";
    result.stderrText = QString("unknown stack module: %1").arg(stackModule);
    return result;
}

static QJsonObject stackModuleCapabilities(const QString &stackModule) {
    QJsonObject obj;
    const QString normalized = normalizedStackModule(stackModule);
    obj["stkmdl"] = normalized;
    obj["supports_stateless_prompt"] = true;
    obj["supports_session_context"] = false;
    obj["supports_streaming"] = false;
    obj["supports_tool_calls"] = false;
    obj["supports_json_schema"] = false;
    obj["recommended_board_package"] = "compact_text";
    obj["module_boundary"] = "harness_prompt_bytes_to_stkmdl_raw_response_bytes";
    if (isOpenAiStackModule(normalized)) {
        obj["transport"] = "https";
        obj["provider"] = "openai";
        obj["api_surface"] = "responses";
        obj["requires_env"] = "OPENAI_API_KEY";
    } else if (isOllamaStackModule(normalized)) {
        obj["transport"] = "http_localhost";
        obj["provider"] = "ollama";
        obj["api_surface"] = "generate";
        obj["requires_env"] = "";
    } else {
        obj["transport"] = "unknown";
        obj["provider"] = "unknown";
        obj["api_surface"] = "unknown";
        obj["requires_env"] = "";
    }
    return obj;
}

static QJsonObject ollamaJson(const OllamaResult &result) {
    QJsonObject obj;
    obj["status"] = result.status;
    obj["exit_code"] = result.exitCode;
    obj["elapsed_s"] = result.elapsedSeconds;
    obj["stdout"] = result.stdoutText;
    obj["stderr"] = result.stderrText;
    obj["backend_done"] = result.backendDone;
    obj["done_reason"] = result.doneReason;
    return obj;
}

static QJsonObject runOneMove(const QString &model, int moveTimeoutSeconds, int stackTimeoutSeconds, int numPredict) {
    StockfishReferee referee;
    QJsonObject obj;
    obj["test_id"] = kTestId;
    obj["mode"] = "one_move";
    obj["model"] = model;
    obj["move_timeout_s"] = moveTimeoutSeconds;
    obj["stack_timeout_s"] = stackTimeoutSeconds;
    obj["num_predict"] = numPredict;
    if (!referee.start()) {
        obj["termination"] = "stockfish_start_failed";
        return obj;
    }
    const QString fen = referee.initialFen();
    const QStringList legal = referee.legalMovesForFen(fen);
    const QString movePrompt = promptForMove(fen, legal, 1);
    const OllamaResult response = askOllama(model, movePrompt, stackTimeoutSeconds, numPredict);
    const MoveParseResult parse = parseViaHarness("one_move", "agent", "harness", movePrompt, response.stdoutText, legal);
    const QString parsed = parse.selected;
    const bool legalMove = legal.contains(parsed);
    obj["fen_before"] = fen;
    obj["response"] = ollamaJson(response);
    obj["move_prompt"] = movePrompt;
    obj["move_parse"] = moveParseJson(parse, "move", movePrompt);
    obj["parsed_uci"] = parsed;
    obj["legal"] = legalMove;
    obj["termination"] = response.status == "completed" ? "completed" : response.status;
    referee.stop();
    return obj;
}

static QJsonObject runGame(const QString &model,
                           int moveTimeoutSeconds,
                           int stackTimeoutSeconds,
                           int numPredict,
                           int gameTimeoutSeconds,
                           int maxPlies,
                           int maxIllegal) {
    StockfishReferee referee;
    QJsonObject obj;
    obj["test_id"] = kTestId;
    obj["mode"] = "game";
    obj["model"] = model;
    obj["move_timeout_s"] = moveTimeoutSeconds;
    obj["stack_timeout_s"] = stackTimeoutSeconds;
    obj["num_predict"] = numPredict;
    obj["game_timeout_s"] = gameTimeoutSeconds;
    obj["max_plies"] = maxPlies;
    obj["max_illegal"] = maxIllegal;
    if (!referee.start()) {
        obj["termination"] = "stockfish_start_failed";
        return obj;
    }

    QString brdst = referee.initialFen();
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
        const QString fen = brdst;
        const QStringList legal = referee.legalMovesForFen(brdst);
        if (legal.isEmpty()) {
            termination = "game_completed";
            break;
        }

        const int remainingSeconds = qMax(1, gameTimeoutSeconds - int(elapsed));
        const int effectiveStackTimeout = qMin(stackTimeoutSeconds, remainingSeconds);
        const QString movePrompt = promptForMove(fen, legal, ply);
        const OllamaResult response = askOllama(model, movePrompt, effectiveStackTimeout, numPredict);
        const MoveParseResult parse = parseViaHarness(QString("game P%1").arg(ply, 3, 10, QChar('0')),
                                                      "agent",
                                                      "harness",
                                                      movePrompt,
                                                      response.stdoutText,
                                                      legal);
        const QString parsed = parse.selected;
        const bool legalMove = legal.contains(parsed);

        QJsonObject event;
        event["ply"] = ply;
        event["fen_before"] = fen;
        event["move_prompt"] = movePrompt;
        event["move_parse"] = moveParseJson(parse, "move", movePrompt);
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
            brdst = referee.fenAfterMove(brdst, parsed);
            event["fen_after"] = brdst;
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
    if (referee.legalMovesForFen(brdst).isEmpty()) {
        termination = "game_completed";
    }

    obj["termination"] = termination;
    obj["completed_game"] = termination == "game_completed";
    obj["plies_played"] = moves.size();
    obj["events_recorded"] = events.size();
    obj["legal_moves_played"] = moves.size();
    obj["illegal_or_unparseable_count"] = illegalCount;
    obj["total_elapsed_s"] = gameTimer.elapsed() / 1000.0;
    obj["brdst"] = brdst;
    obj["final_fen"] = brdst;
    obj["mvhst"] = moves.join(' ');
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

static QJsonObject runAgentOnlyBoardGame(int boardIndex,
                                         const QString &whiteModel,
                                         const QString &blackModel,
                                         const QString &requestedWhiteModel,
                                         const QString &requestedBlackModel,
                                         const QStringList &refereeModels,
                                         const QStringList &requestedRefereeModels,
                                         int moveTimeoutSeconds,
                                         int stackTimeoutSeconds,
                                         int numPredict,
                                         int gameTimeoutSeconds,
                                         int maxPlies,
                                         int correctionRetries) {
    QJsonObject obj;
    const QString boardId = QString("board_%1").arg(boardIndex + 1);
    const QString boardLabel = "b" + QString("%1").arg(boardIndex + 1, 3, 10, QChar('0'));
    const QString refereeModel = selectModelForBoard(refereeModels, 0);
    const QString requestedRefereeModel = selectModelForBoard(requestedRefereeModels, 0);
    obj["test_id"] = kTestId;
    obj["mode"] = "aichess_agent_only";
    obj["validation_mode"] = "ans";
    obj["stockfish_board_validation"] = false;
    obj["stockfish_move_validation"] = false;
    obj["brdid"] = boardId;
    obj["white_model"] = whiteModel;
    obj["black_model"] = blackModel;
    obj["referee_model"] = refereeModel;
    obj["requested_white_model"] = requestedWhiteModel;
    obj["requested_black_model"] = requestedBlackModel;
    obj["requested_referee_model"] = requestedRefereeModel;
    obj["move_timeout_s"] = moveTimeoutSeconds;
    obj["stack_timeout_s"] = stackTimeoutSeconds;
    obj["num_predict"] = numPredict;
    obj["game_timeout_s"] = gameTimeoutSeconds;
    obj["max_plies"] = maxPlies;
    obj["correction_rejection_limit"] = correctionRetries;

    const QString roleWhite = "w=" + requestedWhiteModel;
    const QString roleBlack = "b=" + requestedBlackModel;
    const QString roleReferee = "r=" + requestedRefereeModel;
    const QString modelWhite = "w=" + whiteModel;
    const QString modelBlack = "b=" + blackModel;
    const QString modelReferee = "r=" + refereeModel;
    const int whiteColumnWidth = modelWhite.size() + 2;
    const int blackColumnWidth = modelBlack.size() + 2;
    writeLog(3, QString("AIChess board %1\nroles:  %2%3%4\nmodels: %5%6%7\nvalidation: ans stockfish=none\n")
        .arg(boardId,
             roleWhite.leftJustified(whiteColumnWidth, ' '),
             roleBlack.leftJustified(blackColumnWidth, ' '),
             roleReferee,
             modelWhite.leftJustified(whiteColumnWidth, ' '),
             modelBlack.leftJustified(blackColumnWidth, ' '),
             modelReferee));

    QStringList moves;
    QJsonArray events;
    QElapsedTimer gameTimer;
    gameTimer.start();
    int illegalMoveTotal = 0;
    int invalidMoveTotal = 0;
    int refereeInvalidTotal = 0;
    QString termination = "draw_by_configured_ply_limit";
    QString gameResult = "draw";

    for (int ply = 1; ply <= maxPlies; ++ply) {
        if (gameTimer.elapsed() >= qint64(gameTimeoutSeconds) * 1000) {
            termination = "game_timeout";
            break;
        }
        const bool whiteToMove = (ply % 2) == 1;
        const QString side = whiteToMove ? "white" : "black";
        const QString activeModel = whiteToMove ? whiteModel : blackModel;
        const QString plyPrefix = QString("%1: P%2 %3")
            .arg(boardLabel)
            .arg(ply, 3, 10, QChar('0'))
            .arg(whiteToMove ? "w" : "b");

        QElapsedTimer moveTimer;
        moveTimer.start();
        const QString movePrompt = promptForAgentOnlyMove(moves, side, ply);
        QString finalPrompt = movePrompt;
        OllamaResult response = askOllama(activeModel, movePrompt, stackTimeoutSeconds, numPredict,
                                          ply == 1 ? boardLabel : QString());
        MoveParseResult parse = response.status == "completed"
            ? parseViaHarness(QString("%1 P%2 move").arg(boardLabel).arg(ply, 3, 10, QChar('0')),
                              "agent",
                              "harness",
                              movePrompt,
                              response.stdoutText)
            : MoveParseResult();
        const MoveParseResult originalParse = parse;
        QString parsed = parse.selected;
        bool accepted = false;
        QJsonArray correctionAttempts;
        int correctionsUsed = 0;

        for (;;) {
            if (response.status == "timed_out" || gameTimer.elapsed() >= qint64(gameTimeoutSeconds) * 1000) {
                if (gameTimer.elapsed() >= qint64(gameTimeoutSeconds) * 1000) {
                    response.status = "game_timeout";
                }
                break;
            }
            if (response.status != "completed") {
                break;
            }
            if (parsed.isEmpty()) {
                countRejectedMove(parsed, false, &illegalMoveTotal, &invalidMoveTotal);
            } else {
                OllamaResult refResponse = askOllama(refereeModel, promptForAgentOnlyReferee(moves, side, parsed, ply), stackTimeoutSeconds, numPredict);
                bool refereeParsed = false;
                accepted = parseRefereeLegal(refResponse.stdoutText, &refereeParsed);
                if (!refereeParsed) {
                    refereeInvalidTotal += 1;
                    invalidMoveTotal += 1;
                } else if (!accepted) {
                    countRejectedMove(parsed, false, &illegalMoveTotal, &invalidMoveTotal);
                }
                if (accepted) {
                    break;
                }
            }

            const QString displayMove = parsed.isEmpty() ? QString("-----") : parsed.leftJustified(5, ' ');
            const QString resultFlag = parsed.isEmpty() ? QString("-iv") : QString("-il");
            const QString attemptCounts = errorCountText(illegalMoveTotal, invalidMoveTotal);
            writeLog(3, QString("%1:%2 %3 DT:%4 ET:%5%6\n%7: P%8 raw=\"%9\"\n")
                .arg(plyPrefix,
                     displayMove,
                     resultFlag,
                     formatElapsed(response.elapsedSeconds),
                     formatElapsed(gameTimer.elapsed() / 1000.0),
                     attemptCounts,
                     boardLabel)
                .arg(ply, 3, 10, QChar('0'))
                .arg(rawResponseLine(response.stdoutText)));
            if (illegalMoveTotal + invalidMoveTotal >= correctionRetries) {
                break;
            }
            correctionsUsed += 1;
            const QString rejectedBeforeCorrection = parsed;
            const QString correctionPrompt = promptForAgentOnlyCorrection(moves, side, parsed);
            finalPrompt = correctionPrompt;
            response = askOllama(activeModel, correctionPrompt, stackTimeoutSeconds, numPredict);
            parse = response.status == "completed"
                ? parseViaHarness(QString("%1 P%2 correction").arg(boardLabel).arg(ply, 3, 10, QChar('0')),
                                  "agent",
                                  "harness",
                                  correctionPrompt,
                                  response.stdoutText,
                                  QStringList(),
                                  rejectedBeforeCorrection)
                : MoveParseResult();
            parsed = parse.selected;
            QJsonObject correction;
            correction["retry"] = correctionsUsed;
            correction["correction_prompt"] = correctionPrompt;
            correction["response"] = ollamaJson(response);
            correction["move_parse"] = moveParseJson(parse, "agent_only_correction", correctionPrompt, rejectedBeforeCorrection);
            correction["parsed_uci"] = parsed;
            correctionAttempts.append(correction);
        }

        const QString counts = errorCountText(illegalMoveTotal, invalidMoveTotal);
        const QString dtText = formatElapsed(moveTimer.elapsed() / 1000.0);
        const QString etText = formatElapsed(gameTimer.elapsed() / 1000.0);
        QJsonObject event;
        event["ply"] = ply;
        event["brdid"] = boardId;
        event["side_to_move"] = side;
        event["model"] = activeModel;
        event["move_prompt"] = movePrompt;
        event["original_move_parse"] = moveParseJson(originalParse, "agent_only_move", movePrompt);
        event["response"] = ollamaJson(response);
        event["final_move_parse"] = moveParseJson(parse, "agent_only_final", finalPrompt);
        event["parsed_uci"] = parsed;
        event["accepted_by_agent_referee"] = accepted;
        event["illegal_move_total"] = illegalMoveTotal;
        event["invalid_move_total"] = invalidMoveTotal;
        event["rejected_move_total"] = illegalMoveTotal + invalidMoveTotal;
        event["referee_invalid_total"] = refereeInvalidTotal;
        event["correction_attempts"] = correctionAttempts;
        events.append(event);

        if (response.status == "timed_out" || response.status == "game_timeout") {
            writeLog(2, "mv: \"timeout\"\n");
            writeLog(3, QString("%1:TIMEO -f  DT:%2 ET:%3%4\n").arg(plyPrefix, dtText, etText, counts));
            termination = response.status == "game_timeout" ? "game_timeout" : side + "_forfeit_move_timeout";
            gameResult = whiteToMove ? "black_win" : "white_win";
            break;
        }
        if (response.status != "completed") {
            writeLog(2, QString("mv: \"request_failed %1\"\n").arg(rawResponseLine(response.status)));
            writeLog(3, QString("%1:REQF  -f  DT:%2 ET:%3%4 status=%5\n")
                .arg(plyPrefix, dtText, etText, counts, response.status));
            termination = side + "_forfeit_transport_failure";
            gameResult = whiteToMove ? "black_win" : "white_win";
            break;
        }
        if (accepted) {
            moves << parsed;
            writeLog(2, QString("mv: \"%1\"\nrf: \"legal\"\n").arg(rawResponseLine(parsed)));
            writeLog(3, QString("%1:%2 -a  DT:%3 ET:%4%5\n")
                .arg(plyPrefix, parsed.leftJustified(5, ' '), dtText, etText, counts));
            continue;
        }

        writeLog(2, QString("mv: \"%1\"\nrf: \"%2\"\n")
            .arg(rawResponseLine(parsed.isEmpty() ? QString("none") : parsed))
            .arg(parsed.isEmpty() ? QString("unparseable") : QString("illegal")));
        termination = side + "_forfeit_invalid_or_unparseable_move";
        gameResult = whiteToMove ? "black_win" : "white_win";
        break;
    }

    obj["termination"] = termination;
    obj["game_result"] = gameResult;
    obj["terminal_state_reached"] = true;
    obj["completed_game"] = false;
    obj["plies_played"] = moves.size();
    obj["legal_moves_played"] = moves.size();
    obj["illegal_move_total"] = illegalMoveTotal;
    obj["invalid_move_total"] = invalidMoveTotal;
    obj["rejected_move_total"] = illegalMoveTotal + invalidMoveTotal;
    obj["referee_invalid_total"] = refereeInvalidTotal;
    obj["total_elapsed_s"] = gameTimer.elapsed() / 1000.0;
    obj["mvhst"] = moves.join(' ');
    obj["events"] = events;
    writeLog(3, QString("AIChess board %1 finished: result=%2, termination=%3, plies=%4\n%5\n")
        .arg(boardId, gameResult, termination)
        .arg(moves.size())
        .arg(finalCountText(illegalMoveTotal, invalidMoveTotal, correctionRetries)));
    return obj;
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
                                int stackTimeoutSeconds,
                                int numPredict,
                                int gameTimeoutSeconds,
                                int maxPlies,
                                int maxIllegal,
                                int correctionRetries,
                                int lookaheadLevel,
                                bool autoOutputTokens,
                                bool boardAwarenessProbe,
                                bool includeLegalMoveList,
                                const QString &referenceConfigId,
                                const QString &requestedStackModule,
                                const QString &stackKind,
                                const QString &stackName) {
    StockfishReferee referee;
    QJsonObject obj;
    const QString boardId = QString("board_%1").arg(boardIndex + 1);
    obj["test_id"] = kTestId;
    obj["mode"] = "aichess_hallucination_game";
    obj["reference_config_id"] = referenceConfigId;
    obj["rqstkmdl"] = requestedStackModule;
    obj["stktyp"] = stackKind;
    obj["stknm"] = stackName;
    obj["hrnrol"] = "controlling_reference";
    obj["brdid"] = boardId;
    obj["white_model"] = whiteModel;
    obj["black_model"] = blackModel;
    obj["requested_white_model"] = requestedWhiteModel;
    obj["requested_black_model"] = requestedBlackModel;
    obj["model"] = whiteModel + " vs " + blackModel;
    obj["role_assignment"] = QString("white=%1 resolved=%2; black=%3 resolved=%4")
        .arg(requestedWhiteModel, whiteModel, requestedBlackModel, blackModel);
    obj["ground_truth_rules_engine"] = "harness_dtrm_chess_v1";
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
    const bool harnessOnlyRefereeForRun = refereeModels.size() == 1 && refereeModels.first() == "harness";
    obj["referee_mode"] = harnessOnlyRefereeForRun
        ? "harness_dtrm_chess_v1"
        : "agentic_referee_vote";
    QJsonObject agentConfiguration;
    agentConfiguration["brdid"] = boardId;
    agentConfiguration["rqstkmdl"] = requestedStackModule;
    agentConfiguration["stktyp"] = stackKind;
    agentConfiguration["stknm"] = stackName;
    agentConfiguration["hrnrol"] = "controlling_reference";
    agentConfiguration["white_role"] = QString("%1_white_agent_1").arg(boardId);
    agentConfiguration["white_requested"] = requestedWhiteModel;
    agentConfiguration["white_model"] = whiteModel;
    agentConfiguration["white_stkmdl"] = stackModuleForAgentModel(requestedStackModule, whiteModel);
    agentConfiguration["white_stkmdl_caps"] = stackModuleCapabilities(agentConfiguration.value("white_stkmdl").toString());
    agentConfiguration["black_role"] = QString("%1_black_agent_1").arg(boardId);
    agentConfiguration["black_requested"] = requestedBlackModel;
    agentConfiguration["black_model"] = blackModel;
    agentConfiguration["black_stkmdl"] = stackModuleForAgentModel(requestedStackModule, blackModel);
    agentConfiguration["black_stkmdl_caps"] = stackModuleCapabilities(agentConfiguration.value("black_stkmdl").toString());
    agentConfiguration["referee_mode"] = obj["referee_mode"];
    QJsonArray refereeAssignments;
    const int assignmentRefereeCount = harnessOnlyRefereeForRun ? 1 : refereeCount;
    for (int ref = 1; ref <= assignmentRefereeCount; ++ref) {
        QJsonObject assignment;
        assignment["role"] = QString("%1_referee_%2").arg(boardId).arg(ref);
        assignment["requested"] = selectModelForBoard(requestedRefereeModels, ref - 1);
        assignment["model"] = selectModelForBoard(refereeModels, ref - 1);
        assignment["agentic"] = !harnessOnlyRefereeForRun;
        if (assignment["agentic"].toBool()) {
            const QString refereeStackModule = stackModuleForAgentModel(requestedStackModule, assignment.value("model").toString());
            assignment["stkmdl"] = refereeStackModule;
            assignment["stkmdl_caps"] = stackModuleCapabilities(refereeStackModule);
        } else {
            assignment["stkmdl"] = "harness_rules_reference";
        }
        refereeAssignments.append(assignment);
    }
    agentConfiguration["referees"] = refereeAssignments;
    obj["agent_configuration"] = agentConfiguration;
    obj["move_timeout_s"] = moveTimeoutSeconds;
    obj["stack_timeout_s"] = stackTimeoutSeconds;
    obj["num_predict"] = numPredict;
    obj["auto_output_tokens"] = autoOutputTokens;
    obj["board_awareness_probe"] = boardAwarenessProbe;
    obj["game_timeout_s"] = gameTimeoutSeconds;
    obj["max_plies"] = maxPlies;
    obj["max_illegal"] = maxIllegal;
    obj["correction_retries"] = correctionRetries;
    obj["player_prompt_includes_legal_move_list"] = includeLegalMoveList;
    obj["lkahdlvl"] = lookaheadLevel;
    obj["hallucination_test"] = true;
    DetBoardState brdst = initialDetBoardState();
    QString brdfen = detBoardFen(brdst);
    QStringList moves;
    QJsonArray events;
    int illegalCount = 0;
    int illegalMoveTotal = 0;
    int invalidMoveTotal = 0;
    int preMoveAwarenessPass = 0;
    int preMoveAwarenessFail = 0;
    int preMoveAwarenessSkipped = 0;
    int postMoveAwarenessPass = 0;
    int postMoveAwarenessFail = 0;
    int postMoveAwarenessSkipped = 0;
    int currentOutputTokens = numPredict;
    QString termination = "unknown";
    QString gameResult = "draw";
    QElapsedTimer gameTimer;
    gameTimer.start();
    const QString requestedRefereeText = requestedRefereeModels.join(":");
    const QString resolvedRefereeText = refereeModels.join(":");
    const QString roleWhite = "w=" + requestedWhiteModel;
    const QString roleBlack = "b=" + requestedBlackModel;
    const QString roleReferee = "r=" + requestedRefereeText;
    const QString modelWhite = "w=" + whiteModel;
    const QString modelBlack = "b=" + blackModel;
    const QString modelReferee = "r=" + resolvedRefereeText;
    const int whiteColumnWidth = modelWhite.size() + 2;
    const int blackColumnWidth = modelBlack.size() + 2;
    writeLog(3, QString("AIChess board %1\nroles:  %2%3%4\nmodels: %5%6%7\n")
        .arg(boardId,
             roleWhite.leftJustified(whiteColumnWidth, ' '),
             roleBlack.leftJustified(blackColumnWidth, ' '),
             roleReferee,
             modelWhite.leftJustified(whiteColumnWidth, ' '),
             modelBlack.leftJustified(blackColumnWidth, ' '),
             modelReferee));

    for (int ply = 1; ply <= maxPlies; ++ply) {
        const double elapsed = gameTimer.elapsed() / 1000.0;
        if (elapsed >= gameTimeoutSeconds) {
            termination = "game_timeout";
            gameResult = "draw";
            break;
        }

        const QString fen = brdfen;
        const QStringList legal = legalDetMoves(brdst);
        const bool whiteToMove = brdst.side == "W";
        const QString side = whiteToMove ? "white" : "black";
        const QString activeModel = whiteToMove ? whiteModel : blackModel;
        const QString activeStackModule = stackModuleForAgentModel(requestedStackModule, activeModel);
        const QString role = QString("%1_%2_agent_1").arg(boardId, side);
        const QString boardLabel = "b" + QString("%1").arg(boardId.section('_', -1).toInt(), 3, 10, QChar('0'));

        if (legal.isEmpty()) {
            const bool inCheck = detKingInCheck(brdst, brdst.side);
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
        QElapsedTimer evaluationTimer;
        evaluationTimer.start();
        const bool firstWhiteMove = whiteToMove && ply == 1;
        const int dummyMoveCalls = firstWhiteMove ? boardIndex : 0;
        const int maxFirstWhiteAttempts = firstWhiteMove ? qMax(1, maxIllegal + 1) : 1;
        const QString movePrompt = promptForMove(fen, legal, ply, lookaheadLevel, includeLegalMoveList);
        QString finalPrompt = movePrompt;
        OllamaResult response;
        QString parsed;
        MoveParseResult parse;
        QJsonObject preMoveAwarenessObj;
        QJsonObject postMoveAwarenessObj;
        bool legalMove = false;
        int dummyMoveCallsAttempted = 0;
        int moveCallsAttempted = 0;
        bool stoppedBeforeJudgedMove = false;
        if (boardAwarenessProbe) {
            const QString preMoveLabel = QString("%1 P%2 premvbrd").arg(boardLabel).arg(ply, 3, 10, QChar('0'));
            const QString preMovePrompt = promptForPreMoveBoardAwareness(fen, legal, ply);
            const int preMoveRemainingSeconds = qMax(1, gameTimeoutSeconds - int(gameTimer.elapsed() / 1000.0));
            OllamaResult preMoveResponse = askStackModule(
                activeStackModule,
                activeModel,
                preMovePrompt,
                qMin(stackTimeoutSeconds, preMoveRemainingSeconds),
                currentOutputTokens);
            logAgentOutput(preMoveLabel, preMoveResponse);
            preMoveAwarenessObj["prompt"] = preMovePrompt;
            if (preMoveResponse.status == "completed") {
                const BoardAwarenessResult preMoveAwareness = evaluateBoardAwareness(fen, preMoveResponse.stdoutText);
                preMoveAwarenessObj = boardAwarenessJson(preMoveAwareness, preMoveResponse);
                preMoveAwarenessObj["prompt"] = preMovePrompt;
                if (preMoveAwareness.fenMatches && preMoveAwareness.sideMatches && preMoveAwareness.occupiedMatches) {
                    preMoveAwarenessPass += 1;
                } else {
                    preMoveAwarenessFail += 1;
                }
                writeLog(4, QString("%1 current_board fen=%2 side=%3 occupied=%4\n")
                    .arg(preMoveLabel)
                    .arg(preMoveAwareness.fenMatches ? "pass" : "fail")
                    .arg(preMoveAwareness.sideMatches ? "pass" : "fail")
                    .arg(preMoveAwareness.occupiedMatches ? "pass" : "fail"));
            } else {
                preMoveAwarenessObj["response"] = ollamaJson(preMoveResponse);
                preMoveAwarenessObj["skipped_deterministic_compare"] = "pre_move_response_not_completed";
                preMoveAwarenessSkipped += 1;
            }
        }
        for (int call = 1; call <= dummyMoveCalls; ++call) {
            const int callRemainingSeconds = qMax(1, gameTimeoutSeconds - int(gameTimer.elapsed() / 1000.0));
            const int callTimeout = qMin(stackTimeoutSeconds, callRemainingSeconds);
            const QString startupLabel = call == 1 ? boardLabel : QString();
            response = askStackModule(
                activeStackModule,
                activeModel,
                movePrompt,
                callTimeout,
                currentOutputTokens,
                startupLabel);
            dummyMoveCallsAttempted = call;
            if (response.status == "timed_out" || gameTimer.elapsed() >= qint64(gameTimeoutSeconds) * 1000) {
                stoppedBeforeJudgedMove = true;
                break;
            }
        }
        for (int call = 1; !stoppedBeforeJudgedMove && call <= maxFirstWhiteAttempts; ++call) {
            const int callRemainingSeconds = qMax(1, gameTimeoutSeconds - int(gameTimer.elapsed() / 1000.0));
            const int callTimeout = qMin(stackTimeoutSeconds, callRemainingSeconds);
            const QString startupLabel = firstWhiteMove && dummyMoveCalls == 0 && call == 1 ? boardLabel : QString();
            response = askStackModule(
                activeStackModule,
                activeModel,
                movePrompt,
                callTimeout,
                currentOutputTokens,
                startupLabel);
            moveCallsAttempted = call;
            if (response.status == "timed_out" || gameTimer.elapsed() >= qint64(gameTimeoutSeconds) * 1000) {
                break;
            }
            const QString moveLabel = QString("%1 P%2 move").arg(boardLabel).arg(ply, 3, 10, QChar('0'));
            logAgentOutput(moveLabel, response);
            parse = parseViaHarness(moveLabel, "agent", "harness", movePrompt, response.stdoutText, legal);
            parsed = parse.selected;
            const QString transitionMove = moveFromReportedFenTransition(brdst,
                                                                          legal,
                                                                          fieldValue(response.stdoutText, "bf"),
                                                                          fieldValue(response.stdoutText, "af"),
                                                                          nullptr);
            if (!transitionMove.isEmpty()) {
                parsed = transitionMove;
            }
            legalMove = !transitionMove.isEmpty() && legal.contains(parsed);
            const QString moveTuneLabel = QString("%1 P%2 move").arg(boardLabel).arg(ply, 3, 10, QChar('0'));
            const int tunedOutputTokens = autoOutputTokens
                ? tuneOutputTokens(moveTuneLabel,
                                   currentOutputTokens,
                                   response,
                                   parse)
                : currentOutputTokens;
            if (shouldRetryWithTunedOutputTokens(currentOutputTokens, tunedOutputTokens)) {
                currentOutputTokens = tunedOutputTokens;
                const int retryRemainingSeconds = qMax(1, gameTimeoutSeconds - int(gameTimer.elapsed() / 1000.0));
                const int retryTimeout = qMin(stackTimeoutSeconds, retryRemainingSeconds);
                writeLog(3, QString("%1 output_token_retry prompt=move output_tokens=%2\n")
                    .arg(moveTuneLabel)
                    .arg(currentOutputTokens));
                response = askStackModule(
                    activeStackModule,
                    activeModel,
                    movePrompt,
                    retryTimeout,
                    currentOutputTokens);
                if (response.status != "timed_out" && gameTimer.elapsed() < qint64(gameTimeoutSeconds) * 1000) {
                    logAgentOutput(moveTuneLabel + " retry", response);
                    parse = parseViaHarness(moveTuneLabel + " retry", "agent", "harness", movePrompt, response.stdoutText, legal);
                    parsed = parse.selected;
                    const QString retryTransitionMove = moveFromReportedFenTransition(brdst,
                                                                                      legal,
                                                                                      fieldValue(response.stdoutText, "bf"),
                                                                                      fieldValue(response.stdoutText, "af"),
                                                                                      nullptr);
                    if (!retryTransitionMove.isEmpty()) {
                        parsed = retryTransitionMove;
                    }
                    legalMove = !retryTransitionMove.isEmpty() && legal.contains(parsed);
                }
            } else {
                currentOutputTokens = tunedOutputTokens;
            }
            if (!firstWhiteMove || legalMove) {
                break;
            }
        }
        const bool missedGameDeadline = gameTimer.elapsed() >= qint64(gameTimeoutSeconds) * 1000;
        if (missedGameDeadline) {
            response.status = "game_timeout";
            response.stderrText = "response discarded because game timeout elapsed";
        }
        bool moveTimedOut = response.status == "timed_out" || response.status == "game_timeout";
        bool moveRequestFailed = response.status != "completed" && !moveTimedOut;
        if (moveTimedOut) {
            parsed.clear();
            legalMove = false;
        } else if (moveRequestFailed) {
            parsed.clear();
            legalMove = false;
        }
        if (boardAwarenessProbe && !moveTimedOut && !moveRequestFailed && !parsed.isEmpty()) {
            const QString postMoveLabel = QString("%1 P%2 postboard").arg(boardLabel).arg(ply, 3, 10, QChar('0'));
            const QString postMovePrompt = promptForPostMoveBoardAwareness(movePrompt, response.stdoutText, parsed, ply);
            OllamaResult postMoveResponse = askStackModule(
                activeStackModule,
                activeModel,
                postMovePrompt,
                qMin(stackTimeoutSeconds, qMax(1, gameTimeoutSeconds - int(gameTimer.elapsed() / 1000.0))),
                currentOutputTokens);
            logAgentOutput(postMoveLabel, postMoveResponse);
            if (legalMove && postMoveResponse.status == "completed") {
                DetBoardState expectedAfter = brdst;
                applyDetBoardMove(&expectedAfter, parsed);
                const QString expectedAfterFen = detBoardFen(expectedAfter);
                const BoardAwarenessResult postMoveAwareness = evaluateBoardAwareness(expectedAfterFen, postMoveResponse.stdoutText);
                postMoveAwarenessObj = boardAwarenessJson(postMoveAwareness, postMoveResponse);
                postMoveAwarenessObj["prompt"] = postMovePrompt;
                if (postMoveAwareness.fenMatches && postMoveAwareness.sideMatches && postMoveAwareness.occupiedMatches) {
                    postMoveAwarenessPass += 1;
                } else {
                    postMoveAwarenessFail += 1;
                }
                writeLog(4, QString("%1 board_after_move fen=%2 side=%3 occupied=%4\n")
                    .arg(postMoveLabel)
                    .arg(postMoveAwareness.fenMatches ? "pass" : "fail")
                    .arg(postMoveAwareness.sideMatches ? "pass" : "fail")
                    .arg(postMoveAwareness.occupiedMatches ? "pass" : "fail"));
            } else {
                postMoveAwarenessObj["prompt"] = postMovePrompt;
                postMoveAwarenessObj["response"] = ollamaJson(postMoveResponse);
                postMoveAwarenessObj["skipped_deterministic_compare"] = legalMove
                    ? "post_move_response_not_completed"
                    : "candidate_move_not_legal";
                postMoveAwarenessSkipped += 1;
            }
        } else if (boardAwarenessProbe) {
            if (moveTimedOut) {
                postMoveAwarenessObj["skipped_deterministic_compare"] = "move_response_timed_out";
            } else if (moveRequestFailed) {
                postMoveAwarenessObj["skipped_deterministic_compare"] = "move_request_failed";
            } else {
                postMoveAwarenessObj["skipped_deterministic_compare"] = parsed.isEmpty()
                    ? "no_candidate_move"
                    : "candidate_move_missing";
            }
            postMoveAwarenessSkipped += 1;
        }
        const OllamaResult originalResponse = response;
        const QString originalParsed = parsed;
        const MoveParseResult originalParse = parse;
        const QJsonObject originalPostMoveAwarenessObj = postMoveAwarenessObj;
        const bool originalLegalMove = legalMove;
        const double originalEtSeconds = gameTimer.elapsed() / 1000.0;
        if (!moveTimedOut && !moveRequestFailed && !originalLegalMove) {
            countRejectedMove(originalParsed, originalLegalMove, &illegalMoveTotal, &invalidMoveTotal);
        }
        const int originalIllegalMoveTotal = illegalMoveTotal;
        const int originalInvalidMoveTotal = invalidMoveTotal;
        QJsonArray correctionAttempts;
        int correctionAttemptsUsed = 0;
        for (int retry = 1;
             !moveTimedOut && !moveRequestFailed && !legalMove && (illegalMoveTotal + invalidMoveTotal) < correctionRetries;
             ++retry) {
            const int callRemainingSeconds = qMax(1, gameTimeoutSeconds - int(gameTimer.elapsed() / 1000.0));
            const int callTimeout = qMin(stackTimeoutSeconds, callRemainingSeconds);
            const QString rejectedBeforeCorrection = parsed;
            const QString correctionPrompt = promptForCorrection(fen, legal, rejectedBeforeCorrection);
            finalPrompt = correctionPrompt;
            OllamaResult correctionResponse = askStackModule(
                activeStackModule,
                activeModel,
                correctionPrompt,
                callTimeout,
                currentOutputTokens);
            correctionAttemptsUsed = retry;
            QString correctionParsed;
            MoveParseResult correctionParse;
            bool correctionLegal = false;
            if (correctionResponse.status != "timed_out" &&
                gameTimer.elapsed() < qint64(gameTimeoutSeconds) * 1000) {
                const QString correctionLabel = QString("%1 P%2 correction").arg(boardLabel).arg(ply, 3, 10, QChar('0'));
                logAgentOutput(correctionLabel, correctionResponse);
                correctionParse = parseViaHarness(correctionLabel,
                                                  "agent",
                                                  "harness",
                                                  correctionPrompt,
                                                  correctionResponse.stdoutText,
                                                  legal,
                                                  rejectedBeforeCorrection);
                correctionParsed = correctionParse.selected;
                const QString correctionTransitionMove = moveFromReportedFenTransition(brdst,
                                                                                       legal,
                                                                                       fieldValue(correctionResponse.stdoutText, "bf"),
                                                                                       fieldValue(correctionResponse.stdoutText, "af"),
                                                                                       nullptr);
                if (!correctionTransitionMove.isEmpty()) {
                    correctionParsed = correctionTransitionMove;
                }
                correctionLegal = !correctionTransitionMove.isEmpty() && legal.contains(correctionParsed);
                const QString correctionTuneLabel = QString("%1 P%2 correction").arg(boardLabel).arg(ply, 3, 10, QChar('0'));
                const int tunedOutputTokens = autoOutputTokens
                    ? tuneOutputTokens(correctionTuneLabel,
                                       currentOutputTokens,
                                       correctionResponse,
                                       correctionParse)
                    : currentOutputTokens;
                if (shouldRetryWithTunedOutputTokens(currentOutputTokens, tunedOutputTokens)) {
                    currentOutputTokens = tunedOutputTokens;
                    const int retryRemainingSeconds = qMax(1, gameTimeoutSeconds - int(gameTimer.elapsed() / 1000.0));
                    const int retryTimeout = qMin(stackTimeoutSeconds, retryRemainingSeconds);
                    writeLog(3, QString("%1 output_token_retry prompt=correction output_tokens=%2\n")
                        .arg(correctionTuneLabel)
                        .arg(currentOutputTokens));
                    correctionResponse = askStackModule(
                        activeStackModule,
                        activeModel,
                        correctionPrompt,
                        retryTimeout,
                        currentOutputTokens);
                    if (correctionResponse.status != "timed_out" &&
                        gameTimer.elapsed() < qint64(gameTimeoutSeconds) * 1000) {
                        logAgentOutput(correctionTuneLabel + " retry", correctionResponse);
                        correctionParse = parseViaHarness(correctionTuneLabel + " retry",
                                                          "agent",
                                                          "harness",
                                                          correctionPrompt,
                                                          correctionResponse.stdoutText,
                                                          legal,
                                                          rejectedBeforeCorrection);
                        correctionParsed = correctionParse.selected;
                        const QString correctionRetryTransitionMove = moveFromReportedFenTransition(brdst,
                                                                                                    legal,
                                                                                                    fieldValue(correctionResponse.stdoutText, "bf"),
                                                                                                    fieldValue(correctionResponse.stdoutText, "af"),
                                                                                                    nullptr);
                        if (!correctionRetryTransitionMove.isEmpty()) {
                            correctionParsed = correctionRetryTransitionMove;
                        }
                        correctionLegal = !correctionRetryTransitionMove.isEmpty() && legal.contains(correctionParsed);
                    }
                } else {
                    currentOutputTokens = tunedOutputTokens;
                }
                if (!correctionLegal) {
                    countRejectedMove(correctionParsed, correctionLegal, &illegalMoveTotal, &invalidMoveTotal);
                }
            } else if (gameTimer.elapsed() >= qint64(gameTimeoutSeconds) * 1000) {
                correctionResponse.status = "game_timeout";
                correctionResponse.stderrText = "response discarded because game timeout elapsed";
            }

            QJsonObject correctionObj;
            correctionObj["retry"] = retry;
            correctionObj["correction_prompt"] = correctionPrompt;
            correctionObj["response"] = ollamaJson(correctionResponse);
            correctionObj["move_parse"] = moveParseJson(correctionParse,
                                                        "correction",
                                                        correctionPrompt,
                                                        rejectedBeforeCorrection);
            correctionObj["parsed_uci"] = correctionParsed;
            correctionObj["legal_by_harness"] = correctionLegal;
            correctionAttempts.append(correctionObj);

            response = correctionResponse;
            parse = correctionParse;
            parsed = correctionParsed;
            legalMove = correctionLegal;
            moveTimedOut = response.status == "timed_out" || response.status == "game_timeout";
            moveRequestFailed = response.status != "completed" && !moveTimedOut;
            if (moveTimedOut || moveRequestFailed || legalMove) {
                break;
            }
        }

        QJsonArray refereeVotes;
        int refereeValidVotes = 0;
        int refereeInvalidVotes = 0;
        int refereeUnparseableVotes = 0;
        const bool harnessOnlyReferee = refereeModels.size() == 1 && refereeModels.first() == "harness";
        const int effectiveRefereeCount = harnessOnlyReferee ? 1 : refereeCount;
        if (moveTimedOut || moveRequestFailed) {
            refereeInvalidVotes = 1;
        } else {
            for (int ref = 1; ref <= effectiveRefereeCount; ++ref) {
                QJsonObject vote;
                vote["role"] = QString("%1_referee_%2").arg(boardId).arg(ref);
                const QString requestedReferee = selectModelForBoard(requestedRefereeModels, ref - 1);
                const QString resolvedReferee = selectModelForBoard(refereeModels, ref - 1);
                vote["requested_model"] = requestedReferee;
                vote["model"] = resolvedReferee;
                vote["harness_ground_truth_legal"] = legalMove;
                vote["move"] = parsed;
                if (harnessOnlyReferee) {
                    vote["stack_id"] = "harness_dtrm_chess_v1";
                    vote["referee_backend"] = "harness_dtrm_chess_v1";
                    vote["legal"] = legalMove;
                    vote["parsed_referee_vote"] = true;
                    vote["agrees_with_harness"] = true;
                    vote["reason"] = legalMove ? "move_in_harness_legal_set" : "missing_or_illegal_harness_move";
                    if (legalMove) {
                        refereeValidVotes += 1;
                    } else {
                        refereeInvalidVotes += 1;
                    }
                } else {
                    const OllamaResult refereeResponse = askStackModule(
                        stackModuleForAgentModel(requestedStackModule, resolvedReferee),
                        resolvedReferee,
                        promptForReferee(fen, legal, parsed, ply),
                        qMin(stackTimeoutSeconds, effectiveMoveTimeout),
                        numPredict);
                    bool parsedRefereeVote = false;
                    const bool agentSaysLegal = parseRefereeLegal(refereeResponse.stdoutText, &parsedRefereeVote);
                    vote["stack_id"] = resolvedReferee;
                    vote["referee_backend"] = "ollama_agent";
                    vote["response"] = ollamaJson(refereeResponse);
                    vote["legal"] = agentSaysLegal;
                    vote["parsed_referee_vote"] = parsedRefereeVote;
                    vote["agrees_with_harness"] = parsedRefereeVote && agentSaysLegal == legalMove;
                    if (!parsedRefereeVote) {
                        refereeUnparseableVotes += 1;
                        refereeInvalidVotes += 1;
                    } else if (agentSaysLegal) {
                        refereeValidVotes += 1;
                    } else {
                        refereeInvalidVotes += 1;
                    }
                    vote["reason"] = parsedRefereeVote
                        ? "agentic_referee_vote_compared_to_harness_ground_truth"
                        : "unparseable_agentic_referee_vote";
                }
                const QString refereeVoteText = vote.value("parsed_referee_vote").toBool()
                    ? (vote.value("legal").toBool() ? "legal" : "illegal")
                    : QString("unparseable");
                if (!harnessOnlyReferee) {
                    writeLog(2, QString("rf: \"%1\"\n").arg(rawResponseLine(refereeVoteText)));
                }
                refereeVotes.append(vote);
            }
        }

        QJsonObject event;
        event["ply"] = ply;
        event["brdid"] = boardId;
        event["side_to_move"] = side;
        event["role"] = role;
        event["model"] = activeModel;
        event["fen_before"] = fen;
        event["brdst_before"] = detBoardJson(brdst);
        event["legal_move_count"] = legal.size();
        event["dummy_move_calls_requested"] = dummyMoveCalls;
        event["dummy_move_calls_attempted"] = dummyMoveCallsAttempted;
        event["move_calls_requested"] = maxFirstWhiteAttempts;
        event["move_calls_attempted"] = moveCallsAttempted;
        event["move_prompt"] = movePrompt;
        event["original_move_parse"] = moveParseJson(originalParse, "move", movePrompt);
        if (boardAwarenessProbe) {
            event["pre_move_board_awareness"] = preMoveAwarenessObj;
            event["post_move_board_awareness"] = originalPostMoveAwarenessObj;
        }
        event["original_response"] = ollamaJson(originalResponse);
        event["original_parsed_uci"] = originalParsed;
        event["original_legal_by_harness"] = originalLegalMove;
        event["correction_rejection_limit"] = correctionRetries;
        event["correction_retries_used"] = correctionAttemptsUsed;
        event["correction_attempts"] = correctionAttempts;
        event["illegal_move_total"] = illegalMoveTotal;
        event["invalid_move_total"] = invalidMoveTotal;
        event["rejected_move_total"] = illegalMoveTotal + invalidMoveTotal;
        event["response"] = ollamaJson(response);
        event["final_move_parse"] = moveParseJson(parse, "final", finalPrompt);
        event["parsed_uci"] = parsed;
        event["transport_failure"] = moveRequestFailed;
        const bool refereeMajorityValid = refereeValidVotes > refereeInvalidVotes;
        event["legal_by_harness"] = legalMove;
        event["legal_by_referee_vote"] = refereeMajorityValid;
        event["legal"] = refereeMajorityValid;
        event["referee_valid_votes"] = refereeValidVotes;
        event["referee_invalid_votes"] = refereeInvalidVotes;
        event["referee_unparseable_votes"] = refereeUnparseableVotes;
        event["referee_vote_rule"] = "majority";
        event["referee_votes"] = refereeVotes;
        event["move_to_referee_elapsed_s"] = evaluationTimer.elapsed() / 1000.0;

        const QString plyPrefix = QString("%1: P%2 %3")
            .arg(boardLabel)
            .arg(ply, 3, 10, QChar('0'))
            .arg(whiteToMove ? "w" : "b");
        const QString dtText = formatElapsed(response.elapsedSeconds);
        const QString etText = formatElapsed(gameTimer.elapsed() / 1000.0);
        const QString countsText = errorCountText(illegalMoveTotal, invalidMoveTotal);
        const QString originalCountsText = errorCountText(originalIllegalMoveTotal, originalInvalidMoveTotal);
        if (correctionAttemptsUsed > 0 && originalResponse.status != "timed_out" && originalResponse.status != "game_timeout") {
            const QString originalMove = originalParsed.isEmpty()
                ? QString("-----")
                : originalParsed.leftJustified(5, ' ');
            const QString originalFlag = originalParsed.isEmpty() ? QString("-iv") : QString("-il");
            QString originalOutput = QString("%1:%2 %3 DT:%4 ET:%5%6")
                .arg(plyPrefix,
                     originalMove,
                     originalFlag,
                     formatElapsed(originalResponse.elapsedSeconds),
                     formatElapsed(originalEtSeconds),
                     originalCountsText);
            if (originalParsed.isEmpty()) {
                originalOutput += " status=" + originalResponse.status;
            }
            originalOutput += "\n";
            originalOutput += QString("%1: P%2 raw=\"%3\"\n")
                .arg(boardLabel)
                .arg(ply, 3, 10, QChar('0'))
                .arg(rawResponseLine(originalResponse.stdoutText));
            writeLog(3, originalOutput);
        }
        if (moveTimedOut) {
            logHarnessRoute(plyPrefix, "log", "move_timeout");
            writeLog(2, "mv: \"timeout\"\nrf: \"invalid\"\n");
            writeLog(3, QString("%1:TIMEO -f  DT:%2 ET:%3%4\n")
                .arg(plyPrefix, dtText, etText, countsText));
        } else if (moveRequestFailed) {
            logHarnessRoute(plyPrefix, "log", "transport_failure");
            writeLog(2, QString("mv: \"request_failed %1\"\nrf: \"invalid\"\n").arg(rawResponseLine(response.status)));
            writeLog(3, QString("%1:REQF  -f  DT:%2 ET:%3%4 status=%5\n%6: P%7 stderr=\"%8\"\n")
                .arg(plyPrefix,
                     dtText,
                     etText,
                     countsText,
                     response.status,
                     boardLabel)
                .arg(ply, 3, 10, QChar('0'))
                .arg(rawResponseLine(response.stderrText)));
        } else {
            QString refereeFlag;
            QString displayMove;
            if (parsed.isEmpty()) {
                displayMove = "-----";
                refereeFlag = "-iv";
            } else if (!legalMove) {
                displayMove = parsed.leftJustified(5, ' ');
                refereeFlag = "-il";
            } else {
                displayMove = parsed.leftJustified(5, ' ');
                refereeFlag = "-a ";
            }
            logHarnessRoute(plyPrefix,
                            legalMove ? "board" : "agent",
                            legalMove ? "legal_move" : (parsed.isEmpty() ? "no_candidate" : "illegal_move"));
            QString output = QString("%1:%2 %3 DT:%4 ET:%5%6")
                .arg(plyPrefix, displayMove, refereeFlag, dtText, etText, countsText);
            if (parsed.isEmpty()) {
                output += " status=" + response.status;
            }
            output += "\n";
            if (parsed.isEmpty() || !legalMove) {
                output += QString("%1: P%2 raw=\"%3\"\n")
                    .arg(boardLabel)
                    .arg(ply, 3, 10, QChar('0'))
                    .arg(rawResponseLine(response.stdoutText));
            }
            writeLog(3, output);
            const QString refereeSummary = refereeMajorityValid && legalMove
                ? QString("legal")
                : (parsed.isEmpty() ? QString("unparseable") : QString("illegal"));
            writeLog(2, QString("mv: \"%1\"\nrf: \"%2\"\n")
                .arg(rawResponseLine(parsed.isEmpty() ? QString("none") : parsed))
                .arg(rawResponseLine(refereeSummary)));
        }
        if (moveTimedOut) {
            event["error"] = response.status == "game_timeout"
                ? "game_timeout_move_discarded"
                : "move_timeout_forfeit";
            events.append(event);
            if (response.status == "game_timeout") {
                termination = "game_timeout";
                gameResult = "draw";
            } else {
                termination = side + "_forfeit_move_timeout";
                gameResult = whiteToMove ? "black_win" : "white_win";
            }
            break;
        }
        if (moveRequestFailed) {
            event["error"] = "move_request_transport_failure";
            events.append(event);
            termination = side + "_forfeit_transport_failure";
            gameResult = whiteToMove ? "black_win" : "white_win";
            break;
        }

        if (refereeMajorityValid && legalMove) {
            moves << parsed;
            event["curmv"] = parsed;
            applyDetBoardMove(&brdst, parsed);
            brdfen = detBoardFen(brdst);
            event["fen_after"] = brdfen;
            event["brdst_after"] = detBoardJson(brdst);
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
    obj["pre_move_board_awareness_pass"] = preMoveAwarenessPass;
    obj["pre_move_board_awareness_fail"] = preMoveAwarenessFail;
    obj["pre_move_board_awareness_skipped"] = preMoveAwarenessSkipped;
    obj["post_move_board_awareness_pass"] = postMoveAwarenessPass;
    obj["post_move_board_awareness_fail"] = postMoveAwarenessFail;
    obj["post_move_board_awareness_skipped"] = postMoveAwarenessSkipped;
    obj["total_elapsed_s"] = gameTimer.elapsed() / 1000.0;
    obj["brdst"] = detBoardJson(brdst);
    obj["brdfen"] = brdfen;
    obj["final_fen"] = brdfen;
    obj["mvhst"] = moves.join(' ');
    obj["events"] = events;
    writeLog(3, QString("AIChess board %1 finished: result=%2, termination=%3, plies=%4\n%5\n")
        .arg(boardId, gameResult, termination)
        .arg(moves.size())
        .arg(finalCountText(illegalMoveTotal, invalidMoveTotal, correctionRetries)));
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

    if (logEnabled(3)) {
        QTextStream(stdout)
            << kOutDir << "\n"
            << QFileInfo(summaryPath).fileName() << "\n"
            << QFileInfo(jsonlPath).fileName() << "\n";
    }
}

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    QCommandLineParser parser;
    parser.setApplicationDescription("AIH AIchess runner");
    parser.addHelpOption();

    QCommandLineOption modeOpt("mode", "aichess, hallucination-game, class-game, one-move, game, or both", "mode", "aichess");
    QCommandLineOption modelOpt("models", "Comma-separated Ollama model names used to resolve qwenN/agentN aliases", "models");
    QCommandLineOption whiteModelsOpt("white-models", "Comma-separated White player models, assigned round-robin by board", "models");
    QCommandLineOption blackModelsOpt("black-models", "Comma-separated Black player models, assigned round-robin by board", "models");
    QCommandLineOption refereeModelsOpt("referee-models", "Comma-separated referee model labels, assigned round-robin by referee role", "models");
    QCommandLineOption whiteOpt(QStringList{"w", "white"}, "White player model(s), e.g. qwen1 or qwen1:qwen4", "models");
    QCommandLineOption blackOpt(QStringList{"b", "black"}, "Black player model(s), e.g. qwen1 or qwen1:qwen4", "models");
    QCommandLineOption refereeOpt(QStringList{"r", "referee"}, "Referee model(s), e.g. harness, qwen1, or qwen1:qwen4", "models");
    QCommandLineOption boardsOpt("boards", "Number of boards to run in AIChess hallucination game mode", "count", "1");
    QCommandLineOption loopsOpt("loops", "Number of board batches to run in AIChess hallucination game mode", "count", "1");
    QCommandLineOption refereeCountOpt("referee-count", "Referee votes recorded per board", "count", "1");
    QCommandLineOption moveTimeoutOpt(QStringList{"t", "move-timeout"}, "Per-move response window seconds", "seconds", "10");
    QCommandLineOption stackTimeoutOpt(QStringList{"sto", "stack-timeout"}, "Agent stack response completion timeout seconds", "seconds", "60");
    QCommandLineOption outputTokensOpt(QStringList{"otkns"}, "Agent output token budget", "tokens", "256");
    QCommandLineOption autoOutputTokensOpt(QStringList{"aot", "auto-output-tokens"}, "Enable automatic output-token tuning");
    QCommandLineOption boardAwarenessProbeOpt(QStringList{"bap", "board-awareness-probe"}, "After each move reply, ask the agent to report the board after its move and score it");
    QCommandLineOption legalListOpt("legal-list", "Assisted mode: include hidden legal UCI move list in the player prompt. Default is board-transition only.");
    QCommandLineOption referenceConfigOpt("reference-config", "Reference configuration identifier for this harness/stack run", "id", "aichess_ref_harness_ollama_agentic_v1_20260716");
    QCommandLineOption stackModuleOpt("stack-module", "Harness stack adapter module selected for agent I/O", "module", "auto");
    QCommandLineOption stackKindOpt("stack-kind", "Stack category interacting with the harness", "kind", "ollama_agentic_local");
    QCommandLineOption stackNameOpt("stack-name", "Human-readable stack name interacting with the harness", "name", "ollama_generate_qwen_local");
    QCommandLineOption gameTimeoutOpt(QStringList{"gmto"}, "Per-game timeout seconds", "seconds", "600");
    QCommandLineOption maxPliesOpt(QStringList{"mxply"}, "Maximum plies per game", "plies", "120");
    QCommandLineOption retriesOpt(QStringList{"cnrtlm"}, "Correction retry limit after illegal/unparseable move", "count", "1");
    QCommandLineOption lookaheadOpt(QStringList{"lkahdlvl", "lokahdlvl"}, "Agent chess look-ahead level requested in the move prompt", "level", "0");
    QCommandLineOption logLevelOpt("loglvl", "Logging verbosity 0-5. 0 is least verbose; 5 is most verbose. Level 1 prints compact model hi/ho records; level 2 prints only harness-to-Ollama ho/hi plus mv and rf strings; level 3 adds diagnostics.", "level", "3");
    QCommandLineOption maxIllegalOpt("max-illegal", "Illegal/unparseable move limit before forfeit", "count", "1");
    QCommandLineOption avbOpt("avb", "Stockfish validates board/rules; referee agent validates moves");
    QCommandLineOption avmOpt("avm", "Stockfish validates moves; referee agent validates board");
    QCommandLineOption ansOpt("ans", "Agent-only validation; no Stockfish when no Stockfish validation flag is set");
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
    parser.addOption(loopsOpt);
    parser.addOption(refereeCountOpt);
    parser.addOption(moveTimeoutOpt);
    parser.addOption(stackTimeoutOpt);
    parser.addOption(outputTokensOpt);
    parser.addOption(autoOutputTokensOpt);
    parser.addOption(boardAwarenessProbeOpt);
    parser.addOption(legalListOpt);
    parser.addOption(referenceConfigOpt);
    parser.addOption(stackModuleOpt);
    parser.addOption(stackKindOpt);
    parser.addOption(stackNameOpt);
    parser.addOption(gameTimeoutOpt);
    parser.addOption(maxPliesOpt);
    parser.addOption(retriesOpt);
    parser.addOption(lookaheadOpt);
    parser.addOption(logLevelOpt);
    parser.addOption(maxIllegalOpt);
    parser.addOption(avbOpt);
    parser.addOption(avmOpt);
    parser.addOption(ansOpt);
    parser.addOption(listModelsOpt);
    parser.addOption(dryRunOpt);
    parser.addPositionalArgument("agents",
        "Optional one to three agents. 1 agent fills White and Black; 2 agents play randomized colors; 3rd agent is referee.");
    for (int i = 1; i < argc; ++i) {
        if (QString::fromLocal8Bit(argv[i]) == "/?") {
            parser.showHelp(0);
        }
    }
    const QStringList cliArgs = normalizeWrapperCliArgs(QCoreApplication::arguments());
    parser.process(cliArgs);

    const QStringList positionalAgents = parser.positionalArguments();
    const bool hasRoleOverride = parser.isSet(whiteOpt) || parser.isSet(blackOpt) || parser.isSet(refereeOpt) ||
        parser.isSet(whiteModelsOpt) || parser.isSet(blackModelsOpt) || parser.isSet(refereeModelsOpt);
    if (!positionalAgents.isEmpty() && hasRoleOverride) {
        QTextStream(stderr) << "Do not mix positional agents with -w/-b/-r role overrides.\n";
        return 2;
    }
    if (positionalAgents.size() > 3) {
        QTextStream(stderr) << "Too many positional agents. Use one, two, or three agents.\n";
        return 2;
    }

    QStringList models;
    if (parser.isSet(modelOpt)) {
        models = parser.value(modelOpt).split(',', Qt::SkipEmptyParts);
    } else {
        models = detectOllamaModelsSortedBySize();
    }
    for (QString &model : models) {
        model = resolveAgentAlias(model, models);
    }

    QStringList whiteModels;
    QStringList blackModels;
    QStringList refereeModels;
    if (!positionalAgents.isEmpty()) {
        if (positionalAgents.size() == 1) {
            whiteModels = {positionalAgents.at(0)};
            blackModels = {positionalAgents.at(0)};
            refereeModels = parser.isSet(ansOpt) ? QStringList{positionalAgents.at(0)} : QStringList{"harness"};
        } else {
            const bool swapColors = QRandomGenerator::global()->bounded(2) == 0;
            whiteModels = {positionalAgents.at(swapColors ? 1 : 0)};
            blackModels = {positionalAgents.at(swapColors ? 0 : 1)};
            refereeModels = parser.isSet(ansOpt) ? QStringList{positionalAgents.at(1)} : QStringList{"harness"};
            if (positionalAgents.size() == 3) {
                refereeModels = {positionalAgents.at(2)};
            }
        }
    } else {
        whiteModels = parser.isSet(whiteOpt)
            ? splitModelSpec(parser.value(whiteOpt))
            : parser.isSet(whiteModelsOpt)
            ? splitModelSpec(parser.value(whiteModelsOpt))
            : hasRoleOverride
            ? QStringList{}
            : QStringList{"qwen1"};
        blackModels = parser.isSet(blackOpt)
            ? splitModelSpec(parser.value(blackOpt))
            : parser.isSet(blackModelsOpt)
            ? splitModelSpec(parser.value(blackModelsOpt))
            : hasRoleOverride
            ? QStringList{}
            : QStringList{"qwen1"};
        refereeModels = parser.isSet(refereeOpt)
            ? splitModelSpec(parser.value(refereeOpt))
            : parser.isSet(refereeModelsOpt)
            ? splitModelSpec(parser.value(refereeModelsOpt))
            : parser.isSet(ansOpt) && !hasRoleOverride
            ? QStringList{"qwen1"}
            : QStringList{"harness"};
    }
    QStringList requestedWhiteModels = whiteModels;
    QStringList requestedBlackModels = blackModels;
    QStringList requestedRefereeModels = refereeModels;
    for (QString &model : whiteModels) {
        model = resolveAgentAlias(model, models);
    }
    for (QString &model : blackModels) {
        model = resolveAgentAlias(model, models);
    }
    for (QString &model : refereeModels) {
        if (model.trimmed() == "harness" || model.trimmed() == "stockfish") {
            model = "harness";
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
    const int stackTimeout = parser.value(stackTimeoutOpt).toInt();
    const int outputTokens = parser.value(outputTokensOpt).toInt();
    const bool autoOutputTokens = parser.isSet(autoOutputTokensOpt);
    const bool boardAwarenessProbe = parser.isSet(boardAwarenessProbeOpt);
    const bool includeLegalMoveList = parser.isSet(legalListOpt);
    const QString referenceConfigId = parser.value(referenceConfigOpt);
    const QString stackModule = parser.value(stackModuleOpt);
    QString stackKind = parser.value(stackKindOpt);
    QString stackName = parser.value(stackNameOpt);
    const int gameTimeout = parser.value(gameTimeoutOpt).toInt();
    const int maxPlies = parser.value(maxPliesOpt).toInt();
    const int correctionRetries = qMax(0, parser.value(retriesOpt).toInt());
    const int lookaheadLevel = qMax(0, parser.value(lookaheadOpt).toInt());
    kLogLevel = qBound(0, parser.value(logLevelOpt).toInt(), 5);
    const int maxIllegal = parser.value(maxIllegalOpt).toInt();
    const int boards = qMax(1, parser.value(boardsOpt).toInt());
    const int loops = qMax(1, parser.value(loopsOpt).toInt());
    const int refereeCount = qMax(1, parser.value(refereeCountOpt).toInt());
    const bool avb = parser.isSet(avbOpt);
    const bool avm = parser.isSet(avmOpt);
    const bool ans = parser.isSet(ansOpt);
    const bool agentOnlyNoStockfish = ans && !avb && !avm;
    if (isOpenAiStackModule(stackModule)) {
        if (!parser.isSet(stackKindOpt)) {
            stackKind = "openai_agentic_cloud";
        }
        if (!parser.isSet(stackNameOpt)) {
            stackName = "openai_responses_cloud";
        }
    } else if (isAutoStackModule(stackModule) && hasAnyOpenAiAgent(whiteModels + blackModels + refereeModels)) {
        if (!parser.isSet(stackKindOpt)) {
            stackKind = "openai_agentic_cloud";
        }
        if (!parser.isSet(stackNameOpt)) {
            stackName = "openai_responses_cloud";
        }
    }
    writeLog(3, QString("AIChess run starting  boards: %1  loops: %2  outputs: %3\n")
        .arg(boards)
        .arg(loops)
        .arg(kOutDir));
    writeLog(3, QString("assignments: w=%1  b=%2  r=%3\n\n")
        .arg(requestedWhiteModels.join(":"))
        .arg(requestedBlackModels.join(":"))
        .arg(requestedRefereeModels.join(":")));

    if (parser.isSet(dryRunOpt)) {
        QJsonObject obj;
        obj["mode"] = mode;
        obj["reference_config_id"] = referenceConfigId;
        obj["rqstkmdl"] = stackModule;
        obj["stktyp"] = stackKind;
        obj["stknm"] = stackName;
        obj["stkmdl_auto"] = isAutoStackModule(stackModule);
        obj["hrnrol"] = "controlling_reference";
        obj["boards"] = boards;
        obj["loops"] = loops;
        obj["referee_count"] = refereeCount;
        obj["validation_mode"] = agentOnlyNoStockfish ? "ans" : "harness_dtrm_chess_v1";
        obj["move_timeout_s"] = moveTimeout;
        obj["stack_timeout_s"] = stackTimeout;
        obj["output_tokens"] = outputTokens;
        obj["auto_output_tokens"] = autoOutputTokens;
        obj["board_awareness_probe"] = boardAwarenessProbe;
        obj["player_prompt_includes_legal_move_list"] = includeLegalMoveList;
        obj["game_timeout_s"] = gameTimeout;
        obj["max_plies"] = maxPlies;
        obj["correction_retries"] = correctionRetries;
        obj["lkahdlvl"] = lookaheadLevel;
        obj["loglvl"] = kLogLevel;
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
            assignment["brdid"] = boardId;
            assignment["white_role"] = QString("%1_white_agent_1").arg(boardId);
            assignment["white_requested"] = selectModelForBoard(requestedWhiteModels, board);
            assignment["white_model"] = selectModelForBoard(whiteModels, board);
            assignment["white_stkmdl"] = stackModuleForAgentModel(stackModule, assignment.value("white_model").toString());
            assignment["white_stkmdl_caps"] = stackModuleCapabilities(assignment.value("white_stkmdl").toString());
            assignment["black_role"] = QString("%1_black_agent_1").arg(boardId);
            assignment["black_requested"] = selectModelForBoard(requestedBlackModels, board);
            assignment["black_model"] = selectModelForBoard(blackModels, board);
            assignment["black_stkmdl"] = stackModuleForAgentModel(stackModule, assignment.value("black_model").toString());
            assignment["black_stkmdl_caps"] = stackModuleCapabilities(assignment.value("black_stkmdl").toString());
            const bool harnessOnly = refereeModels.size() == 1 && refereeModels.first() == "harness";
            assignment["referee_mode"] = harnessOnly
                ? "harness_dtrm_chess_v1"
                : "agentic_referee_vote";
            QJsonArray refereeAssignments;
            const int assignmentRefereeCount = harnessOnly ? 1 : refereeCount;
            for (int ref = 1; ref <= assignmentRefereeCount; ++ref) {
                QJsonObject refereeAssignment;
                refereeAssignment["role"] = QString("%1_referee_%2").arg(boardId).arg(ref);
                refereeAssignment["requested"] = selectModelForBoard(requestedRefereeModels, ref - 1);
                refereeAssignment["model"] = selectModelForBoard(refereeModels, ref - 1);
                refereeAssignment["agentic"] = !harnessOnly;
                if (refereeAssignment["agentic"].toBool()) {
                    const QString refereeStackModule = stackModuleForAgentModel(stackModule, refereeAssignment.value("model").toString());
                    refereeAssignment["stkmdl"] = refereeStackModule;
                    refereeAssignment["stkmdl_caps"] = stackModuleCapabilities(refereeStackModule);
                } else {
                    refereeAssignment["stkmdl"] = "harness_rules_reference";
                }
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
        for (int loop = 0; loop < loops; ++loop) {
            std::vector<std::future<QJsonObject>> boardFutures;
            boardFutures.reserve(boards);
            for (int board = 0; board < boards; ++board) {
                const QString whiteModel = selectModelForBoard(whiteModels, board);
                const QString blackModel = selectModelForBoard(blackModels, board);
                const QString requestedWhiteModel = selectModelForBoard(requestedWhiteModels, board);
                const QString requestedBlackModel = selectModelForBoard(requestedBlackModels, board);
                boardFutures.push_back(std::async(std::launch::async,
                    [loop,
                     loops,
                     board,
                     whiteModel,
                     blackModel,
                     requestedWhiteModel,
                     requestedBlackModel,
                     refereeModels,
                     requestedRefereeModels,
                     refereeCount,
                     moveTimeout,
                     stackTimeout,
                     outputTokens,
                     gameTimeout,
                     maxPlies,
                     maxIllegal,
                     correctionRetries,
                     lookaheadLevel,
                     autoOutputTokens,
                     boardAwarenessProbe,
                     includeLegalMoveList,
                     referenceConfigId,
                     stackModule,
                     stackKind,
                     stackName,
                     agentOnlyNoStockfish]() {
                        QJsonObject result = agentOnlyNoStockfish
                            ? runAgentOnlyBoardGame(board,
                                                    whiteModel,
                                                    blackModel,
                                                    requestedWhiteModel,
                                                    requestedBlackModel,
                                                    refereeModels,
                                                    requestedRefereeModels,
                                                    moveTimeout,
                                                    stackTimeout,
                                                    outputTokens,
                                                    gameTimeout,
                                                    maxPlies,
                                                    correctionRetries)
                            : runBoardGame(board,
                                           whiteModel,
                                           blackModel,
                                           requestedWhiteModel,
                                           requestedBlackModel,
                                           refereeModels,
                                           requestedRefereeModels,
                                           refereeCount,
                                           moveTimeout,
                                           stackTimeout,
                                           outputTokens,
                                           gameTimeout,
                                           maxPlies,
                                           maxIllegal,
                                           correctionRetries,
                                           lookaheadLevel,
                                           autoOutputTokens,
                                           boardAwarenessProbe,
                                           includeLegalMoveList,
                                           referenceConfigId,
                                           stackModule,
                                           stackKind,
                                           stackName);
                        result["loop_index"] = loop + 1;
                        result["loop_count"] = loops;
                        return result;
                    }));
            }
            for (std::future<QJsonObject> &future : boardFutures) {
                results.append(future.get());
            }
        }
    } else {
        for (const QString &model : models) {
            if (mode == "one-move" || mode == "both") {
            results.append(runOneMove(model, moveTimeout, stackTimeout, outputTokens));
            }
            if (mode == "game" || mode == "both") {
            results.append(runGame(model, moveTimeout, stackTimeout, outputTokens, gameTimeout, maxPlies, maxIllegal));
            }
        }
    }
    writeOutputs(results);
    return 0;
}
