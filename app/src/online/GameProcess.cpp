#include "GameProcess.h"

#include "../GameSession.h"
#include "AppPaths.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QPointer>
#include <QTimer>

#include <algorithm>

namespace {
QPointer<GameSession> g_session;

// the newest sessions' folders stay (a desync or crash report reads their game.log); older ones go
void PruneSessions(const QString& dir, int keep)
{
    QDir d(dir);
    const QFileInfoList all = d.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Time);
    for (int i = keep; i < all.size(); i++)
        QDir(all[i].absoluteFilePath()).removeRecursively();
}
}  // namespace

void GameProcess::setSession(GameSession* session) { g_session = session; }

GameProcess::GameProcess(const QString& gameId, int instance, QObject* parent)
    : QObject(parent), gameId_(gameId), instance_(instance)
{
}

GameProcess::~GameProcess()
{
    if (g_session && link_)
        g_session->dropLink(link_);
}

qint64 GameProcess::pid() const { return running_ ? QCoreApplication::applicationPid() : 0; }

bool GameProcess::start(const QString&, QString* error, bool)
{
    if (!g_session) {
        if (error)
            *error = tr("The game is not available.");
        return false;
    }
    const QString base = paths::sessionsDir();
    PruneSessions(base, 20);
    sessionDir_ = QDir(base).filePath(QStringLiteral("%1-%2-%3")
                                          .arg(gameId_, QDateTime::currentDateTime().toString("yyyyMMdd-hhmmss"))
                                          .arg(instance_));
    QDir().mkpath(sessionDir_);
    connect(g_session, &GameSession::linkStatus, this, &GameProcess::onSessionStatus, Qt::UniqueConnection);
    running_ = true;
    setStatus(QStringLiteral("booting"));
    setStatus(QStringLiteral("ready"));
    return true;
}

void GameProcess::quit()
{
    if (!running_)
        return;
    if (g_session && link_)
        g_session->dropLink(link_);
    link_ = 0;
    running_ = false;
    exitLater();
}

void GameProcess::exitLater()
{
    QTimer::singleShot(0, this, [this] { emit exited(0, false); });
}

QHash<QString, QString> GameProcess::parseKeys(const QString& line)
{
    QHash<QString, QString> keys;
    const QStringList tokens = line.split(QLatin1Char(' '), Qt::SkipEmptyParts);
    for (int i = 1; i < tokens.size(); i++) {
        const int eq = tokens[i].indexOf(QLatin1Char('='));
        if (eq > 0)
            keys.insert(tokens[i].left(eq), tokens[i].mid(eq + 1));
    }
    return keys;
}

void GameProcess::command(const QString& line)
{
    if (!running_)
        return;
    const QString verb = line.section(QLatin1Char(' '), 0, 0);
    const QHash<QString, QString> keys = parseKeys(line);
    applog::write(QStringLiteral("game %1: %2").arg(gameId_, line));
    if (verb == QLatin1String("start")) {
        if (keys.value(QStringLiteral("watch")) == QLatin1String("1")) {
            setStatus(QStringLiteral("error watching a match is not supported"));
            return;
        }
        const QString match = keys.value(QStringLiteral("match"));
        if (link_ && match == linkMatch_)
            return;  // the match it already took (a start sent again)
        if (!isReady()) {
            setStatus(QStringLiteral("error not at the hold point"));
            return;
        }
        LinkStart s;
        s.set = keys.value(QStringLiteral("game"), gameId_);
        s.mode = keys.value(QStringLiteral("mode"), QStringLiteral("kombat"));
        s.side = keys.value(QStringLiteral("side")).toInt() == 2 ? 2 : 1;
        s.localPort = keys.value(QStringLiteral("local_port")).toInt();
        s.peer = keys.value(QStringLiteral("remote"));
        s.delay = keys.value(QStringLiteral("delay")).toInt();
        s.match = match;
        s.sessionDir = sessionDir_;
        s.p1Name = keys.value(QStringLiteral("p1_name"));
        s.p2Name = keys.value(QStringLiteral("p2_name"));
        if (s.localPort <= 0 || s.peer.isEmpty()) {
            setStatus(QStringLiteral("error bad command"));
            return;
        }
        linkMatch_ = match;
        link_ = g_session->playLink(s);
        setStatus(QStringLiteral("starting match=") + match);
    } else if (verb == QLatin1String("park")) {
        if (link_) {
            g_session->parkLink(link_);  // "ready" comes back once the match's end has played
        } else {
            setStatus(QStringLiteral("ready"));
        }
    } else if (verb == QLatin1String("quit")) {
        quit();
    } else {
        setStatus(QStringLiteral("error bad command"));
    }
}

void GameProcess::onSessionStatus(const QString& line, quint64 link)
{
    if (!running_ || link == 0 || link != link_)
        return;
    if (line == QLatin1String("error exited")) {
        // its player left the match (the pause menu): to the launcher, the game closed
        link_ = 0;
        setStatus(line);
        running_ = false;
        exitLater();
        return;
    }
    if (line == QLatin1String("ready"))
        link_ = 0;  // parked: back at the hold point
    setStatus(line);
}

void GameProcess::setStatus(const QString& line)
{
    QTimer::singleShot(0, this, [this, line] {
        // a park while already at the hold point says "ready" again: MatchController waits for it to finish a match
        if (line == status_ && line != QLatin1String("ready"))
            return;
        status_ = line;
        emit statusChanged(line);
    });
}
