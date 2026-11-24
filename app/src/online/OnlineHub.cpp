#include "OnlineHub.h"

#include "../Settings.h"
#include "AppPaths.h"
#include "MatchController.h"
#include "ServerClient.h"

#include <QDateTime>
#include <QFile>
#include <QJsonArray>
#include <QMetaEnum>

namespace {
Settings* S() { return Settings::instance(); }
qint64 Now() { return QDateTime::currentMSecsSinceEpoch(); }
QString Str(const QJsonObject& o, const char* key) { return o.value(QLatin1String(key)).toString(); }
}  // namespace

OnlineHub::OnlineHub(QObject* parent) : QObject(parent)
{
    server_ = new ServerClient(this);
    match_ = new MatchController(server_, this);

    connect(server_, &ServerClient::stateChanged, this, [this] {
        if (!server_->isOnline())
            waiting_ = false;
        emit changed();
    });
    connect(server_, &ServerClient::message, this, &OnlineHub::onMessage);
    connect(server_, &ServerClient::connectionError, this, [this](const QString& text) { fail(text); });
    connect(server_, &ServerClient::untrustedCertificate, this, [this](const QString& fp) {
        fail(tr("The server's certificate (%1) is not the one this app trusts.")
                 .arg(ServerClient::prettyFingerprint(fp)));
    });
    connect(server_, &ServerClient::loggedIn, this, [this](const QJsonObject& account) {
        waiting_ = false;
        error_.clear();
        S()->setPlayerName(Str(account, "name"));
        S()->setToken(remember_ ? Str(account, "token") : QString());
        send({{QStringLiteral("type"), QStringLiteral("online")}});
        loadProfile();
        emit changed();
        emit signedIn();
        if (test_.on && !test_.started) {
            test_.started = true;
            if (test_.lobby == QLatin1String("host"))
                QTimer::singleShot(500, this, [this] { createLobby(test_.game, test_.mode, QString()); });
            else if (test_.lobby == QLatin1String("join"))
                testPoll_.start(1000);
            else
                QTimer::singleShot(500, this, [this] { search(test_.game, test_.ranked); });
        }
    });
    testPoll_.setInterval(1000);
    connect(&testPoll_, &QTimer::timeout, this, [this] {
        if (!lobby_.isEmpty()) {
            testPoll_.stop();
        } else if (!test_.codeFile.isEmpty()) {
            QFile f(test_.codeFile);
            const QString code = f.open(QIODevice::ReadOnly) ? QString::fromLatin1(f.readAll()).trimmed() : QString();
            if (code.size() == 5 && !test_.joining) {
                test_.joining = true;
                joinLobby(code);
            }
        } else {
            refreshLobbies(test_.game);
        }
    });
    connect(server_, &ServerClient::loggedOut, this, [this] {
        lobby_.clear();
        emit lobbyChanged();
        emit changed();
    });
    connect(server_, &ServerClient::resumeFailed, this, [this] {
        S()->setToken(QString());
        waiting_ = false;
        emit changed();
    });
    connect(server_, &ServerClient::kicked, this, [this](const QString&) {
        S()->setToken(QString());
        fail(tr("Your account signed in somewhere else."));
    });

    connect(match_, &MatchController::phaseChanged, this, [this] { emit matchChanged(); });
    connect(match_, &MatchController::matchFound, this, [this](const QJsonObject& found) {
        foundAt_ = Now();
        result_.clear();
        emit matchChanged();
        if (found.value(QStringLiteral("accept_required")).toBool()) {
            emit matchFound();
            if (test_.on)
                QTimer::singleShot(1500, this, [this] { accept(); });
        }
    });
    connect(match_, &MatchController::matchCancelled, this, [this](const QString& reason, bool requeued) {
        static const QHash<QString, QString> why = {
            {"declined", tr("Your opponent declined.")},
            {"accept_timeout", tr("Nobody accepted in time.")},
            {"no_udp", tr("No connection could be set up.")},
            {"punch_failed", tr("The two of you could not connect.")},
            {"ping_too_high", tr("The ping between you was too high.")},
            {"ping_timeout", tr("The connection test took too long.")},
            {"peer_left", tr("Your opponent left.")}};
        QString text = why.value(reason, tr("The match was called off."));
        if (requeued)
            text += QLatin1Char(' ') + tr("Back in the queue.");
        emit notice(text);
        emit matchChanged();
    });
    connect(match_, &MatchController::matchFinished, this, [this](const QJsonObject& result) {
        result_ = result.toVariantMap();
        emit matchChanged();
        emit matchOver();
        if (test_.on) {
            const QString status = Str(result, "status");
            applog::write(QStringLiteral("test: match %1, %2, winner %3")
                              .arg(Str(result, "match"), status)
                              .arg(result.value(QStringLiteral("winner")).toInt()));
            // Online Practice's matches end "practice": nothing to agree on, no record
            if (status != QLatin1String("agreed") && status != QLatin1String("practice")) {
                emit testFinished(false, QStringLiteral("result ") + status);
                return;
            }
            test_.played++;
            if (!test_.lobby.isEmpty() && test_.played >= test_.matches) {
                QTimer::singleShot(8000, this, [this] {  // after the match's end has played and the game parked
                    leaveLobby();
                    emit testFinished(true, QStringLiteral("lobby: %1 match(es) played").arg(test_.played));
                });
            }
        }
    });
    connect(match_, &MatchController::seriesChanged, this, [this](const QJsonObject& series) {
        seriesAt_ = Now();
        lastSeries_ = series.toVariantMap();
        emit matchChanged();
        if (!test_.on)
            return;
        const QString state = Str(series, "state");
        if (state == QLatin1String("over")) {
            emit testFinished(true, QStringLiteral("series over: ") + Str(series, "outcome"));
        } else if (state == QLatin1String("between")) {
            if (test_.played >= test_.matches)
                QTimer::singleShot(1000, this, [this] { leaveSeries(); });
            else if (!series.value(QStringLiteral("ranked")).toBool())
                QTimer::singleShot(1000, this, [this] { rematch(); });
        }
    });

    clock_.setInterval(1000);
    connect(&clock_, &QTimer::timeout, this, &OnlineHub::tick);
    clock_.start();

    // a remembered sign-in: on the server from the start, as the console menus show who is signed in
    if (!S()->token().isEmpty())
        QTimer::singleShot(1500, this, &OnlineHub::connectServer);
}

