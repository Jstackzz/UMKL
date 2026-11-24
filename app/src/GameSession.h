#pragma once
// One running game: the emulation core on its own thread, paced to the board's frame rate, its sound through SDL,
// its pictures handed to GameView. Modes: "preview" (the attract mode behind the menus: no input, quiet), "arcade"
// (player 1's controller and keyboard), "versus" (two controllers), "net" (rollback against another PC, Direct
// Connect) and "link" (an online match the server set up: MatchController drives it through GameProcess).
#include <QImage>
#include <QMutex>
#include <QObject>
#include <QString>

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <thread>

class InputHub;
struct NetConfig;

// the newest picture, from the game thread to the renderer
class FrameBox {
public:
    void put(const uint8_t* src, int w, int h, int pitch);
    // copies the picture when it is newer than serial; false when nothing new came
    bool take(QImage& out, quint64& serial);

private:
    QMutex m_;
    QImage img_;
    quint64 serial_ = 0;
};

// An online match the server set up, from MatchController's start line (mk3d-rollback docs/PROTOCOL.md 3.3)
struct LinkStart {
    QString set;
    QString mode = QStringLiteral("kombat");  // or "sparring", Online Practice: no result, it runs until one leaves
    int side = 1;
    int localPort = 7101;
    QString peer;  // ip:port
    int delay = 2;
    QString match;       // its id, echoed in its status lines
    QString sessionDir;  // its game.log goes there
    QString p1Name, p2Name;
};

class GameSession : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool running READ running NOTIFY stateChanged)
    Q_PROPERTY(bool paused READ paused NOTIFY stateChanged)
    Q_PROPERTY(QString set READ set NOTIFY stateChanged)
    Q_PROPERTY(QString mode READ mode NOTIFY stateChanged)
    Q_PROPERTY(bool linkActive READ linkActive NOTIFY stateChanged)
    Q_PROPERTY(QString linkNames READ linkNames NOTIFY stateChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    Q_PROPERTY(QString netStatus READ netStatus NOTIFY netChanged)
    Q_PROPERTY(int netPing READ netPing NOTIFY netChanged)
    Q_PROPERTY(int netRollback READ netRollback NOTIFY netChanged)

public:
    explicit GameSession(InputHub* input, QObject* parent = nullptr);
    ~GameSession() override;

    Q_INVOKABLE void play(const QString& set, const QString& mode);
    // rollback netplay against another copy of this app: side 1 or 2, the peer as host:port
    Q_INVOKABLE void playNet(const QString& set, int side, int localPort, const QString& peer, int delay);
    // --net-test: scripted inputs for frames frames, the state's hash every 300 frames into log
    void playNetTest(const QString& set, int side, int localPort, const QString& peer, int delay, int frames,
                     const QString& log);
    // an online match the server set up; returns its number, which its status lines (linkStatus) carry. Its game
    // reports what the protocol's game process would: starting, in_match, result, desync, disconnected, error
    quint64 playLink(const LinkStart& start);
    // the match is over for the server: its game plays the match's end (the winner's finisher) a few seconds more,
    // then stops, and its status is "ready" (back at the hold point: this app's menus)
    void parkLink(quint64 link);
    // the match's game closes now and says nothing more (its GameProcess quit)
    void dropLink(quint64 link);
    // tests: what the local player presses in an online match comes from a script ("attack": walk in and punch,
    // "idle": pick a fighter, then stand), so two copies can play a whole match by themselves
    void setAutoplay(const QString& how) { autoplay_ = how; }

    Q_INVOKABLE void stop();
    Q_INVOKABLE void setPaused(bool p);
    Q_INVOKABLE void restart();

    bool running() const { return running_; }
    bool paused() const { return paused_.load(); }
    QString set() const { return set_; }
    QString mode() const { return mode_; }
    bool linkActive() const { return link_ != 0 && running_; }
    QString linkNames() const { return linkNames_; }
    QString error() const { return error_; }
    QString netStatus() const { return netStatus_; }
    int netPing() const { return netPing_; }
    int netRollback() const { return netRollback_; }
    FrameBox* frames() { return &frames_; }

signals:
    void stateChanged();
    void errorChanged();
    void netChanged();
    void frameReady();
    // the game thread stopped by itself (an error, the other player left)
    void ended(const QString& reason);
    // a better picture of a game's attract mode was kept for its card
    void artChanged(const QString& set);
    // an online match's game: a status line for its GameProcess (link: playLink's number)
    void linkStatus(const QString& line, quint64 link);
    // an online match's game started (its screen comes up), or went back to the menus after its end
    void linkStarted(const QString& set);
    void linkParked();

private:
    void launch(const QString& set, const QString& mode, std::shared_ptr<NetConfig> net);
    void run(QString set, QString mode, std::shared_ptr<NetConfig> net, quint64 generation);
    void post(std::function<void()> f);
    void fail(quint64 generation, const QString& why);
    void onLinkLine(quint64 generation, const QString& line);
    void onLinkDone(quint64 generation, bool byItself);

    InputHub* input_;
    FrameBox frames_;
    std::thread worker_;
    std::atomic<bool> stop_{false};
    std::atomic<bool> paused_{false};
    std::atomic<bool> restart_{false};
    std::atomic<bool> park_{false};  // the running online match is to stop once its end has played
    std::atomic<quint64> generation_{0};
    bool running_ = false;
    QString set_, mode_, error_;
    QString netStatus_;
    int netPing_ = 0, netRollback_ = 0;
    std::shared_ptr<NetConfig> lastNet_;
    // the online match (its generation; 0 none), and how it stands
    quint64 link_ = 0;
    QString linkMatch_, linkMode_, linkNames_;
    bool linkResult_ = false;  // its result went out
    bool linkEnded_ = false;   // its game stopped by itself (desync, disconnected, error, the peer gone after a result)
    QString autoplay_;
};
