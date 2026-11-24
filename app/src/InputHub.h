#pragma once
// Controllers (SDL3) and the keyboard (through QML), for two jobs: steering the menus (nav signals, with key repeat
// on held directions) and playing (each player's arcade pad as bits, read by the game thread every frame).
#include <QObject>
#include <QString>
#include <QTimer>

#include <array>
#include <atomic>
#include <cstdint>
#include <vector>

struct SDL_Gamepad;

class InputHub : public QObject {
    Q_OBJECT
    Q_PROPERTY(int padCount READ padCount NOTIFY padsChanged)
    Q_PROPERTY(QStringList padNames READ padNames NOTIFY padsChanged)
    Q_PROPERTY(bool gameMode READ gameMode WRITE setGameMode NOTIFY gameModeChanged)
    Q_PROPERTY(QString glyphs READ glyphs NOTIFY padsChanged)  // "xbox", "playstation", "switch" or "keyboard"

public:
    explicit InputHub(QObject* parent = nullptr);
    ~InputHub() override;

    // a key from QML; true when it was taken (menus: navigation, game: a pad button or pause)
    Q_INVOKABLE bool keyEvent(int key, bool pressed, bool autoRepeat);
    // a short rumble on player's controller (0 or 1)
    Q_INVOKABLE void rumble(int player, int ms);

    // read by the game thread: a player's arcade pad, pad:: bits
    uint16_t padState(int player) const { return player >= 0 && player < 2 ? pads_[player].load() : 0; }

    int padCount() const { return int(open_.size()); }
    QStringList padNames() const;
    bool gameMode() const { return gameMode_; }
    void setGameMode(bool on);
    QString glyphs() const;

signals:
    // up, down, left, right, accept, back, option (Y / Tab), tabLeft, tabRight, start
    void nav(const QString& action);
    void pauseRequested();  // in a game: BACK / VIEW on a controller, Escape on the keyboard
    void padsChanged();
    void gameModeChanged();

private:
    void poll();
    void openPad(uint32_t id);
    void closePad(uint32_t id);
    void navPress(const QString& action);
    void updatePads();

    struct Pad {
        uint32_t id = 0;
        SDL_Gamepad* pad = nullptr;
        QString name;
        int type = 0;
    };
    std::vector<Pad> open_;
    QTimer timer_;
    bool ok_ = false;
    bool gameMode_ = false;
    uint16_t keyboard_ = 0;  // player 1's keys
    std::array<std::atomic<uint16_t>, 2> pads_{};

    // held directions repeat in menus
    QString heldNav_;
    qint64 heldSince_ = 0, lastRepeat_ = 0;
    int stickDir_[2] = {0, 0};  // left stick of the first controller, as a menu direction (-1, 0, 1 per axis)
};