OnlineHub::~OnlineHub()
{
    match_->shutdown();
}

// ------------------------------------------------------------------------------------------------- properties

QString OnlineHub::state() const
{
    switch (server_->state()) {
    case ServerClient::State::Offline:
        return QStringLiteral("offline");
    case ServerClient::State::Connecting:
        return QStringLiteral("connecting");
    case ServerClient::State::Online:
        return QStringLiteral("online");
    case ServerClient::State::LoggedIn:
        return QStringLiteral("signedIn");
    }
    return QStringLiteral("offline");
}

QString OnlineHub::account() const { return server_->accountName(); }
QString OnlineHub::lastName() const { return S()->savedName(); }
bool OnlineHub::remembered() const { return !S()->token().isEmpty(); }
QString OnlineHub::motd() const { return Str(server_->welcome(), "motd"); }

QString OnlineHub::phase() const
{
    const QMetaEnum e = QMetaEnum::fromType<MatchController::Phase>();
    return QString::fromLatin1(e.valueToKey(int(match_->phase()))).toLower();
}

QString OnlineHub::phaseText() const { return match_->phaseText(); }
QString OnlineHub::searchGame() const { return match_->lastRequest().game; }
bool OnlineHub::searchRanked() const { return match_->lastRequest().ranked; }

int OnlineHub::searchSeconds() const
{
    return match_->phase() == MatchController::Phase::Searching ? match_->searchSeconds() : 0;
}

QVariantMap OnlineHub::found() const { return match_->currentFound().toVariantMap(); }

int OnlineHub::acceptSeconds() const
{
    const int total = match_->currentFound().value(QStringLiteral("accept_timeout")).toInt(20);
    return qMax(0, total - int((Now() - foundAt_) / 1000));
}

QVariantMap OnlineHub::series() const { return lastSeries_; }

int OnlineHub::seriesSeconds() const
{
    const int left = lastSeries_.value(QStringLiteral("seconds_left")).toInt();
    return qMax(0, left - int((Now() - seriesAt_) / 1000));
}

bool OnlineHub::lobbyReady() const
{
    const QString me = account();
    for (const QVariant& v : lobby_.value(QStringLiteral("members")).toList()) {
        const QVariantMap m = v.toMap();
        if (m.value(QStringLiteral("name")).toString() == me)
            return m.value(QStringLiteral("ready")).toBool();
    }
    return false;
}

// ---------------------------------------------------------------------------------------- connection, account

void OnlineHub::connectServer()
{
    if (server_->state() != ServerClient::State::Offline)
        return;
    error_.clear();
    const QString token = S()->token();
    server_->setResumeToken(token);
    waiting_ = !token.isEmpty();
    server_->connectTo(S()->server(), S()->fingerprint());
    emit changed();
}

