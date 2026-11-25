#pragma once
// What the menus see of online play (QML's "online"): the connection to the MK3D server on Oracle Cloud and the
// account, Ranked and Kasual searches with their series, lobbies, the leaderboard and profiles. MK3D's ServerClient and
// MatchController do the work; this turns their messages into properties.
#include <QJsonObject>
#include <QObject>
#include <QStringList>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>

class ServerClient;
class MatchController;

class OnlineHub : public QObject {
    Q_OBJECT
    // the connection: "offline", "connecting", "online" (not signed in) or "signedIn"
    Q_PROPERTY(QString state READ state NOTIFY changed)
    Q_PROPERTY(QString account READ account NOTIFY changed)
    Q_PROPERTY(QString lastName READ lastName NOTIFY changed)  // the name signed in with last
    Q_PROPERTY(bool remembered READ remembered NOTIFY changed)  // a sign-in to resume
    // this PC never signed in (no sign-in remembered, no name saved): the first PRESS START asks for an account
    Q_PROPERTY(bool newHere READ newHere NOTIFY changed)
    Q_PROPERTY(bool waiting READ waiting NOTIFY changed)        // a sign-in on its way
    Q_PROPERTY(QString error READ error NOTIFY changed)         // the last thing that went wrong, for the screen showing it
    Q_PROPERTY(QString motd READ motd NOTIFY changed)
    Q_PROPERTY(int onlineCount READ onlineCount NOTIFY countsChanged)
    Q_PROPERTY(QVariantMap gameCounts READ gameCounts NOTIFY countsChanged)  // per game: playing, searching, matches, lobbies
    // matchmaking: MatchController's phase in lower case (idle, searching, found, connecting, starting, playing,
    // reporting, finishing), the search, the match found and its countdown, the series and the last result
    Q_PROPERTY(QString phase READ phase NOTIFY matchChanged)
    Q_PROPERTY(QString phaseText READ phaseText NOTIFY matchChanged)
    Q_PROPERTY(QString searchGame READ searchGame NOTIFY matchChanged)
    Q_PROPERTY(bool searchRanked READ searchRanked NOTIFY matchChanged)
    Q_PROPERTY(int searchSeconds READ searchSeconds NOTIFY tick)
    Q_PROPERTY(QVariantMap found READ found NOTIFY matchChanged)
    Q_PROPERTY(int acceptSeconds READ acceptSeconds NOTIFY tick)
    Q_PROPERTY(QVariantMap series READ series NOTIFY matchChanged)
    Q_PROPERTY(int seriesSeconds READ seriesSeconds NOTIFY tick)
    Q_PROPERTY(QVariantMap result READ result NOTIFY matchChanged)
    // lobbies: the list, and the one this player is in ({} none)
    Q_PROPERTY(QVariantList lobbies READ lobbies NOTIFY lobbiesChanged)
    Q_PROPERTY(QVariantMap lobby READ lobby NOTIFY lobbyChanged)
    Q_PROPERTY(bool lobbyReady READ lobbyReady NOTIFY lobbyChanged)
    // the leaderboard asked for last, and a profile
    Q_PROPERTY(QVariantMap board READ board NOTIFY boardChanged)
    Q_PROPERTY(QVariantMap profile READ profile NOTIFY profileChanged)

public:
    explicit OnlineHub(QObject* parent = nullptr);
    ~OnlineHub() override;

    QString state() const;
    QString account() const;
    QString lastName() const;
    bool remembered() const;
    bool newHere() const;
    bool waiting() const { return waiting_; }
    QString error() const { return error_; }
    QString motd() const;
    int onlineCount() const { return onlineCount_; }
    QVariantMap gameCounts() const { return gameCounts_; }
    QString phase() const;
    QString phaseText() const;
    QString searchGame() const;
    bool searchRanked() const;
    int searchSeconds() const;
    QVariantMap found() const;
    int acceptSeconds() const;
    QVariantMap series() const;
    int seriesSeconds() const;
    QVariantMap result() const { return result_; }
    QVariantList lobbies() const { return lobbies_; }
    QVariantMap lobby() const { return lobby_; }
    bool lobbyReady() const;
    QVariantMap board() const { return board_; }
    QVariantMap profile() const { return profile_; }

