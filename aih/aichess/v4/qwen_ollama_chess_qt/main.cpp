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
#include <algorithm>
#include <future>
#include <vector>

static const QString kTestId = "aichess_v4_pairwise_prototype_20260729";
static const QString kOutDir =
    "/home/sag/RPA2/myLLC/AI/brilliance/aih/aichess/v4/runs/"
    "aih_v4_pairwise_prototype_20260729";
static const QString kStockfishPath = "/usr/games/stockfish";
static const QString kOllamaPath = "/usr/local/bin/ollama";
static const QString kOpenAiEndpoint = "https://api.openai.com/v1/responses";
static const QString kAnthropicEndpoint = "https://api.anthropic.com/v1/messages";
static const QString kDefaultGeminiCliModel = "gemini-cli-default";
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

static int traceStringLimit() {
    bool ok = false;
    const int configured = qEnvironmentVariableIntValue("AICHESS_TRACE_STRING_CHARS", &ok);
    if (ok) {
        return qBound(220, configured, 1024 * 1024);
    }
    return 8192;
}

static QString traceString(const QString &text) {
    const QString raw = rawResponseLine(text);
    const int maxTraceChars = traceStringLimit();
    if (raw.size() <= maxTraceChars) {
        return raw;
    }
    const QString suffix = QString("...<truncated raw_chars=%1 shown_chars=%2>")
        .arg(raw.size())
        .arg(maxTraceChars);
    const int prefixChars = qMax(0, maxTraceChars - suffix.size());
    if (prefixChars == 0) {
        return raw.left(maxTraceChars);
    }
    return raw.left(prefixChars) + suffix;
}

static QString traceSha256(const QString &text) {
    return QString::fromLatin1(QCryptographicHash::hash(text.toUtf8(), QCryptographicHash::Sha256).toHex());
}

static QString traceSha(const QString &text) {
    return traceSha256(text).left(12);
}

static QString readTextFile(const QString &path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    }
    return QString::fromUtf8(file.readAll());
}

static QString logoff(int depth) {
    return QString(depth, '\t');
}

static bool isModelHarnessEndpoint(const QString &endpoint) {
    return endpoint.startsWith("ollama:") ||
        endpoint.startsWith("openai:") ||
        endpoint.startsWith("codex:") ||
        endpoint.startsWith("gemini:");
}

static QString level2ModelIoText(const QString &text) {
    return text;
}

static void logLevel2String(const QString &token, const QString &text) {
    const QString value = text.trimmed();
    writeLog(1, QString("%1: strlen=%2 sha256=%3\n")
        .arg(token)
        .arg(value.size())
        .arg(traceSha256(value)));
}