void OnlineHub::signIn(const QString& name, const QString& password, bool remember)
{
    remember_ = remember;
    error_.clear();
    waiting_ = true;
    if (server_->isOnline()) {
        server_->login(name.trimmed(), password);
    } else {
        pending_ = {QStringLiteral("login"), name.trimmed(), password};
        connectServer();
    }
    emit changed();
}

void OnlineHub::createAccount(const QString& name, const QString& password, bool remember)
{
    remember_ = remember;
    error_.clear();
    waiting_ = true;
    if (server_->isOnline()) {
        server_->registerAccount(name.trimmed(), password);
    } else {
        pending_ = {QStringLiteral("register"), name.trimmed(), password};
        connectServer();
    }
    emit changed();
}

void OnlineHub::signOut()
{
    match_->cancelSearch();
    if (!lobby_.isEmpty())
        leaveLobby();
    S()->setToken(QString());
    server_->logout();
    profile_.clear();
    emit profileChanged();
    emit changed();
}

void OnlineHub::clearError()
{
    if (error_.isEmpty())
        return;
    error_.clear();
    emit changed();
}

void OnlineHub::fail(const QString& text)
{
    waiting_ = false;
    error_ = text;
    emit changed();
    emit notice(text);
    if (test_.on)
        emit testFinished(false, text);
}

void OnlineHub::send(QJsonObject msg)
{
    if (server_->send(std::move(msg)) < 0)
        emit notice(tr("Not connected to the server."));
}

QString OnlineHub::errorText(const QString& code, const QString& fallback)
{
    static const QHash<QString, QString> text = {
        {"bad_login", tr("Wrong name or password.")},
        {"bad_token", tr("Sign in again.")},
        {"bad_name", tr("A name is 3 to 16 letters, digits, - or _.")},
        {"bad_password", tr("A password is 6 to 128 characters.")},
        {"name_taken", tr("That name is taken.")},
        {"rate_limited", tr("Too many tries: wait a few minutes.")},
        {"not_logged_in", tr("Sign in first.")},
        {"busy", tr("You are already searching, in a lobby or in a match.")},
        {"bad_game", tr("The server does not have this game.")},
        {"bad_mode", tr("That mode is not open for this game.")},
        {"bad_region", tr("Unknown region.")},
        {"no_such_lobby", tr("That lobby is gone.")},
        {"lobby_full", tr("That lobby is full.")},
        {"bad_lobby_password", tr("Wrong lobby password.")},
        {"not_ready", tr("Both players must be ready.")},
        {"not_in_lobby", tr("You are not in a lobby.")},
        {"slow_down", tr("Slow down.")},
        {"no_such_account", tr("No such player.")},
        {"bad_product", tr("The server does not host the arcade games yet.")},
    };
    return text.value(code, fallback.isEmpty() ? code : fallback);
}