    Q_INVOKABLE void connectServer();
    Q_INVOKABLE void signIn(const QString& name, const QString& password, bool remember);
    Q_INVOKABLE void createAccount(const QString& name, const QString& password, bool remember);
    Q_INVOKABLE void signOut();
    Q_INVOKABLE void clearError();

    Q_INVOKABLE bool search(const QString& game, bool ranked);
    Q_INVOKABLE void cancelSearch();
    Q_INVOKABLE void accept();
    Q_INVOKABLE void decline();
    Q_INVOKABLE void rematch();      // the series' next match
    Q_INVOKABLE void leaveSeries();  // a ranked set left after a match counted is lost
    Q_INVOKABLE void clearResult();

    Q_INVOKABLE void refreshLobbies(const QString& game = QString());
    // mode: "kombat" (a versus match) or "sparring" (Online Practice: endless, a friend joins by code)
    Q_INVOKABLE void createLobby(const QString& game, const QString& mode, const QString& password);
    Q_INVOKABLE void joinLobby(const QString& id, const QString& password = QString());
    Q_INVOKABLE void leaveLobby();
    Q_INVOKABLE void setReady(bool ready);
    Q_INVOKABLE void chat(const QString& text);
    // the player left a lobby match (its pause menu): not ready for another until they say so. Online Practice keeps
    // both ready after each match, so its next one would count down by itself; the server takes the change only
    // once the match is over, so it goes out then
    Q_INVOKABLE void standDown();

    Q_INVOKABLE void loadBoard(const QString& game);
    Q_INVOKABLE void loadProfile(const QString& name = QString());

    // tests (--online-test): sign in or create the account, search, accept and rematch by themselves; or, with lobby
    // "host" or "join", make a lobby of mode (kombat, or sparring: Online Practice) or join one (the first of the
    // game, or the code the host writes to codeFile, as a practice room is not listed), get ready and play its matches
    void runTest(const QString& name, const QString& password, const QString& game, bool ranked, int matches,
                 const QString& lobby = QString(), const QString& mode = QStringLiteral("kombat"),
                 const QString& codeFile = QString());

    MatchController* controller() const { return match_; }

signals:
    void changed();
    void countsChanged();
    void matchChanged();
    void tick();
    void lobbiesChanged();
    void lobbyChanged();
    void boardChanged();
    void profileChanged();
    void notice(const QString& text);  // a toast
    void signedIn();
    void matchFound();       // the accept screen, wherever the player is
    void matchOver();        // a match's result came back
    void testFinished(bool ok, const QString& why);

private:
    void onMessage(const QJsonObject& msg);
    void fail(const QString& text);
    static QString errorText(const QString& code, const QString& fallback);
    void send(QJsonObject msg);

    ServerClient* server_;
    MatchController* match_;
    QTimer clock_;
    bool remember_ = true;
    bool waiting_ = false;
    QString error_;
    int onlineCount_ = 0;
    QVariantMap gameCounts_;
    qint64 foundAt_ = 0, seriesAt_ = 0;  // when match_found and the series' state came (ms since epoch)
    QVariantMap result_;
    QVariantMap lastSeries_;  // the series' last state, kept once it is over (its outcome, the Elo it moved)
    QStringList pending_;     // a sign-in asked for while offline: {"login" or "register", name, password}
    QVariantList lobbies_;
    QVariantMap lobby_;
    bool standDown_ = false;  // un-ready once the lobby's match is over (standDown)
    QVariantMap board_;
    QVariantMap profile_;
    // --online-test
    struct Test {
        bool on = false;
        QString name, password, game;
        bool ranked = false;
        int matches = 1, played = 0;
        bool triedRegister = false;
        bool started = false;  // its search or lobby went out (a remembered sign-in and the password both sign in)
        QString lobby;  // "host", "join" or empty (a queue search)
        QString mode = QStringLiteral("kombat");
        QString codeFile;  // the host writes its lobby's code there, the joiner reads it
        bool codeShared = false, joining = false;
    } test_;
    QTimer testPoll_;   // a joining test looks for its lobby
    void testStep();
};