static void logHarnessInput(const QString &label, const QString &inputSource, const QString &text) {
    if (kLogLevel == 2 && isModelHarnessEndpoint(inputSource)) {
        Q_UNUSED(label);
        const QString visibleText = level2ModelIoText(text);
        logLevel2String("mi", visibleText);
        return;
    }
    const int level = isModelHarnessEndpoint(inputSource)
        ? 1
        : 4;
    writeLog(level, QString("%1%2 module mi=\"%3\" miln=%4 miby=%5 misha=%6 mitr=%7 misrc=\"%8\"\n")
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
        logLevel2String("mo", visibleText);
        return;
    }
    const int level = isModelHarnessEndpoint(outputTarget)
        ? 1
        : 4;
    writeLog(level, QString("%1%2 module mo=\"%3\" moln=%4 moby=%5 mosha=%6 motr=%7 motgt=\"%8\"\n")
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

static QString fenBoardAndSide(const QString &fen) {
    return QString("%1 %2").arg(fen.section(' ', 0, 0), fen.section(' ', 1, 1));
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
    if (fenBoardAndSide(reportedBeforeFen) != fenBoardAndSide(expectedBeforeFen)) {
        if (reason) {
            *reason = "bm_mismatch";
        }
        return QString();
    }
    if (reportedAfterFen.isEmpty()) {
        if (reason) {
            *reason = "am_missing";
        }
        return QString();
    }

    QStringList matchingMoves;
    for (const QString &move : legalMoves) {
        DetBoardState afterBoard = beforeBoard;
        applyDetBoardMove(&afterBoard, move);
        if (fenBoardAndSide(detBoardFen(afterBoard)) == fenBoardAndSide(reportedAfterFen)) {
            matchingMoves << move;
        }
    }
    if (matchingMoves.size() == 1) {
        if (reason) {
            *reason = "bm_am_match_one_legal_move";
        }
        return matchingMoves.first();
    }
    if (reason) {
        *reason = matchingMoves.isEmpty() ? "am_matches_no_legal_move" : "am_matches_multiple_legal_moves";
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

static bool hasAnyGeminiAgent(const QStringList &models) {
    for (const QString &model : models) {
        if (model.startsWith("gemini:", Qt::CaseInsensitive) ||
            model.compare("gemini", Qt::CaseInsensitive) == 0 ||
            model.compare("gemini-cli", Qt::CaseInsensitive) == 0) {
            return true;
        }
    }
    return false;
}

static bool hasAnyAnthropicAgent(const QStringList &models) {
    for (const QString &model : models) {
        if (model.startsWith("anthropic:", Qt::CaseInsensitive) ||
            model.startsWith("claude-", Qt::CaseInsensitive)) {
            return true;
        }
    }
    return false;
}

static bool hasAnyCodexAgent(const QStringList &models) {
    for (const QString &model : models) {
        if (model.startsWith("codex:", Qt::CaseInsensitive) ||
            model.compare("codex", Qt::CaseInsensitive) == 0 ||
            model.compare("codex-cli", Qt::CaseInsensitive) == 0) {
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
    writeLog(3, QString("%1%2 runner routetgt=\"%3\" reason=\"%4\"\n")
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
    writeLog(3, QString("%1%2 stack_output status=\"%3\" done=%4 done_reason=\"%5\" output=\"%6\" outstrlen=%7 outsha256=%8 outtrunc=%9\n")
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
    const QString reportedBeforeFen = fieldValue(reply, "bm");
    const QString reportedAfterFen = fieldValue(reply, "am");
    if (!reportedBeforeFen.isEmpty() && !reportedAfterFen.isEmpty() && !legalMoves.isEmpty()) {
        DetBoardState beforeBoard = initialDetBoardState();
        QString transitionReason;
        const QString selectedFromTransition = moveFromReportedFenTransition(
            beforeBoard,
            legalMoves,
            reportedBeforeFen,
            reportedAfterFen,
            &transitionReason);
        if (!selectedFromTransition.isEmpty() &&
            (previous.isEmpty() || selectedFromTransition != previous)) {
            if (!result.candidates.contains(selectedFromTransition)) {
                result.candidates.prepend(selectedFromTransition);
            }
            result.selected = selectedFromTransition;
            result.reason = transitionReason;
            logParserOutput(result);
            return result;
        }
        if (result.candidates.isEmpty()) {
            result.reason = transitionReason;
            logParserOutput(result);
            return result;
        }
    }
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

static QJsonObject candidateDetailJson(const MoveParseResult &parse, const QStringList &legalMoves) {
    int legalCount = 0;
    int illegalCount = 0;
    for (const QString &candidate : parse.candidates) {
        if (legalMoves.contains(candidate)) {
            legalCount += 1;
        } else {
            illegalCount += 1;
        }
    }
    QJsonObject obj;
    obj["candidate_count"] = parse.candidates.size();
    obj["legal_candidate_count"] = legalCount;
    obj["illegal_candidate_count"] = illegalCount;
    obj["first_candidate_legal"] = !parse.candidates.isEmpty() && legalMoves.contains(parse.candidates.first());
    obj["contains_mixed_legal_illegal_candidates"] = legalCount > 0 && illegalCount > 0;
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

static QString errorCountText(int illegalTotal, int invalidTotal, int irrelevantTotal) {
    return QString(" il=%1 iv=%2 ir=%3 nr=%4")
        .arg(count3(illegalTotal))
        .arg(count3(invalidTotal))
        .arg(count3(irrelevantTotal))
        .arg(count3(illegalTotal + invalidTotal + irrelevantTotal));
}

static QString finalCountText(int illegalTotal, int invalidTotal, int correctionLimit) {
    return QString("nil=%1 niv=%2 ncr=%3")
        .arg(count3(illegalTotal), count3(invalidTotal), count3(correctionLimit));
}

static QString finalCountText(int illegalTotal, int invalidTotal, int irrelevantTotal, int correctionLimit) {
    return QString("nil=%1 niv=%2 nir=%3 ncr=%4")
        .arg(count3(illegalTotal))
        .arg(count3(invalidTotal))
        .arg(count3(irrelevantTotal))
        .arg(count3(correctionLimit));
}

static QString promptForMove(const QString &fen,
                             const QStringList &legalMoves,
                             int ply,
                             int lookaheadLevel = 0,
                             bool includeLegalMoveList = false,
                             int clueMode = 0,
                             const QString &suggestedMove = QString(),
                             const QString &suggestedAfterFen = QString()) {
    Q_UNUSED(lookaheadLevel);
    QString prompt = QString(
        "Chess move request.\n"
        "Ply: %1\n"
        "FEN: %2\n")
        .arg(ply)
        .arg(fen);
    if (clueMode == 1 || clueMode == 3) {
        prompt += QString("Clue: valid UCI moves for this position are: %1\n").arg(legalMoves.join(' '));
    }
    if (clueMode == 2 || clueMode == 3) {
        prompt += "Clue: the current board state has been verified as valid.\n";
    }
    if (clueMode >= 4) {
        prompt += QString("Clue: use this legal UCI move: %1\n").arg(suggestedMove);
        prompt += QString("Clue: current board FEN: %1\n").arg(fen);
    }
    if (clueMode >= 5) {
        prompt += QString("Clue: board FEN after that suggested move: %1\n").arg(suggestedAfterFen);
    }
    if (clueMode >= 6) {
        prompt += "Highest scaffold task: choose one legal chess move for this position.\n";
        return prompt;
    }
    if (includeLegalMoveList) {
        prompt += QString("Legal UCI moves: %1\n").arg(legalMoves.join(' '));
    }
    prompt +=
        "Choose one legal chess move for this position.";
    return prompt;
}

static QString promptForReferee(const QString &fen, const QStringList &legalMoves, const QString &candidateMove, int ply) {
    return QString(
        "You are a strict chess referee. Decide whether the candidate move is "
        "legal for the given position.\n"
        "Ply: %1\n"
        "FEN: %2\n"
        "Legal UCI moves: %3\n"
        "Candidate move: %4")
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
        "Choose one replacement legal chess move for this position.")
        .arg(verdict)
        .arg(fen);
}

static QString promptForAgentOnlyMove(const QStringList &moves, const QString &side, int ply) {
    return QString(
        "Chess move request.\n"
        "Ply: %1\n"
        "Side to move: %2\n"
        "Move history: %3\n"
        "Choose one legal chess move.")
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
        "Decide whether the candidate move is legal.")
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
        "Choose one replacement legal chess move.")
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

static bool isGeminiModel(const QString &model) {
    return model.startsWith("gemini:", Qt::CaseInsensitive) ||
        model.compare("gemini", Qt::CaseInsensitive) == 0 ||
        model.compare("gemini-cli", Qt::CaseInsensitive) == 0;
}

static bool isAnthropicModel(const QString &model) {
    return model.startsWith("anthropic:", Qt::CaseInsensitive) ||
        model.startsWith("claude-", Qt::CaseInsensitive);
}

static bool isCodexModel(const QString &model) {
    return model.startsWith("codex:", Qt::CaseInsensitive) ||
        model.compare("codex", Qt::CaseInsensitive) == 0 ||
        model.compare("codex-cli", Qt::CaseInsensitive) == 0;
}

static QString openAiModelName(const QString &model) {
    const QString name = isOpenAiModel(model) ? model.section(':', 1) : model;
    return name.isEmpty() ? QString::fromLocal8Bit(qgetenv("AICHESS_OPENAI_MODEL")) : name;
}

static QString reasoningPerformanceMode() {
    const QString mode = QString::fromLocal8Bit(qgetenv("AICHESS_REASONING_PERFORMANCE_MODE")).trimmed();
    if (!mode.isEmpty() && mode != "default") {
        return mode;
    }
    const QString openAiMode = QString::fromLocal8Bit(qgetenv("AICHESS_OPENAI_REASONING_EFFORT")).trimmed();
    if (!openAiMode.isEmpty() && openAiMode != "default") {
        return openAiMode;
    }
    return "medium";
}

static QString openAiTextVerbosity() {
    QString verbosity = QString::fromLocal8Bit(qgetenv("AICHESS_VERBOSITY")).trimmed();
    if (verbosity.isEmpty() || verbosity == "default") {
        verbosity = QString::fromLocal8Bit(qgetenv("AICHESS_OPENAI_TEXT_VERBOSITY")).trimmed();
    }
    if (!verbosity.isEmpty() && verbosity != "default") {
        return verbosity;
    }
    return "medium";
}

static QString openAiReasoningEffort() {
    const QString mode = reasoningPerformanceMode();
    if (mode == "low" || mode == "medium" || mode == "high") {
        return mode;
    }
    return "high";
}

static QString anthropicEffortForReasoningMode(const QString &mode) {
    if (mode == "low") return "medium";
    if (mode == "medium") return "medium";
    if (mode == "xhigh") return "max";
    return "max";
}

static QString geminiThinkingLevelForReasoningMode(const QString &mode) {
    if (mode == "low") return "LOW";
    if (mode == "medium") return "MEDIUM";
    return "HIGH";
}

static QString anthropicModelName(const QString &model) {
    const QString name = model.startsWith("anthropic:", Qt::CaseInsensitive) ? model.section(':', 1) : model;
    if (name == "claude-opus-4") return "claude-opus-4-8";
    if (name == "claude-sonnet-4") return "claude-sonnet-4-6";
    if (name == "claude-3-7-sonnet") return "claude-sonnet-4-6";
    if (name == "claude-3-5-haiku") return "claude-haiku-4-5";
    return name.isEmpty() ? QString::fromLocal8Bit(qgetenv("AICHESS_ANTHROPIC_MODEL")) : name;
}

static QString geminiModelName(const QString &model) {
    if (model.compare("gemini", Qt::CaseInsensitive) == 0 ||
        model.compare("gemini-cli", Qt::CaseInsensitive) == 0) {
        return QString::fromLocal8Bit(qgetenv("AICHESS_GEMINI_MODEL"));
    }
    const QString name = isGeminiModel(model) ? model.section(':', 1) : model;
    if (name.isEmpty() || name.compare("cli", Qt::CaseInsensitive) == 0 ||
        name.compare("default", Qt::CaseInsensitive) == 0) {
        return QString::fromLocal8Bit(qgetenv("AICHESS_GEMINI_MODEL"));
    }
    return name;
}

static QString codexModelName(const QString &model) {
    const QString envModel = QString::fromLocal8Bit(qgetenv("AICHESS_CODEX_MODEL")).trimmed();
    if (model.startsWith("codex:", Qt::CaseInsensitive)) {
        const QString name = model.section(':', 1).trimmed();
        if (name.isEmpty() || name.compare("codex-cli", Qt::CaseInsensitive) == 0 ||
            name.compare("default", Qt::CaseInsensitive) == 0) {
            return envModel;
        }
        return name;
    }
    if (model.compare("codex", Qt::CaseInsensitive) == 0 ||
        model.compare("codex-cli", Qt::CaseInsensitive) == 0) {
        return envModel;
    }
    return model;
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

static bool isGeminiStackModule(const QString &stackModule) {
    const QString normalized = normalizedStackModule(stackModule);
    return normalized == "gemini_cli" || normalized == "gemini" || normalized == "cloud_gemini";
}

static bool isAnthropicStackModule(const QString &stackModule) {
    const QString normalized = normalizedStackModule(stackModule);
    return normalized == "anthropic_messages" || normalized == "anthropic" || normalized == "cloud_anthropic";
}

static bool isCodexStackModule(const QString &stackModule) {
    const QString normalized = normalizedStackModule(stackModule);
    return normalized == "codex_cli" || normalized == "codex" || normalized == "local_codex_cli";
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
    if (isGeminiModel(model)) {
        return "gemini_cli";
    }
    if (isAnthropicModel(model)) {
        return "anthropic_messages";
    }
    if (isCodexModel(model)) {
        return "codex_cli";
    }
    return "ollama_generate";
}

static void applyOllamaGenerateBody(OllamaResult *result, const QByteArray &body) {
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        result->status = "bad_json";
        result->stdoutText = QString::fromUtf8(body).trimmed();
        result->stderrText = parseError.errorString();
        return;
    }

    const QJsonObject obj = doc.object();
    result->status = "completed";
    result->exitCode = 0;
    result->backendDone = obj.value("done").toBool(false);
    result->doneReason = obj.value("done_reason").toString();
    result->stdoutText = obj.value("response").toString().trimmed();
    if (!result->backendDone) {
        result->status = "incomplete_response";
        result->stderrText = "Ollama response did not report done=true";
    }
    if (obj.contains("error")) {
        result->status = "request_failed";
        result->stderrText = obj.value("error").toString();
    }
}

static OllamaResult askOllamaWithCurl(const QString &model,
                                      const QJsonDocument &payloadDoc,
                                      int timeoutSeconds,
                                      const QElapsedTimer &timer) {
    OllamaResult result;
    QProcess process;
    process.setProgram("curl");
    process.setArguments({
        "-sS",
        "http://127.0.0.1:11434/api/generate",
        "-H",
        "Content-Type: application/json",
        "-d",
        QString::fromUtf8(payloadDoc.toJson(QJsonDocument::Compact)),
    });
    process.start();
    if (!process.waitForStarted(5000)) {
        result.status = "request_failed";
        result.stderrText = QString("failed to start curl fallback for Ollama %1: %2").arg(model, process.errorString());
        result.elapsedSeconds = timer.elapsed() / 1000.0;
        return result;
    }
    if (!process.waitForFinished(timeoutSeconds * 1000)) {
        process.kill();
        process.waitForFinished(3000);
        result.status = "timed_out";
        result.stdoutText = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
        result.stderrText = QString::fromUtf8(process.readAllStandardError()).trimmed();
        result.elapsedSeconds = timer.elapsed() / 1000.0;
        return result;
    }

    result.elapsedSeconds = timer.elapsed() / 1000.0;
    result.exitCode = process.exitCode();
    const QByteArray body = process.readAllStandardOutput();
    const QString stderrText = QString::fromUtf8(process.readAllStandardError()).trimmed();
    if (process.exitStatus() != QProcess::NormalExit || result.exitCode != 0) {
        result.status = "request_failed";
        result.stdoutText = QString::fromUtf8(body).trimmed();
        result.stderrText = stderrText.isEmpty()
            ? QString("curl fallback exited with code %1").arg(result.exitCode)
            : stderrText;
        return result;
    }
    applyOllamaGenerateBody(&result, body);
    if (!stderrText.isEmpty() && result.stderrText.isEmpty()) {
        result.stderrText = stderrText;
    }
    return result;
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
    const QJsonDocument payloadDoc(payload);

    OllamaResult curlResult = askOllamaWithCurl(model, payloadDoc, timeoutSeconds, timer);
    logHarnessInput(traceLabel, traceEndpoint, curlResult.stdoutText);
    return curlResult;

    QNetworkAccessManager manager;
    QNetworkRequest request(QUrl("http://127.0.0.1:11434/api/generate"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply *reply = manager.post(request, payloadDoc.toJson(QJsonDocument::Compact));
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
        if (result.stdoutText.isEmpty()) {
            result = askOllamaWithCurl(model, payloadDoc, timeoutSeconds, timer);
        }
        logHarnessInput(traceLabel, traceEndpoint, result.stdoutText);
        return result;
    }

    applyOllamaGenerateBody(&result, body);
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
    if (resolvedModel.startsWith("gpt-5", Qt::CaseInsensitive)) {
        QJsonObject reasoning;
        reasoning["effort"] = openAiReasoningEffort();
        payload["reasoning"] = reasoning;

        QJsonObject text;
        text["verbosity"] = openAiTextVerbosity();
        payload["text"] = text;
    }

    QNetworkAccessManager manager;
    QNetworkRequest request{QUrl(kOpenAiEndpoint)};
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + apiKey.toUtf8());

    QNetworkReply *reply = manager.post(request, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
    if (!startupLabel.isEmpty()) {
        writeLog(3, QString("%1: i/o pending...\n").arg(startupLabel));
    }
    timeout.start(timeoutSeconds * 1000);
    loop.exec();

    result.elapsedSeconds = timer.elapsed() / 1000.0;
    if (timeout.isActive()) {
        timeout.stop();
        if (!startupLabel.isEmpty()) {
            writeLog(3, QString("%1: i/o complete\n").arg(startupLabel));
        }
    } else {
        reply->abort();
        reply->deleteLater();
        result.status = "timed_out";
        if (!startupLabel.isEmpty()) {
            writeLog(3, QString("%1: i/o error: timeout\n").arg(startupLabel));
        }
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
    if (result.doneReason != "completed" && result.stdoutText.isEmpty()) {
        result.status = "incomplete_response";
        const QJsonObject details = obj.value("incomplete_details").toObject();
        const QString reason = details.value("reason").toString();
        result.stderrText = reason.isEmpty()
            ? QString("OpenAI response status: %1").arg(result.doneReason)
            : QString("OpenAI response status: %1 reason=%2").arg(result.doneReason, reason);
    }
    const QJsonValue errorValue = obj.value("error");
    if (!errorValue.isUndefined() && !errorValue.isNull()) {
        result.status = "request_failed";
        if (errorValue.isObject()) {
            const QJsonObject error = errorValue.toObject();
            result.stderrText = error.value("message").toString();
        } else {
            result.stderrText = errorValue.toVariant().toString();
        }
    }
    reply->deleteLater();
    logHarnessInput(traceLabel, traceEndpoint, result.stdoutText);
    return result;
}

static OllamaResult askGeminiCli(const QString &model,
                                 const QString &prompt,
                                 int timeoutSeconds,
                                 int numPredict,
                                 const QString &startupLabel = QString()) {
    Q_UNUSED(numPredict);
    QElapsedTimer timer;
    timer.start();
    OllamaResult result;
    const QString traceLabel = startupLabel.isEmpty() ? QString("gemini") : startupLabel + " gemini";
    const QString requestedModel = geminiModelName(model);
    const QString traceEndpoint = requestedModel.isEmpty()
        ? QString("gemini:%1").arg(kDefaultGeminiCliModel)
        : QString("gemini:%1").arg(requestedModel);
    logHarnessOutput(traceLabel, traceEndpoint, prompt);

    const QString cli = QString::fromLocal8Bit(qgetenv("AICHESS_GEMINI_CLI")).trimmed().isEmpty()
        ? QString("gemini")
        : QString::fromLocal8Bit(qgetenv("AICHESS_GEMINI_CLI")).trimmed();
    QProcess process;
    process.setProgram(cli);
    QStringList args;
    if (!requestedModel.isEmpty()) {
        args << "-m" << requestedModel;
    }
    args << "-p" << prompt
         << "--output-format" << "text";
    process.setArguments(args);

    if (!startupLabel.isEmpty()) {
        writeLog(3, QString("%1: starting gemini cli\n").arg(startupLabel));
    }
    process.start();
    if (!process.waitForStarted(5000)) {
        result.status = "request_failed";
        result.stderrText = QString("failed to start Gemini CLI: %1").arg(process.errorString());
        result.elapsedSeconds = timer.elapsed() / 1000.0;
        logHarnessInput(traceLabel, traceEndpoint, result.stdoutText);
        return result;
    }
    if (!process.waitForFinished(timeoutSeconds * 1000)) {
        process.kill();
        process.waitForFinished(3000);
        result.status = "timed_out";
        result.elapsedSeconds = timer.elapsed() / 1000.0;
        result.stdoutText = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
        result.stderrText = QString::fromUtf8(process.readAllStandardError()).trimmed();
        logHarnessInput(traceLabel, traceEndpoint, result.stdoutText);
        return result;
    }

    result.elapsedSeconds = timer.elapsed() / 1000.0;
    result.exitCode = process.exitCode();
    result.stdoutText = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
    result.stderrText = QString::fromUtf8(process.readAllStandardError()).trimmed();
    if (process.exitStatus() != QProcess::NormalExit || result.exitCode != 0) {
        result.status = "request_failed";
    } else if (result.stdoutText.isEmpty()) {
        result.status = "incomplete_response";
        result.stderrText = result.stderrText.isEmpty() ? "Gemini CLI returned empty output" : result.stderrText;
    } else {
        result.status = "completed";
        result.backendDone = true;
        result.doneReason = "completed";
    }
    logHarnessInput(traceLabel, traceEndpoint, result.stdoutText);
    return result;
}

static QString geminiOutputText(const QJsonObject &obj) {
    QStringList parts;
    const QJsonArray candidates = obj.value("candidates").toArray();
    for (const QJsonValue &candidateValue : candidates) {
        const QJsonObject candidate = candidateValue.toObject();
        const QJsonObject content = candidate.value("content").toObject();
        const QJsonArray contentParts = content.value("parts").toArray();
        for (const QJsonValue &partValue : contentParts) {
            const QString text = partValue.toObject().value("text").toString().trimmed();
            if (!text.isEmpty()) {
                parts << text;
            }
        }
    }
    return parts.join('\n').trimmed();
}

static OllamaResult askGeminiGenerateContent(const QString &model,
                                             const QString &prompt,
                                             int timeoutSeconds,
                                             int numPredict,
                                             const QString &startupLabel = QString()) {
    QElapsedTimer timer;
    timer.start();
    OllamaResult result;
    const QString traceLabel = startupLabel.isEmpty() ? QString("gemini") : startupLabel + " gemini";
    const QString apiKey = QString::fromLocal8Bit(qgetenv("GEMINI_API_KEY"));
    const QString resolvedModel = geminiModelName(model);
    const QString traceEndpoint = resolvedModel.isEmpty() ? QString("gemini") : "gemini:" + resolvedModel;
    logHarnessOutput(traceLabel, traceEndpoint, prompt);
    if (apiKey.isEmpty()) {
        result.status = "request_failed";
        result.stderrText = "GEMINI_API_KEY is not set";
        logHarnessInput(traceLabel, traceEndpoint, result.stdoutText);
        return result;
    }
    if (resolvedModel.isEmpty()) {
        result.status = "request_failed";
        result.stderrText = "Gemini model is empty; use gemini:<model> or AICHESS_GEMINI_MODEL";
        logHarnessInput(traceLabel, traceEndpoint, result.stdoutText);
        return result;
    }

    QJsonObject part;
    part["text"] = prompt;
    QJsonArray parts;
    parts.append(part);
    QJsonObject content;
    content["parts"] = parts;
    QJsonArray contents;
    contents.append(content);
    QJsonObject generationConfig;
    generationConfig["maxOutputTokens"] = numPredict;
    if (resolvedModel.startsWith("gemini-3", Qt::CaseInsensitive)) {
        QJsonObject thinkingConfig;
        thinkingConfig["thinkingLevel"] = geminiThinkingLevelForReasoningMode(reasoningPerformanceMode());
        generationConfig["thinkingConfig"] = thinkingConfig;
    }
    QJsonObject payload;
    payload["contents"] = contents;
    payload["generationConfig"] = generationConfig;

    const QString apiVersion = QString::fromLocal8Bit(qgetenv("GEMINI_API_VERSION")).trimmed().isEmpty()
        ? QString("v1beta")
        : QString::fromLocal8Bit(qgetenv("GEMINI_API_VERSION")).trimmed();
    const QUrl url(QString("https://generativelanguage.googleapis.com/%1/models/%2:generateContent?key=%3")
        .arg(apiVersion, resolvedModel, apiKey));
    QNetworkAccessManager manager;
    QNetworkRequest request{url};
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply *reply = manager.post(request, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
    timeout.start(timeoutSeconds * 1000);
    loop.exec();

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
    const QJsonObject error = obj.value("error").toObject();
    if (!error.isEmpty()) {
        result.status = "request_failed";
        result.stderrText = error.value("message").toString();
        result.stdoutText = QString::fromUtf8(body).trimmed();
        reply->deleteLater();
        logHarnessInput(traceLabel, traceEndpoint, result.stdoutText);
        return result;
    }
    result.status = "completed";
    result.exitCode = 0;
    result.backendDone = true;
    result.doneReason = "completed";
    result.stdoutText = geminiOutputText(obj);
    if (result.stdoutText.isEmpty()) {
        result.status = "incomplete_response";
        result.stderrText = "Gemini response contained no text content";
        result.stdoutText = QString::fromUtf8(body).trimmed();
    }
    reply->deleteLater();
    logHarnessInput(traceLabel, traceEndpoint, result.stdoutText);
    return result;
}

static QString anthropicOutputText(const QJsonObject &obj) {
    QStringList parts;
    const QJsonArray content = obj.value("content").toArray();
    for (const QJsonValue &contentValue : content) {
        const QJsonObject contentObj = contentValue.toObject();
        const QString text = contentObj.value("text").toString().trimmed();
        if (!text.isEmpty()) {
            parts << text;
        }
    }
    return parts.join('\n').trimmed();
}

static OllamaResult askAnthropic(const QString &model,
                                 const QString &prompt,
                                 int timeoutSeconds,
                                 int numPredict,
                                 const QString &startupLabel = QString()) {
    QElapsedTimer timer;
    timer.start();
    OllamaResult result;
    const QString traceLabel = startupLabel.isEmpty() ? QString("anthropic") : startupLabel + " anthropic";
    const QString apiKey = QString::fromLocal8Bit(qgetenv("ANTHROPIC_API_KEY"));
    const QString resolvedModel = anthropicModelName(model);
    const QString traceEndpoint = resolvedModel.isEmpty() ? QString("anthropic") : "anthropic:" + resolvedModel;
    logHarnessOutput(traceLabel, traceEndpoint, prompt);
    if (apiKey.isEmpty()) {
        result.status = "request_failed";
        result.stderrText = "ANTHROPIC_API_KEY is not set";
        logHarnessInput(traceLabel, traceEndpoint, result.stdoutText);
        return result;
    }
    if (resolvedModel.isEmpty()) {
        result.status = "request_failed";
        result.stderrText = "Anthropic model is empty; use anthropic:<model> or AICHESS_ANTHROPIC_MODEL";
        logHarnessInput(traceLabel, traceEndpoint, result.stdoutText);
        return result;
    }

    QJsonObject message;
    message["role"] = "user";
    message["content"] = prompt;
    QJsonArray messages;
    messages.append(message);

    QJsonObject payload;
    payload["model"] = resolvedModel;
    payload["max_tokens"] = numPredict;
    payload["messages"] = messages;
    if (resolvedModel.contains("claude-haiku-4-5", Qt::CaseInsensitive)) {
        if (numPredict > 1024) {
            QJsonObject thinking;
            thinking["type"] = "enabled";
            thinking["budget_tokens"] = 1024;
            thinking["display"] = "omitted";
            payload["thinking"] = thinking;
        }
    } else if (resolvedModel.contains("claude-opus-4-8", Qt::CaseInsensitive) ||
               resolvedModel.contains("claude-sonnet-4-6", Qt::CaseInsensitive)) {
        QJsonObject thinking;
        thinking["type"] = "adaptive";
        thinking["display"] = "omitted";
        payload["thinking"] = thinking;

        QJsonObject outputConfig;
        outputConfig["effort"] = anthropicEffortForReasoningMode(reasoningPerformanceMode());
        payload["output_config"] = outputConfig;
    }

    QNetworkAccessManager manager;
    QNetworkRequest request{QUrl(kAnthropicEndpoint)};
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("x-api-key", apiKey.toUtf8());
    request.setRawHeader("anthropic-version", "2023-06-01");

    QNetworkReply *reply = manager.post(request, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
    if (!startupLabel.isEmpty()) {
        writeLog(3, QString("%1: i/o pending...\n").arg(startupLabel));
    }
    timeout.start(timeoutSeconds * 1000);
    loop.exec();

    result.elapsedSeconds = timer.elapsed() / 1000.0;
    if (timeout.isActive()) {
        timeout.stop();
        if (!startupLabel.isEmpty()) {
            writeLog(3, QString("%1: i/o complete\n").arg(startupLabel));
        }
    } else {
        reply->abort();
        reply->deleteLater();
        result.status = "timed_out";
        if (!startupLabel.isEmpty()) {
            writeLog(3, QString("%1: i/o error: timeout\n").arg(startupLabel));
        }
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
    const QJsonObject error = obj.value("error").toObject();
    if (!error.isEmpty()) {
        result.status = "request_failed";
        result.stderrText = error.value("message").toString();
        result.stdoutText = QString::fromUtf8(body).trimmed();
        reply->deleteLater();
        logHarnessInput(traceLabel, traceEndpoint, result.stdoutText);
        return result;
    }
    result.status = "completed";
    result.exitCode = 0;
    result.backendDone = true;
    result.doneReason = obj.value("stop_reason").toString();
    result.stdoutText = anthropicOutputText(obj);
    if (result.stdoutText.isEmpty()) {
        result.status = "incomplete_response";
        result.stderrText = "Anthropic response contained no text content";
    }
    reply->deleteLater();
    logHarnessInput(traceLabel, traceEndpoint, result.stdoutText);
    return result;
}

static OllamaResult askCodexCli(const QString &model,
                                const QString &prompt,
                                int timeoutSeconds,
                                int numPredict,
                                const QString &startupLabel = QString()) {
    Q_UNUSED(numPredict);
    QElapsedTimer timer;
    timer.start();
    OllamaResult result;
    const QString traceLabel = startupLabel.isEmpty() ? QString("codex") : startupLabel + " codex";
    const QString resolvedModel = codexModelName(model);
    const QString traceEndpoint = resolvedModel.isEmpty() ? QString("codex:codex-cli-default") : "codex:" + resolvedModel;
    logHarnessOutput(traceLabel, traceEndpoint, prompt);

    QDir().mkpath(kOutDir);
    const QString cli = QString::fromLocal8Bit(qgetenv("AICHESS_CODEX_CLI")).trimmed().isEmpty()
        ? QString("codex")
        : QString::fromLocal8Bit(qgetenv("AICHESS_CODEX_CLI")).trimmed();
    const QString lastMessagePath = QString("%1/codex_last_message_%2_%3.txt")
        .arg(kOutDir, traceSha(prompt), QDateTime::currentDateTimeUtc().toString("yyyyMMddHHmmsszzz"));

    QProcess process;
    process.setProgram(cli);
    QStringList args;
    args << "exec"
         << "--sandbox" << "read-only"
         << "--ask-for-approval" << "never"
         << "--skip-git-repo-check"
         << "--ephemeral"
         << "--color" << "never"
         << "-C" << kOutDir
         << "--output-last-message" << lastMessagePath;
    if (!resolvedModel.isEmpty()) {
        args << "-m" << resolvedModel;
    }
    args << "-";
    process.setArguments(args);

    if (!startupLabel.isEmpty()) {
        writeLog(3, QString("%1: starting codex cli\n").arg(startupLabel));
    }
    process.start();
    if (!process.waitForStarted(5000)) {
        result.status = "request_failed";
        result.stderrText = QString("failed to start Codex CLI: %1").arg(process.errorString());
        result.elapsedSeconds = timer.elapsed() / 1000.0;
        logHarnessInput(traceLabel, traceEndpoint, result.stdoutText);
        return result;
    }
    process.write(prompt.toUtf8());
    process.closeWriteChannel();
    if (!process.waitForFinished(timeoutSeconds * 1000)) {
        process.kill();
        process.waitForFinished(3000);
        result.status = "timed_out";
        result.elapsedSeconds = timer.elapsed() / 1000.0;
        result.stdoutText = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
        result.stderrText = QString::fromUtf8(process.readAllStandardError()).trimmed();
        logHarnessInput(traceLabel, traceEndpoint, result.stdoutText);
        return result;
    }

    result.elapsedSeconds = timer.elapsed() / 1000.0;
    result.exitCode = process.exitCode();
    const QString stdoutText = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
    const QString stderrText = QString::fromUtf8(process.readAllStandardError()).trimmed();
    const QString lastMessage = readTextFile(lastMessagePath).trimmed();
    result.stdoutText = lastMessage.isEmpty() ? stdoutText : lastMessage;
    result.stderrText = stderrText;
    if (!lastMessagePath.isEmpty()) {
        result.doneReason = QString("last_message=%1").arg(lastMessagePath);
    }
    if (process.exitStatus() != QProcess::NormalExit || result.exitCode != 0) {
        result.status = "request_failed";
        if (result.stderrText.isEmpty()) {
            result.stderrText = QString("Codex CLI exited with code %1").arg(result.exitCode);
        }
    } else if (result.stdoutText.isEmpty()) {
        result.status = "incomplete_response";
        result.stderrText = result.stderrText.isEmpty() ? "Codex CLI returned empty output" : result.stderrText;
    } else {
        result.status = "completed";
        result.backendDone = true;
        if (result.doneReason.isEmpty()) {
            result.doneReason = "completed";
        }
    }
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
    if (isGeminiStackModule(stackModule)) {
        return askGeminiGenerateContent(model, prompt, timeoutSeconds, numPredict, startupLabel);
    }
    if (isAnthropicStackModule(stackModule)) {
        return askAnthropic(model, prompt, timeoutSeconds, numPredict, startupLabel);
    }
    if (isCodexStackModule(stackModule)) {
        return askCodexCli(model, prompt, timeoutSeconds, numPredict, startupLabel);
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
    obj["module_boundary"] = "runner_prompt_bytes_to_stkmdl_raw_response_bytes";
    if (isOpenAiStackModule(normalized)) {
        obj["transport"] = "https";
        obj["provider"] = "openai";
        obj["api_surface"] = "responses";
        obj["requires_env"] = "OPENAI_API_KEY";
    } else if (isGeminiStackModule(normalized)) {
        obj["transport"] = "https";
        obj["provider"] = "google";
        obj["api_surface"] = "generateContent";
        obj["requires_env"] = "GEMINI_API_KEY";
    } else if (isAnthropicStackModule(normalized)) {
        obj["transport"] = "https";
        obj["provider"] = "anthropic";
        obj["api_surface"] = "messages";
        obj["requires_env"] = "ANTHROPIC_API_KEY";
    } else if (isCodexStackModule(normalized)) {
        obj["transport"] = "local_cli";
        obj["provider"] = "openai";
        obj["api_surface"] = "codex_exec";
        obj["requires_env"] = "";
        obj["supports_session_context"] = false;
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

static QString nonresponseFailureClass(const QString &stackModule) {
    if (isOllamaStackModule(stackModule)) {
        int exitCode = -1;
        runTextProcess("curl",
                       {"-sS", "--max-time", "2", "http://127.0.0.1:11434/api/tags"},
                       3000,
                       &exitCode);
        return exitCode == 0 ? "agt_noresp_stk_ok" : "stk_noresp";
    }
    if (isGeminiStackModule(stackModule) || isOpenAiStackModule(stackModule) || isAnthropicStackModule(stackModule)) {
        return "cld_stk_or_agt_noresp";
    }
    return "stk_or_agt_noresp";
}

static QString responseFailureClass(const OllamaResult &result) {
    if (result.status == "completed") {
        return "none";
    }
    if (result.status == "timed_out" || result.status == "game_timeout") {
        return "stack_timeout";
    }
    if (result.status == "bad_json") {
        return "adapter_bad_json";
    }
    if (result.status == "incomplete_response") {
        return "adapter_incomplete_response";
    }
    const QString signal = (result.status + " " + result.stderrText + " " + result.stdoutText).toLower();
    if (signal.contains("api_key is not set") ||
        signal.contains("api key is not set") ||
        signal.contains("missing api key") ||
        signal.contains("no api key")) {
        return "missing_provider_key";
    }
    if (signal.contains("resource_exhausted") ||
        signal.contains("rate limit") ||
        signal.contains("rate_limit") ||
        signal.contains("too many requests") ||
        signal.contains("http 429") ||
        signal.contains("\"code\":429") ||
        signal.contains("quota")) {
        return "cloud_rate_limit_or_quota_throttle";
    }
    if (signal.contains("unauthorized") ||
        signal.contains("forbidden") ||
        signal.contains("permission") ||
        signal.contains("not authorized") ||
        signal.contains("access denied") ||
        signal.contains("entitlement") ||
        signal.contains("account") ||
        signal.contains("billing") ||
        signal.contains("license")) {
        return "cloud_authorization_or_entitlement_failure";
    }
    if (signal.contains("disabled") ||
        signal.contains("deactivated") ||
        signal.contains("retired") ||
        signal.contains("deprecated") ||
        signal.contains("model not found") ||
        signal.contains("not found for api version") ||
        signal.contains("not supported") ||
        signal.contains("unavailable")) {
        return "suspected_remote_disablement_or_stack_availability";
    }
    if (result.status == "request_failed") {
        return "stack_request_failed";
    }
    return "stack_failure";
}

static QJsonObject ollamaJson(const OllamaResult &result) {
    QJsonObject obj;
    obj["status"] = result.status;
    obj["failure_class"] = responseFailureClass(result);
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
    const MoveParseResult parse = parseViaHarness("one_move", "agent", "rules", movePrompt, response.stdoutText, legal);
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
                                                      "rules",
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
    obj["illegal_move_total"] = illegalCount;
    obj["invalid_move_total"] = 0;
    obj["rejected_move_total"] = illegalCount;
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

static QString aihResultMode(const QString &gameResult, const QString &termination) {
    if (termination.contains("cloud_rate_limit_or_quota_throttle")) {
        return "aih_failure_cloud_rate_limit_or_quota_throttle";
    }
    if (gameResult == "invalid_run") {
        return "aih_failure_run_invalidated";
    }
    if (termination.contains("forfeit") ||
        termination.contains("transport_failure") ||
        termination.contains("invalid_or_unparseable") ||
        termination.contains("move_timeout")) {
        return "aih_failure_agent_stopped_playing_chess";
    }
    if (termination.endsWith("checkmate") ||
        termination == "stalemate" ||
        termination == "game_completed" ||
        termination == "draw_by_configured_ply_limit" ||
        termination == "game_timeout") {
        return "aih_tournament_game_played";
    }
    return "aih_result_unclassified";
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
                              "rules",
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
                                  "rules",
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
            writeLog(3, "mv: \"timeout\"\n");
            writeLog(3, QString("%1:TIMEO -f  DT:%2 ET:%3%4\n").arg(plyPrefix, dtText, etText, counts));
            termination = response.status == "game_timeout" ? "game_timeout" : side + "_forfeit_move_timeout";
            gameResult = whiteToMove ? "black_win" : "white_win";
            break;
        }
        if (response.status != "completed") {
            writeLog(3, QString("mv: \"request_failed %1\"\n").arg(rawResponseLine(response.status)));
            writeLog(3, QString("%1:REQF  -f  DT:%2 ET:%3%4 status=%5\n")
                .arg(plyPrefix, dtText, etText, counts, response.status));
            termination = side + "_forfeit_transport_failure";
            gameResult = whiteToMove ? "black_win" : "white_win";
            break;
        }
        if (accepted) {
            moves << parsed;
            writeLog(3, QString("mv: \"%1\"\nrf: \"legal\"\n").arg(rawResponseLine(parsed)));
            writeLog(3, QString("%1:%2 -a  DT:%3 ET:%4%5\n")
                .arg(plyPrefix, parsed.leftJustified(5, ' '), dtText, etText, counts));
            continue;
        }

        writeLog(3, QString("mv: \"%1\"\nrf: \"%2\"\n")
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
                                int clueMode,
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
    obj["reasoning_performance_mode"] = reasoningPerformanceMode();
    obj["verbosity"] = openAiTextVerbosity();
    obj["openai_reasoning_effort_effective"] = openAiReasoningEffort();
    obj["openai_text_verbosity"] = openAiTextVerbosity();
    obj["anthropic_reasoning_effort_effective"] = anthropicEffortForReasoningMode(reasoningPerformanceMode());
    obj["gemini_thinking_level_effective"] = geminiThinkingLevelForReasoningMode(reasoningPerformanceMode());
    obj["hrnrol"] = "controlling_reference";
    obj["brdid"] = boardId;
    obj["white_model"] = whiteModel;
    obj["black_model"] = blackModel;
    obj["requested_white_model"] = requestedWhiteModel;
    obj["requested_black_model"] = requestedBlackModel;
    obj["model"] = whiteModel + " vs " + blackModel;
    obj["role_assignment"] = QString("white=%1 resolved=%2; black=%3 resolved=%4")
        .arg(requestedWhiteModel, whiteModel, requestedBlackModel, blackModel);
    obj["ground_truth_rules_engine"] = "rules_dtrm_chess_v2";
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
        ? "rules_dtrm_chess_v2"
        : "agentic_referee_vote";
    QJsonObject agentConfiguration;
    agentConfiguration["brdid"] = boardId;
    agentConfiguration["rqstkmdl"] = requestedStackModule;
    agentConfiguration["stktyp"] = stackKind;
    agentConfiguration["stknm"] = stackName;
    agentConfiguration["reasoning_performance_mode"] = reasoningPerformanceMode();
    agentConfiguration["verbosity"] = openAiTextVerbosity();
    agentConfiguration["openai_reasoning_effort_effective"] = openAiReasoningEffort();
    agentConfiguration["openai_text_verbosity"] = openAiTextVerbosity();
    agentConfiguration["anthropic_reasoning_effort_effective"] = anthropicEffortForReasoningMode(reasoningPerformanceMode());
    agentConfiguration["gemini_thinking_level_effective"] = geminiThinkingLevelForReasoningMode(reasoningPerformanceMode());
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
            assignment["stkmdl"] = "rules_reference";
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
    obj["clue_mode"] = clueMode;
    obj["hallucination_test"] = true;
    DetBoardState brdst = initialDetBoardState();
    QString brdfen = detBoardFen(brdst);
    QStringList moves;
    QJsonArray events;
    int illegalCount = 0;
    int illegalMoveTotal = 0;
    int invalidMoveTotal = 0;
    int irrelevantAgentReturnTotal = 0;
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
        const int dummyMoveCalls = 0;
        const int maxFirstWhiteAttempts = qMax(1, correctionRetries);
        const QString suggestedMove = legal.isEmpty() ? QString() : legal.first();
        QString suggestedAfterFen;
        if (!suggestedMove.isEmpty()) {
            DetBoardState suggestedAfterBoard = brdst;
            applyDetBoardMove(&suggestedAfterBoard, suggestedMove);
            suggestedAfterFen = detBoardFen(suggestedAfterBoard);
        }
        const QString movePrompt = promptForMove(fen,
                                                 legal,
                                                 ply,
                                                 lookaheadLevel,
                                                 includeLegalMoveList,
                                                 clueMode,
                                                 suggestedMove,
                                                 suggestedAfterFen);
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
            writeLog(3, QString("%1 P%2 mvreq warm=%3/%4 mdl=%5 TOs=%6 stk=%7\n")
                .arg(boardLabel)
                .arg(ply, 3, 10, QChar('0'))
                .arg(call)
                .arg(dummyMoveCalls)
                .arg(activeModel)
                .arg(callTimeout)
                .arg(activeStackModule));
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
            writeLog(3, QString("%1 P%2 mvreq try=%3/%4 mdl=%5 TOs=%6 stk=%7\n")
                .arg(boardLabel)
                .arg(ply, 3, 10, QChar('0'))
                .arg(call)
                .arg(maxFirstWhiteAttempts)
                .arg(activeModel)
                .arg(callTimeout)
                .arg(activeStackModule));
            response = askStackModule(
                activeStackModule,
                activeModel,
                movePrompt,
                callTimeout,
                currentOutputTokens,
                startupLabel);
            moveCallsAttempted = call;
            if (response.status == "timed_out" && call < maxFirstWhiteAttempts &&
                gameTimer.elapsed() < qint64(gameTimeoutSeconds) * 1000) {
                continue;
            }
            if (response.status == "timed_out" || gameTimer.elapsed() >= qint64(gameTimeoutSeconds) * 1000) {
                break;
            }
            const QString moveLabel = QString("%1 P%2 move").arg(boardLabel).arg(ply, 3, 10, QChar('0'));
            logAgentOutput(moveLabel, response);
            parse = parseViaHarness(moveLabel, "agent", "rules", movePrompt, response.stdoutText, legal);
            parsed = parse.selected;
            const QString transitionMove = moveFromReportedFenTransition(brdst,
                                                                          legal,
                                                                          fieldValue(response.stdoutText, "bm"),
                                                                          fieldValue(response.stdoutText, "am"),
                                                                          nullptr);
            if (!transitionMove.isEmpty()) {
                parsed = transitionMove;
            }
            const bool directUciMove = clueMode >= 6 && !parsed.isEmpty() && legal.contains(parsed);
            legalMove = (!transitionMove.isEmpty() && legal.contains(parsed)) || directUciMove;
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
                    parse = parseViaHarness(moveTuneLabel + " retry", "agent", "rules", movePrompt, response.stdoutText, legal);
                    parsed = parse.selected;
                    const QString retryTransitionMove = moveFromReportedFenTransition(brdst,
                                                                                      legal,
                                                                                      fieldValue(response.stdoutText, "bm"),
                                                                                      fieldValue(response.stdoutText, "am"),
                                                                                      nullptr);
                    if (!retryTransitionMove.isEmpty()) {
                        parsed = retryTransitionMove;
                    }
                    const bool retryDirectUciMove = clueMode >= 6 && !parsed.isEmpty() && legal.contains(parsed);
                    legalMove = (!retryTransitionMove.isEmpty() && legal.contains(parsed)) || retryDirectUciMove;
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
        if (!moveTimedOut && !moveRequestFailed && !originalLegalMove && originalResponse.status == "completed") {
            if (originalParsed.isEmpty()) {
                irrelevantAgentReturnTotal += 1;
            } else {
                countRejectedMove(originalParsed, originalLegalMove, &illegalMoveTotal, &invalidMoveTotal);
            }
        } else if (!moveTimedOut && !moveRequestFailed && !originalLegalMove) {
            countRejectedMove(originalParsed, originalLegalMove, &illegalMoveTotal, &invalidMoveTotal);
        }
        const int originalIllegalMoveTotal = illegalMoveTotal;
        const int originalInvalidMoveTotal = invalidMoveTotal;
        const int originalIrrelevantAgentReturnTotal = irrelevantAgentReturnTotal;
        QJsonArray correctionAttempts;
        int correctionAttemptsUsed = 0;
        for (int retry = 1;
             !moveTimedOut && !moveRequestFailed && !legalMove && (illegalMoveTotal + invalidMoveTotal + irrelevantAgentReturnTotal) < correctionRetries;
             ++retry) {
            const int callRemainingSeconds = qMax(1, gameTimeoutSeconds - int(gameTimer.elapsed() / 1000.0));
            const int callTimeout = qMin(stackTimeoutSeconds, callRemainingSeconds);
            const QString rejectedBeforeCorrection = parsed;
            const QString correctionPrompt = movePrompt;
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
                                                  "rules",
                                                  correctionPrompt,
                                                  correctionResponse.stdoutText,
                                                  legal);
                correctionParsed = correctionParse.selected;
                const QString correctionTransitionMove = moveFromReportedFenTransition(brdst,
                                                                                       legal,
                                                                                       fieldValue(correctionResponse.stdoutText, "bm"),
                                                                                       fieldValue(correctionResponse.stdoutText, "am"),
                                                                                       nullptr);
                if (!correctionTransitionMove.isEmpty()) {
                    correctionParsed = correctionTransitionMove;
                }
                const bool correctionDirectUciMove = clueMode >= 6 && !correctionParsed.isEmpty() && legal.contains(correctionParsed);
                correctionLegal = (!correctionTransitionMove.isEmpty() && legal.contains(correctionParsed)) || correctionDirectUciMove;
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
                                                          "rules",
                                                          correctionPrompt,
                                                          correctionResponse.stdoutText,
                                                          legal);
                        correctionParsed = correctionParse.selected;
                        const QString correctionRetryTransitionMove = moveFromReportedFenTransition(brdst,
                                                                                                    legal,
                                                                                                    fieldValue(correctionResponse.stdoutText, "bm"),
                                                                                                    fieldValue(correctionResponse.stdoutText, "am"),
                                                                                                    nullptr);
                        if (!correctionRetryTransitionMove.isEmpty()) {
                            correctionParsed = correctionRetryTransitionMove;
                        }
                        const bool correctionRetryDirectUciMove = clueMode >= 6 && !correctionParsed.isEmpty() && legal.contains(correctionParsed);
                        correctionLegal = (!correctionRetryTransitionMove.isEmpty() && legal.contains(correctionParsed)) || correctionRetryDirectUciMove;
                    }
                } else {
                    currentOutputTokens = tunedOutputTokens;
                }
                if (!correctionLegal && correctionResponse.status == "completed") {
                    if (correctionParsed.isEmpty()) {
                        irrelevantAgentReturnTotal += 1;
                    } else {
                        countRejectedMove(correctionParsed, correctionLegal, &illegalMoveTotal, &invalidMoveTotal);
                    }
                } else if (!correctionLegal) {
                    countRejectedMove(correctionParsed, correctionLegal, &illegalMoveTotal, &invalidMoveTotal);
                }
            } else if (gameTimer.elapsed() >= qint64(gameTimeoutSeconds) * 1000) {
                correctionResponse.status = "game_timeout";
                correctionResponse.stderrText = "response discarded because game timeout elapsed";
            }

            QJsonObject correctionObj;
            correctionObj["retry"] = retry;
            correctionObj["retry_policy"] = "repeat_previous_prompt_exactly";
            correctionObj["correction_prompt"] = correctionPrompt;
            correctionObj["response"] = ollamaJson(correctionResponse);
            correctionObj["move_parse"] = moveParseJson(correctionParse,
                                                        "correction",
                                                        correctionPrompt,
                                                        rejectedBeforeCorrection);
            correctionObj["parsed_uci"] = correctionParsed;
            correctionObj["legal_by_rules"] = correctionLegal;
            if (!correctionLegal && correctionResponse.status == "completed") {
                correctionObj["aih_event"] = correctionParsed.isEmpty()
                    ? "irrelevant_agent_return"
                    : "agent_response_with_illegal_candidate_move";
                correctionObj["referee_relevance"] = correctionParsed.isEmpty() ? "irrelevant" : "relevant";
                correctionObj["aih_event_counts_as_referee_invalid_attempt"] = !correctionParsed.isEmpty();
            }
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
                vote["rules_ground_truth_legal"] = legalMove;
                vote["move"] = parsed;
                if (harnessOnlyReferee) {
                    vote["stack_id"] = "rules_dtrm_chess_v2";
                    vote["referee_backend"] = "rules_dtrm_chess_v2";
                    vote["legal"] = legalMove;
                    vote["parsed_referee_vote"] = true;
                    vote["agrees_with_rules"] = true;
                    vote["reason"] = legalMove ? "move_in_rules_legal_set" : "missing_or_illegal_rules_move";
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
                    vote["agrees_with_rules"] = parsedRefereeVote && agentSaysLegal == legalMove;
                    if (!parsedRefereeVote) {
                        refereeUnparseableVotes += 1;
                        refereeInvalidVotes += 1;
                    } else if (agentSaysLegal) {
                        refereeValidVotes += 1;
                    } else {
                        refereeInvalidVotes += 1;
                    }
                    vote["reason"] = parsedRefereeVote
                        ? "agentic_referee_vote_compared_to_rules_ground_truth"
                        : "unparseable_agentic_referee_vote";
                }
                const QString refereeVoteText = vote.value("parsed_referee_vote").toBool()
                    ? (vote.value("legal").toBool() ? "legal" : "illegal")
                    : QString("unparseable");
                if (!harnessOnlyReferee) {
                    writeLog(3, QString("rf: \"%1\"\n").arg(rawResponseLine(refereeVoteText)));
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
        event["original_legal_by_rules"] = originalLegalMove;
        event["correction_rejection_limit"] = correctionRetries;
        event["correction_retries_used"] = correctionAttemptsUsed;
        event["correction_attempts"] = correctionAttempts;
        event["illegal_move_total"] = illegalMoveTotal;
        event["invalid_move_total"] = invalidMoveTotal;
        event["irrelevant_agent_return_total"] = irrelevantAgentReturnTotal;
        event["rejected_move_total"] = illegalMoveTotal + invalidMoveTotal + irrelevantAgentReturnTotal;
        if (!originalLegalMove && originalResponse.status == "completed") {
            event["aih_event"] = originalParsed.isEmpty()
                ? "irrelevant_agent_return"
                : "agent_response_with_illegal_candidate_move";
            event["referee_relevance"] = originalParsed.isEmpty() ? "irrelevant" : "relevant";
            event["aih_event_counts_as_referee_invalid_attempt"] = !originalParsed.isEmpty();
        }
        event["response"] = ollamaJson(response);
        event["response_failure_class"] = responseFailureClass(response);
        event["final_move_parse"] = moveParseJson(parse, "final", finalPrompt);
        event["final_candidate_detail"] = candidateDetailJson(parse, legal);
        event["parsed_uci"] = parsed;
        event["transport_failure"] = moveRequestFailed;
        const bool refereeMajorityValid = refereeValidVotes > refereeInvalidVotes;
        event["legal_by_rules"] = legalMove;
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
        const QString countsText = errorCountText(illegalMoveTotal, invalidMoveTotal, irrelevantAgentReturnTotal);
        const QString originalCountsText = errorCountText(originalIllegalMoveTotal, originalInvalidMoveTotal, originalIrrelevantAgentReturnTotal);
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
            logHarnessRoute(plyPrefix, "log", "mv_TO");
            writeLog(3, "mv:\"TO\"\nrf:\"inv\"\n");
            writeLog(3, QString("%1:TO -f  DT:%2 ET:%3%4\n")
                .arg(plyPrefix, dtText, etText, countsText));
        } else if (moveRequestFailed) {
            logHarnessRoute(plyPrefix, "log", "xpt_fail");
            writeLog(3, QString("mv:\"req_fail %1\"\nrf:\"inv\"\n").arg(rawResponseLine(response.status)));
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
            writeLog(3, QString("mv: \"%1\"\nrf: \"%2\"\n")
                .arg(rawResponseLine(parsed.isEmpty() ? QString("none") : parsed))
                .arg(rawResponseLine(refereeSummary)));
        }
        if (moveTimedOut) {
            const QString nonresponseClass = response.status == "game_timeout"
                ? QString("game_timeout")
                : nonresponseFailureClass(activeStackModule);
            event["error"] = response.status == "game_timeout"
                ? "game_timeout_move_discarded"
                : "move_timeout_forfeit";
            event["nonresponse_failure_class"] = nonresponseClass;
            event["nonresponse_attempts"] = moveCallsAttempted;
            events.append(event);
            if (response.status == "game_timeout") {
                termination = "game_timeout";
                gameResult = "draw";
            } else if (nonresponseClass == "agt_noresp_stk_ok") {
                termination = side + "_forfeit_agt_noresp";
                gameResult = whiteToMove ? "black_win" : "white_win";
            } else if (nonresponseClass == "stk_noresp") {
                termination = side + "_forfeit_stk_noresp";
                gameResult = whiteToMove ? "black_win" : "white_win";
            } else {
                termination = side + "_forfeit_move_timeout";
                gameResult = whiteToMove ? "black_win" : "white_win";
            }
            break;
        }
        if (moveRequestFailed) {
            event["error"] = "move_request_transport_failure";
            const QString failureClass = responseFailureClass(response);
            event["error_failure_class"] = failureClass;
            events.append(event);
            if (failureClass == "missing_provider_key" ||
                failureClass == "cloud_rate_limit_or_quota_throttle" ||
                failureClass == "cloud_authorization_or_entitlement_failure" ||
                failureClass == "suspected_remote_disablement_or_stack_availability") {
                termination = side + "_run_invalidated_" + failureClass;
                gameResult = "invalid_run";
            } else {
                termination = side + "_forfeit_transport_failure";
                gameResult = whiteToMove ? "black_win" : "white_win";
            }
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
            if (parsed.isEmpty()) {
                event["error"] = "irrelevant_agent_return_hallucination";
            } else if (refereeMajorityValid && !legalMove) {
                event["error"] = "referee_majority_accepted_illegal_move_hallucination";
            } else if (!refereeMajorityValid && legalMove) {
                event["error"] = "referee_majority_rejected_legal_move_hallucination";
            } else {
                event["error"] = "illegal_move_hallucination";
            }
        }
        events.append(event);

        if (irrelevantAgentReturnTotal >= correctionRetries && illegalMoveTotal + invalidMoveTotal == 0) {
            termination = side + "_forfeit_irrelevant_agent_return";
            gameResult = whiteToMove ? "black_win" : "white_win";
            break;
        }
        if (illegalMoveTotal + invalidMoveTotal >= maxIllegal) {
            termination = side + "_forfeit_invalid_move";
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
    obj["aih_result_mode"] = aihResultMode(gameResult, termination);
    obj["terminal_state_reached"] = true;
    obj["completed_game"] = termination.endsWith("checkmate") || termination == "stalemate";
    obj["plies_played"] = moves.size();
    obj["events_recorded"] = events.size();
    obj["legal_moves_played"] = moves.size();
    obj["illegal_or_unparseable_count"] = illegalCount;
    obj["illegal_move_total"] = illegalMoveTotal;
    obj["invalid_move_total"] = invalidMoveTotal;
    obj["irrelevant_agent_return_total"] = irrelevantAgentReturnTotal;
    obj["rejected_move_total"] = illegalMoveTotal + invalidMoveTotal + irrelevantAgentReturnTotal;
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
        .arg(finalCountText(illegalMoveTotal, invalidMoveTotal, irrelevantAgentReturnTotal, correctionRetries)));
    return obj;
}

static QString reportModelCode(QString value) {
    value.replace("gemini:gemini-", "gmn-");
    value.replace("openai:", "oai-");
    value.replace("anthropic:", "ant-");
    value.replace(" vs ", " v ");
    value.replace(":latest", "");
    return value;
}

static QString reportModeCode(const QString &value) {
    if (value == "aichess_hallucination_game") return "AIH";
    if (value == "aichess_agent_only") return "aao";
    if (value == "game") return "gam";
    if (value == "one_move") return "one";
    return value;
}

static QString reportResultCode(const QString &value) {
    if (value == "aih_failure_cloud_rate_limit_or_quota_throttle") return "fail.qta";
    if (value == "aih_failure_run_invalidated") return "fail.inv";
    if (value == "aih_failure_agent_stopped_playing_chess") return "fail.stp";
    if (value == "aih_tournament_game_played") return "ok.ply";
    if (value == "aih_result_unclassified") return "unk";
    return value;
}

static QString reportTerminationCode(QString value) {
    value.replace("white_", "w.");
    value.replace("black_", "b.");
    value.replace("run_invalidated_", "inv.");
    value.replace("cloud_rate_limit_or_quota_throttle", "qta");
    value.replace("cloud_authorization_or_entitlement_failure", "auth");
    value.replace("suspected_remote_disablement_or_stack_availability", "rem");
    value.replace("missing_provider_key", "key");
    value.replace("forfeit_move_timeout", "fto");
    value.replace("forfeit_transport_failure", "ftr");
    value.replace("forfeit_agt_noresp", "fft/agt/nrsp");
    value.replace("forfeit_stk_noresp", "fft/stk/nrsp");
    value.replace("forfeit_irrelevant_agent_return", "fir");
    value.replace("forfeit_invalid_move", "fim");
    value.replace("forfeit_invalid_or_unparseable_move", "fim");
    value.replace("draw_by_configured_ply_limit", "dpl");
    value.replace("game_timeout", "gto");
    value.replace("game_completed", "gcm");
    return value;
}

struct BracketEntrant {
    QString model;
    QString requestedModel;
    int seed = 0;
};

struct RankPlacement {
    int rank = 0;
    int level = 0;
    QString side;
};

static QString winnerSideForBoardResult(const QJsonObject &result) {
    const QString gameResult = result.value("game_result").toString();
    const QString termination = result.value("termination").toString();
    if (gameResult == "white_win" || termination.startsWith("black_")) {
        return "white";
    }
    if (gameResult == "black_win" || termination.startsWith("white_")) {
        return "black";
    }
    const int rejected = result.value("rejected_move_total").toInt();
    const int irrelevant = result.value("irrelevant_agent_return_total").toInt();
    const int illegal = result.value("illegal_or_unparseable_count").toInt();
    Q_UNUSED(rejected);
    Q_UNUSED(irrelevant);
    Q_UNUSED(illegal);
    return "white";
}

static void applyFinalRanks(QList<QJsonObject> &results,
                            const QMap<QString, int> &rankByModel) {
    QMap<QString, RankPlacement> placementByModel;
    for (const QJsonObject &result : results) {
        const int level = result.value("tournament_level").toInt();
        const QString whiteModel = result.value("white_model").toString();
        const QString blackModel = result.value("black_model").toString();
        if (!whiteModel.isEmpty() && level >= placementByModel.value(whiteModel).level) {
            placementByModel[whiteModel] = {rankByModel.value(whiteModel, 0), level, "white"};
        }
        if (!blackModel.isEmpty() && level >= placementByModel.value(blackModel).level) {
            placementByModel[blackModel] = {rankByModel.value(blackModel, 0), level, "black"};
        }
    }
    auto placementText = [](const RankPlacement &placement) {
        if (placement.rank <= 0) {
            return QString("FR#-- L-- --");
        }
        return QString("FR#%1 L%2 %3")
            .arg(placement.rank, 2, 10, QChar('0'))
            .arg(placement.level, 2, 10, QChar('0'))
            .arg(placement.side);
    };
    for (QJsonObject &result : results) {
        const QString whiteModel = result.value("white_model").toString();
        const QString blackModel = result.value("black_model").toString();
        const int whiteRank = rankByModel.value(whiteModel, 0);
        const int blackRank = rankByModel.value(blackModel, 0);
        result["white_final_rank"] = whiteRank;
        result["black_final_rank"] = blackRank;
        result["white_final_placement"] = placementText(placementByModel.value(whiteModel));
        result["black_final_placement"] = placementText(placementByModel.value(blackModel));
        result["final_rank"] = QString("W %1 / B %2")
            .arg(result.value("white_final_placement").toString(),
                 result.value("black_final_placement").toString());
    }
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
    out << "curDateTime: " << QDateTime::currentDateTime().toString(Qt::ISODate) << "\n\n";
    bool singleMode = !results.isEmpty();
    QString sharedMode = results.isEmpty() ? QString() : reportModeCode(results.first().value("mode").toString());
    for (const QJsonObject &r : results) {
        if (reportModeCode(r.value("mode").toString()) != sharedMode) {
            singleMode = false;
            break;
        }
    }
    if (singleMode) {
        out << "GameMode: " << sharedMode << "\n\n";
    }
    out << "| Model | Rank | Res | Term | Cmplt | Plies | Legal | Fail | Ntrlv | Rej | Sec |\n";
    out << "| --- | --- | --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |\n";
    QList<QJsonObject> sortedResults = results;
    std::sort(sortedResults.begin(), sortedResults.end(), [](const QJsonObject &a, const QJsonObject &b) {
        const int arw = a.value("white_final_rank").toInt(999999);
        const int arb = a.value("black_final_rank").toInt(999999);
        const int brw = b.value("white_final_rank").toInt(999999);
        const int brb = b.value("black_final_rank").toInt(999999);
        const int ar = qMin(arw, arb);
        const int br = qMin(brw, brb);
        if (ar != br) {
            return ar < br;
        }
        const int atr = a.value("tournament_round").toInt(999999);
        const int btr = b.value("tournament_round").toInt(999999);
        if (atr != btr) {
            return atr < btr;
        }
        return a.value("model").toString() < b.value("model").toString();
    });
    for (const QJsonObject &r : sortedResults) {
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
        const int rejected = r.contains("rejected_move_total")
            ? r.value("rejected_move_total").toInt()
            : illegal;
        const int irrelevant = r.value("irrelevant_agent_return_total").toInt();
        out << "| " << reportModelCode(r.value("model").toString())
            << " | " << r.value("final_rank").toString("-")
            << " | " << reportResultCode(r.value("aih_result_mode").toString())
            << " | " << reportTerminationCode(r.value("termination").toString())
            << " | " << (r.value("completed_game").toBool() ? "yes" : "no")
            << " | " << plies
            << " | " << legal
            << " | " << illegal
            << " | " << irrelevant
            << " | " << rejected
            << " | " << QString::number(elapsed, 'f', 3)
            << " |\n";
    }
    summary.close();

    if (logEnabled(1)) {
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
    QCommandLineOption refereeOpt(QStringList{"r", "referee"}, "Referee model(s), e.g. rules, qwen1, or qwen1:qwen4", "models");
    QCommandLineOption boardsOpt("boards", "Number of boards to run in AIChess hallucination game mode", "count", "1");
    QCommandLineOption loopsOpt("loops", "Number of board batches to run in AIChess hallucination game mode", "count", "1");
    QCommandLineOption refereeCountOpt("referee-count", "Referee votes recorded per board", "count", "1");
    QCommandLineOption tournamentBracketOpt("tournament-bracket", "Run AIH promotion bracket: winners advance, odd entrant gets a bye");
    QCommandLineOption moveTimeoutOpt(QStringList{"t", "move-timeout"}, "Per-move response window seconds", "seconds", "10");
    QCommandLineOption stackTimeoutOpt(QStringList{"sto", "stack-timeout"}, "Agent stack response completion timeout seconds", "seconds", "60");
    QCommandLineOption outputTokensOpt(QStringList{"otkns"}, "Agent output token budget", "tokens", "256");
    QCommandLineOption autoOutputTokensOpt(QStringList{"aot", "auto-output-tokens"}, "Enable automatic output-token tuning");
    QCommandLineOption boardAwarenessProbeOpt(QStringList{"bap", "board-awareness-probe"}, "After each move reply, ask the agent to report the board after its move and score it");
    QCommandLineOption legalListOpt("legal-list", "Assisted mode: include hidden legal UCI move list in the player prompt. Default is board-transition only.");
    QCommandLineOption referenceConfigOpt("reference-config", "Reference configuration identifier for this runner/stack run", "id", "aichess_ref_runner_ollama_agentic_v1_20260716");
    QCommandLineOption stackModuleOpt("stack-module", "Runner stack adapter module selected for agent I/O", "module", "auto");
    QCommandLineOption stackKindOpt("stack-kind", "Stack category interacting with the runner", "kind", "ollama_agentic_local");
    QCommandLineOption stackNameOpt("stack-name", "Human-readable stack name interacting with the runner", "name", "ollama_generate_qwen_local");
    QCommandLineOption gameTimeoutOpt(QStringList{"gmto"}, "Per-game timeout seconds", "seconds", "600");
    QCommandLineOption maxPliesOpt(QStringList{"mxply"}, "Maximum plies per game", "plies", "120");
    QCommandLineOption retriesOpt(QStringList{"cnrtlm"}, "Correction retry limit after illegal/unparseable move", "count", "1");
    QCommandLineOption lookaheadOpt(QStringList{"lkahdlvl", "lokahdlvl"}, "Agent chess look-ahead level requested in the move prompt", "level", "0");
    QCommandLineOption clueModeOpt("clue-mode", "Agent clue mode 0-6. 0=no clue, 1=valid moves, 2=board valid, 3=both, 4=suggested move, 5=expected transition, 6=direct UCI move.", "level", "0");
    QCommandLineOption logLevelOpt("loglvl", "Logging verbosity 0-5. 0 is least verbose; 5 is most verbose. Level 1 prints compact module mi/mo records; level 2 prints module output/input mo/mi plus mv and rf strings; level 3 adds diagnostics.", "level", "3");
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
    parser.addOption(tournamentBracketOpt);
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
    parser.addOption(clueModeOpt);
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
        if (model.trimmed() == "harness" || model.trimmed() == "rules" || model.trimmed() == "stockfish") {
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
    const int clueMode = qBound(0, parser.value(clueModeOpt).toInt(), 6);
    kLogLevel = qBound(0, parser.value(logLevelOpt).toInt(), 5);
    const int maxIllegal = parser.value(maxIllegalOpt).toInt();
    const int boards = qMax(1, parser.value(boardsOpt).toInt());
    const int loops = qMax(1, parser.value(loopsOpt).toInt());
    const int refereeCount = qMax(1, parser.value(refereeCountOpt).toInt());
    const bool tournamentBracket = parser.isSet(tournamentBracketOpt);
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
    } else if (isGeminiStackModule(stackModule)) {
        if (!parser.isSet(stackKindOpt)) {
            stackKind = "google_agentic_cloud";
        }
        if (!parser.isSet(stackNameOpt)) {
            stackName = "gemini_cli_cloud";
        }
    } else if (isAnthropicStackModule(stackModule)) {
        if (!parser.isSet(stackKindOpt)) {
            stackKind = "anthropic_agentic_cloud";
        }
        if (!parser.isSet(stackNameOpt)) {
            stackName = "anthropic_messages_cloud";
        }
    } else if (isCodexStackModule(stackModule)) {
        if (!parser.isSet(stackKindOpt)) {
            stackKind = "openai_codex_local_cli";
        }
        if (!parser.isSet(stackNameOpt)) {
            stackName = "codex_cli_local";
        }
    } else if (isAutoStackModule(stackModule) && hasAnyOpenAiAgent(whiteModels + blackModels + refereeModels)) {
        if (!parser.isSet(stackKindOpt)) {
            stackKind = "openai_agentic_cloud";
        }
        if (!parser.isSet(stackNameOpt)) {
            stackName = "openai_responses_cloud";
        }
    } else if (isAutoStackModule(stackModule) && hasAnyGeminiAgent(whiteModels + blackModels + refereeModels)) {
        if (!parser.isSet(stackKindOpt)) {
            stackKind = "google_agentic_cloud";
        }
        if (!parser.isSet(stackNameOpt)) {
            stackName = "gemini_cli_cloud";
        }
    } else if (isAutoStackModule(stackModule) && hasAnyAnthropicAgent(whiteModels + blackModels + refereeModels)) {
        if (!parser.isSet(stackKindOpt)) {
            stackKind = "anthropic_agentic_cloud";
        }
        if (!parser.isSet(stackNameOpt)) {
            stackName = "anthropic_messages_cloud";
        }
    } else if (isAutoStackModule(stackModule) && hasAnyCodexAgent(whiteModels + blackModels + refereeModels)) {
        if (!parser.isSet(stackKindOpt)) {
            stackKind = "openai_codex_local_cli";
        }
        if (!parser.isSet(stackNameOpt)) {
            stackName = "codex_cli_local";
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
        obj["reasoning_performance_mode"] = reasoningPerformanceMode();
        obj["verbosity"] = openAiTextVerbosity();
        obj["openai_reasoning_effort_effective"] = openAiReasoningEffort();
        obj["openai_text_verbosity"] = openAiTextVerbosity();
        obj["anthropic_reasoning_effort_effective"] = anthropicEffortForReasoningMode(reasoningPerformanceMode());
        obj["gemini_thinking_level_effective"] = geminiThinkingLevelForReasoningMode(reasoningPerformanceMode());
        obj["hrnrol"] = "controlling_reference";
        obj["boards"] = boards;
        obj["loops"] = loops;
        obj["referee_count"] = refereeCount;
        obj["validation_mode"] = agentOnlyNoStockfish ? "ans" : "rules_dtrm_chess_v2";
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
        obj["clue_mode"] = clueMode;
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
            assignment["reasoning_performance_mode"] = reasoningPerformanceMode();
            assignment["verbosity"] = openAiTextVerbosity();
            assignment["openai_reasoning_effort_effective"] = openAiReasoningEffort();
            assignment["openai_text_verbosity"] = openAiTextVerbosity();
            assignment["anthropic_reasoning_effort_effective"] = anthropicEffortForReasoningMode(reasoningPerformanceMode());
            assignment["gemini_thinking_level_effective"] = geminiThinkingLevelForReasoningMode(reasoningPerformanceMode());
            assignment["black_role"] = QString("%1_black_agent_1").arg(boardId);
            assignment["black_requested"] = selectModelForBoard(requestedBlackModels, board);
            assignment["black_model"] = selectModelForBoard(blackModels, board);
            assignment["black_stkmdl"] = stackModuleForAgentModel(stackModule, assignment.value("black_model").toString());
            assignment["black_stkmdl_caps"] = stackModuleCapabilities(assignment.value("black_stkmdl").toString());
            const bool harnessOnly = refereeModels.size() == 1 && refereeModels.first() == "harness";
            assignment["referee_mode"] = harnessOnly
                ? "rules_dtrm_chess_v2"
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
                    refereeAssignment["stkmdl"] = "rules_reference";
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
        auto runBoardMatch = [&](int loop, int board, const QString &whiteModel, const QString &blackModel,
                                 const QString &requestedWhiteModel, const QString &requestedBlackModel) {
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
                               clueMode,
                               autoOutputTokens,
                               boardAwarenessProbe,
                               includeLegalMoveList,
                               referenceConfigId,
                               stackModule,
                               stackKind,
                               stackName);
            result["loop_index"] = loop + 1;
            result["loop_count"] = loops;
            result["white_model"] = whiteModel;
            result["black_model"] = blackModel;
            return result;
        };

        if (tournamentBracket) {
            QList<BracketEntrant> entrants;
            const int openingBoards = qMax(1, boards);
            for (int board = 0; board < openingBoards; ++board) {
                entrants.append({selectModelForBoard(whiteModels, board), selectModelForBoard(requestedWhiteModels, board), board * 2 + 1});
                entrants.append({selectModelForBoard(blackModels, board), selectModelForBoard(requestedBlackModels, board), board * 2 + 2});
            }

            QList<QList<QString>> eliminatedByRound;
            int round = 1;
            while (entrants.size() > 1) {
                QList<BracketEntrant> nextEntrants;
                QList<QString> eliminatedThisRound;
                int matchIndex = 0;
                for (int i = 0; i < entrants.size(); i += 2) {
                    if (i + 1 >= entrants.size()) {
                        nextEntrants.append(entrants.at(i));
                        continue;
                    }
                    const BracketEntrant white = entrants.at(i);
                    const BracketEntrant black = entrants.at(i + 1);
                    QJsonObject result = runBoardMatch(0, matchIndex, white.model, black.model, white.requestedModel, black.requestedModel);
                    const QString winnerSide = winnerSideForBoardResult(result);
                    const BracketEntrant winner = winnerSide == "black" ? black : white;
                    const BracketEntrant loser = winnerSide == "black" ? white : black;
                    result["tournament_round"] = round;
                    result["tournament_level"] = round;
                    result["tournament_board"] = matchIndex + 1;
                    result["white_seed"] = white.seed;
                    result["black_seed"] = black.seed;
                    result["winner_side"] = winnerSide;
                    result["winner_model"] = winner.model;
                    result["eliminated_model"] = loser.model;
                    results.append(result);
                    nextEntrants.append(winner);
                    eliminatedThisRound.append(loser.model);
                    ++matchIndex;
                }
                eliminatedByRound.append(eliminatedThisRound);
                entrants = nextEntrants;
                ++round;
            }

            QMap<QString, int> rankByModel;
            int rank = 1;
            if (!entrants.isEmpty()) {
                rankByModel[entrants.first().model] = rank++;
            }
            for (int i = eliminatedByRound.size() - 1; i >= 0; --i) {
                for (const QString &model : eliminatedByRound.at(i)) {
                    if (!rankByModel.contains(model)) {
                        rankByModel[model] = rank++;
                    }
                }
            }
            applyFinalRanks(results, rankByModel);
	        } else {
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
	                     clueMode,
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
	                                           clueMode,
	                                           autoOutputTokens,
	                                           boardAwarenessProbe,
	                                           includeLegalMoveList,
	                                           referenceConfigId,
	                                           stackModule,
	                                           stackKind,
	                                           stackName);
	                        result["loop_index"] = loop + 1;
	                        result["loop_count"] = loops;
	                        result["white_model"] = whiteModel;
	                        result["black_model"] = blackModel;
	                        return result;
	                    }));
	            }
	            for (std::future<QJsonObject> &future : boardFutures) {
	                results.append(future.get());
	            }
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