void OnlineHub::onMessage(const QJsonObject& msg)
{
    const QString type = Str(msg, "type");
    if (type == QLatin1String("welcome")) {
        if (!pending_.isEmpty()) {
            const QStringList p = pending_;
            pending_.clear();
            if (p[0] == QLatin1String("login"))
                server_->login(p[1], p[2]);
            else
                server_->registerAccount(p[1], p[2]);
        }
        emit changed();
    } else if (type == QLatin1String("online")) {
        onlineCount_ = msg.value(QStringLiteral("count")).toInt();
        gameCounts_ = msg.value(QStringLiteral("games")).toObject().toVariantMap();
        emit countsChanged();
    } else if (type == QLatin1String("lobbies")) {
        lobbies_ = msg.value(QStringLiteral("lobbies")).toArray().toVariantList();
        emit lobbiesChanged();
        if (test_.on && test_.lobby == QLatin1String("join") && lobby_.isEmpty()) {
            for (const QVariant& v : lobbies_) {
                const QVariantMap l = v.toMap();
                if (l.value(QStringLiteral("game")).toString() == test_.game) {
                    joinLobby(l.value(QStringLiteral("id")).toString());
                    break;
                }
            }
        }
    } else if (type == QLatin1String("lobby")) {
        lobby_ = msg.value(QStringLiteral("lobby")).toObject().toVariantMap();
        emit lobbyChanged();
        if (standDown_ && lobby_.value(QStringLiteral("state")).toString() != QLatin1String("in_match")) {
            standDown_ = false;
            if (lobbyReady())
                setReady(false);
        }
        if (test_.on && !test_.lobby.isEmpty()) {
            if (test_.lobby == QLatin1String("host") && !test_.codeFile.isEmpty() && !test_.codeShared) {
                QFile f(test_.codeFile);
                if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                    f.write(lobby_.value(QStringLiteral("id")).toString().toLatin1());
                    test_.codeShared = true;
                }
            }
            testStep();
        }
    } else if (type == QLatin1String("lobby_chat")) {
        if (msg.value(QStringLiteral("lobby")).toString() == lobby_.value(QStringLiteral("id")).toString()) {
            QVariantList chat = lobby_.value(QStringLiteral("chat")).toList();
            chat.append(QVariantMap{{"from", Str(msg, "from")}, {"text", Str(msg, "text")}});
            while (chat.size() > 30)
                chat.removeFirst();
            lobby_.insert(QStringLiteral("chat"), chat);
            emit lobbyChanged();
        }
    } else if (type == QLatin1String("lobby_left")) {
        lobby_.clear();
        match_->closeGameWhenIdle();
        emit lobbyChanged();
    } else if (type == QLatin1String("leaderboard")) {
        board_ = msg.toVariantMap();
        emit boardChanged();
    } else if (type == QLatin1String("profile")) {
        profile_ = msg.toVariantMap();
        emit profileChanged();
    } else if (type == QLatin1String("notice")) {
        emit notice(Str(msg, "text"));
    } else if (type == QLatin1String("error")) {
        const QString code = Str(msg, "code"), re = Str(msg, "re");
        const QString text = errorText(code, Str(msg, "message"));
        if ((re == QLatin1String("login") || re == QLatin1String("register")) && code == QLatin1String("busy") &&
            server_->isLoggedIn()) {
            // the remembered sign-in got there first: signed in all the same
            waiting_ = false;
            emit changed();
        } else if (re == QLatin1String("login") || re == QLatin1String("register")) {
            if (test_.on && code == QLatin1String("bad_login") && !test_.triedRegister) {
                // a test account seen for the first time: create it
                test_.triedRegister = true;
                server_->registerAccount(test_.name, test_.password);
                return;
            }
            fail(text);
        } else if (re == QLatin1String("auth")) {
            waiting_ = false;
            emit changed();
        } else if (re == QLatin1String("lobby_ready")) {
            emit notice(text);
            emit lobbyChanged();
        } else if (code != QLatin1String("idle")) {
            emit notice(text);
            if (test_.on && re == QLatin1String("queue_join"))
                emit testFinished(false, QStringLiteral("queue_join: ") + code);
        }
    }
}

// ------------------------------------------------------------------------------------------------ matchmaking

bool OnlineHub::search(const QString& game, bool ranked)
{
    QueueRequest req;
    req.game = game;
    req.mode = QStringLiteral("kombat");
    req.region = S()->region();
    req.character = QString();  // both pick on the game's select screen
    req.ranked = ranked;
    req.maxPing = S()->maxPing();
    req.delay = cfg::inputDelay();
    req.deathTraps = false;
    QString err;
    if (!match_->findMatch(req, &err)) {
        emit notice(err);
        return false;
    }
    result_.clear();
    lastSeries_.clear();
    emit matchChanged();
    return true;
}

void OnlineHub::cancelSearch() { match_->cancelSearch(); }
void OnlineHub::accept() { match_->accept(); }
void OnlineHub::decline() { match_->decline(); }
void OnlineHub::rematch() { match_->seriesReady(true, QStringLiteral("rematch")); }
void OnlineHub::leaveSeries() { match_->seriesReady(false); }

void OnlineHub::clearResult()
{
    result_.clear();
    if (lastSeries_.value(QStringLiteral("state")).toString() == QLatin1String("over"))
        lastSeries_.clear();
    emit matchChanged();
}

// ------------------------------------------------------------------------------------------------------ lobbies

void OnlineHub::refreshLobbies(const QString& game)
{
    QJsonObject msg{{QStringLiteral("type"), QStringLiteral("lobby_list")}};
    if (!game.isEmpty())
        msg.insert(QStringLiteral("game"), game);
    send(msg);
}

void OnlineHub::createLobby(const QString& game, const QString& mode, const QString& password)
{
    QJsonObject msg{{QStringLiteral("type"), QStringLiteral("lobby_create")},
                    {QStringLiteral("name"), tr("%1's lobby").arg(account())},
                    {QStringLiteral("game"), game},
                    {QStringLiteral("mode"), mode},
                    {QStringLiteral("password"), password},
                    {QStringLiteral("rounds"), 3},
                    {QStringLiteral("region"), S()->region()},
                    {QStringLiteral("character"), QString()},
                    {QStringLiteral("death_traps"), false},
                    {QStringLiteral("koth"), false},
                    {QStringLiteral("link"), server_->linkType()}};
    if (match_->serverRttMs() >= 0)
        msg.insert(QStringLiteral("server_rtt"), match_->serverRttMs());
    send(msg);
}

