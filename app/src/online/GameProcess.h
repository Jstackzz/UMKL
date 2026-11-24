#pragma once
// MK3D's launcher runs each game as a process and talks to it through cmd.txt and status.txt (mk3d-rollback
// docs/PROTOCOL.md section 3). Here the game runs inside this app (GameSession), so GameProcess is that conversation
// without a process: a match's `start` line goes to GameSession::playLink, and its status lines come back from it, in
// the protocol's words, so MatchController drives it as it drives a game process. Its hold point is this app's menus:
// "ready" whenever no match of it runs.
#include <QHash>
#include <QObject>
#include <QString>
#include <QStringList>

class GameSession;

class GameProcess : public QObject {
    Q_OBJECT
public:
    GameProcess(const QString& gameId, int instance, QObject* parent = nullptr);
    ~GameProcess() override;

    // the app's game, set once at start-up
    static void setSession(GameSession* session);

    // "boots" to the hold point: booting, then ready
    bool start(const QString& exe, QString* error, bool netMode = true);
    void setEnv(const QString&, const QString&) {}
    void setArgs(const QStringList&) {}
    void quit();  // a match it plays closes; then it has "exited"
    void kill() { quit(); }
    bool isRunning() const { return running_; }
    qint64 pid() const;  // the app's own while running: nothing freezes it (WinUtil.h)

    QString gameId() const { return gameId_; }
    int instance() const { return instance_; }
    QString sessionDir() const { return sessionDir_; }
    QString status() const { return status_; }
    int exitCode() const { return 0; }
    QString state() const { return status_.section(QLatin1Char(' '), 0, 0); }
    bool isReady() const { return state() == QLatin1String("ready"); }

    // start, park, quit (rematch is not used: every match starts from the hold point)
    void command(const QString& line);
    bool commandWaiting() const { return false; }

    static QHash<QString, QString> parseKeys(const QString& line);  // key=value tokens after the verb
    static bool codeStale(const QString&) { return false; }

signals:
    void statusChanged(const QString& line);
    void outputLine(const QString& line);
    void exited(int exitCode, bool crashed);

private:
    // the status as a game process would write it: read on the launcher's next poll, so never inside command()
    void setStatus(const QString& line);
    void onSessionStatus(const QString& line, quint64 link);
    void exitLater();

    QString gameId_;
    int instance_;
    QString sessionDir_;
    QString status_;
    bool running_ = false;
    quint64 link_ = 0;  // GameSession's number for the match this one started (0: none)
    QString linkMatch_;  // and its id
};