void OnlineHub::joinLobby(const QString& id, const QString& password)
{
    QJsonObject msg{{QStringLiteral("type"), QStringLiteral("lobby_join")},
                    {QStringLiteral("lobby"), id.trimmed().toUpper()},
                    {QStringLiteral("password"), password},
                    {QStringLiteral("character"), QString()},
                    {QStringLiteral("spectate"), false},
                    {QStringLiteral("link"), server_->linkType()}};
    if (match_->serverRttMs() >= 0)
        msg.insert(QStringLiteral("server_rtt"), match_->serverRttMs());
    send(msg);
}

void OnlineHub::leaveLobby() { send({{QStringLiteral("type"), QStringLiteral("lobby_leave")}}); }

void OnlineHub::setReady(bool ready)
{
    if (lobby_.isEmpty())
        return;
    if (ready) {
        QString err;
        if (!match_->prepareGame(lobby_.value(QStringLiteral("game")).toString(), &err)) {
            emit notice(err);
            return;
        }
    }
    QJsonObject msg{{QStringLiteral("type"), QStringLiteral("lobby_ready")},
                    {QStringLiteral("ready"), ready},
                    {QStringLiteral("delay"), cfg::inputDelay()},
                    {QStringLiteral("relay"), cfg::hideIp()},
                    {QStringLiteral("hide_always"), cfg::hideIpAlways()},
                    {QStringLiteral("link"), server_->linkType()}};
    if (match_->serverRttMs() >= 0)
        msg.insert(QStringLiteral("server_rtt"), match_->serverRttMs());
    send(msg);
}

void OnlineHub::standDown()
{
    if (lobby_.isEmpty())
        return;
    if (lobby_.value(QStringLiteral("state")).toString() == QLatin1String("in_match")) {
        standDown_ = true;  // once the server has the match's end
        return;
    }
    if (lobbyReady())
        setReady(false);
}

void OnlineHub::chat(const QString& text)
{
    const QString t = text.trimmed().left(200);
    if (!t.isEmpty())
        send({{QStringLiteral("type"), QStringLiteral("lobby_chat")}, {QStringLiteral("text"), t}});
}

// --------------------------------------------------------------------------------------- leaderboard, profile

void OnlineHub::loadBoard(const QString& game)
{
    send({{QStringLiteral("type"), QStringLiteral("leaderboard")},
          {QStringLiteral("game"), game},
          {QStringLiteral("mode"), QStringLiteral("kombat")},
          {QStringLiteral("limit"), 100},
          {QStringLiteral("offset"), 0},
          {QStringLiteral("by"), QStringLiteral("elo")}});
}

void OnlineHub::loadProfile(const QString& name)
{
    QJsonObject msg{{QStringLiteral("type"), QStringLiteral("profile")}};
    if (!name.isEmpty())
        msg.insert(QStringLiteral("name"), name);
    send(msg);
}

// ---------------------------------------------------------------------------------------------------- tests

// a lobby test: ready once both players are in, the game is idle and matches are left to play
void OnlineHub::testStep()
{
    int players = 0;
    for (const QVariant& v : lobby_.value(QStringLiteral("members")).toList())
        if (!v.toMap().value(QStringLiteral("spectator")).toBool())
            players++;
    if (players == 2 && !lobbyReady() && match_->phase() == MatchController::Phase::Idle &&
        lobby_.value(QStringLiteral("state")).toString() != QLatin1String("in_match") && test_.played < test_.matches)
        QTimer::singleShot(500, this, [this] {
            if (!lobbyReady())
                setReady(true);
        });
}

void OnlineHub::runTest(const QString& name, const QString& password, const QString& game, bool ranked, int matches,
                        const QString& lobby, const QString& mode, const QString& codeFile)
{
    test_.on = true;
    test_.name = name;
    test_.password = password;
    test_.game = game;
    test_.ranked = ranked;
    test_.matches = qMax(1, matches);
    test_.lobby = lobby;
    test_.mode = mode.isEmpty() ? QStringLiteral("kombat") : mode;
    test_.codeFile = codeFile;
    applog::write(QStringLiteral("test: %1 searches %2 %3, %4 match(es)")
                      .arg(name, ranked ? QStringLiteral("ranked") : QStringLiteral("kasual"), game)
                      .arg(test_.matches));
    signIn(name, password, true);  // remembered: the test's data folder then opens signed in (screenshots)
}
