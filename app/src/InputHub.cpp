#include "InputHub.h"

#include "EmuCore.h"

#include <QDateTime>
#include <QDebug>

#include <SDL3/SDL.h>

namespace {

qint64 NowMs() { return QDateTime::currentMSecsSinceEpoch(); }

// the arcade layout on a controller, as the MK console games had it: the face buttons are the four attacks (west
// high punch, south low punch, north high kick, east low kick), the shoulders block, the triggers run
uint16_t PadBits(SDL_Gamepad* g)
{
    uint16_t b = 0;
    auto btn = [&](SDL_GamepadButton x) { return SDL_GetGamepadButton(g, x); };
    if (btn(SDL_GAMEPAD_BUTTON_DPAD_UP))
        b |= pad::Up;
    if (btn(SDL_GAMEPAD_BUTTON_DPAD_DOWN))
        b |= pad::Down;
    if (btn(SDL_GAMEPAD_BUTTON_DPAD_LEFT))
        b |= pad::Left;
    if (btn(SDL_GAMEPAD_BUTTON_DPAD_RIGHT))
        b |= pad::Right;
    const int lx = SDL_GetGamepadAxis(g, SDL_GAMEPAD_AXIS_LEFTX);
    const int ly = SDL_GetGamepadAxis(g, SDL_GAMEPAD_AXIS_LEFTY);
    const int dead = 16000;
    if (lx < -dead)
        b |= pad::Left;
    if (lx > dead)
        b |= pad::Right;
    if (ly < -dead)
        b |= pad::Up;
    if (ly > dead)
        b |= pad::Down;
    if (btn(SDL_GAMEPAD_BUTTON_WEST))
        b |= pad::HighPunch;
    if (btn(SDL_GAMEPAD_BUTTON_SOUTH))
        b |= pad::LowPunch;
    if (btn(SDL_GAMEPAD_BUTTON_NORTH))
        b |= pad::HighKick;
    if (btn(SDL_GAMEPAD_BUTTON_EAST))
        b |= pad::LowKick;
    if (btn(SDL_GAMEPAD_BUTTON_LEFT_SHOULDER) || btn(SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER))
        b |= pad::Block;
    if (SDL_GetGamepadAxis(g, SDL_GAMEPAD_AXIS_LEFT_TRIGGER) > 12000 ||
        SDL_GetGamepadAxis(g, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) > 12000)
        b |= pad::Run;
    if (btn(SDL_GAMEPAD_BUTTON_START))
        b |= pad::Start;
    return b;
}

// player 1's keyboard: arrows or WASD, the arcade panel's two rows on U I O / J K L (high punch, block, high kick /
// low punch, run, low kick), Enter for start
uint16_t KeyBit(int key)
{
    switch (key) {
    case Qt::Key_Up: case Qt::Key_W: return pad::Up;
    case Qt::Key_Down: case Qt::Key_S: return pad::Down;
    case Qt::Key_Left: case Qt::Key_A: return pad::Left;
    case Qt::Key_Right: case Qt::Key_D: return pad::Right;
    case Qt::Key_U: return pad::HighPunch;
    case Qt::Key_I: case Qt::Key_Space: return pad::Block;
    case Qt::Key_O: return pad::HighKick;
    case Qt::Key_J: return pad::LowPunch;
    case Qt::Key_K: return pad::Run;
    case Qt::Key_L: return pad::LowKick;
    case Qt::Key_Return: case Qt::Key_Enter: return pad::Start;
    default: return 0;
    }
}

}  // namespace

InputHub::InputHub(QObject* parent) : QObject(parent)
{
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
    ok_ = SDL_InitSubSystem(SDL_INIT_GAMEPAD);
    if (!ok_)
        qWarning() << "SDL gamepads:" << SDL_GetError();
    connect(&timer_, &QTimer::timeout, this, &InputHub::poll);
    timer_.setTimerType(Qt::PreciseTimer);
    timer_.start(2);
}

InputHub::~InputHub()
{
    for (auto& p : open_)
        SDL_CloseGamepad(p.pad);
    open_.clear();
    if (ok_)
        SDL_QuitSubSystem(SDL_INIT_GAMEPAD);
}

void InputHub::openPad(uint32_t id)
{
    for (auto& p : open_)
        if (p.id == id)
            return;
    SDL_Gamepad* g = SDL_OpenGamepad(id);
    if (!g)
        return;
    Pad p;
    p.id = id;
    p.pad = g;
    const char* n = SDL_GetGamepadName(g);
    p.name = n ? QString::fromUtf8(n) : QStringLiteral("Controller");
    p.type = int(SDL_GetGamepadType(g));
    open_.push_back(p);
    emit padsChanged();
}

void InputHub::closePad(uint32_t id)
{
    for (auto it = open_.begin(); it != open_.end(); ++it) {
        if (it->id == id) {
            SDL_CloseGamepad(it->pad);
            open_.erase(it);
            emit padsChanged();
            return;
        }
    }
}

QStringList InputHub::padNames() const
{
    QStringList l;
    for (auto& p : open_)
        l << p.name;
    return l;
}

QString InputHub::glyphs() const
{
    if (open_.empty())
        return QStringLiteral("keyboard");
    switch (open_.front().type) {
    case SDL_GAMEPAD_TYPE_PS3:
    case SDL_GAMEPAD_TYPE_PS4:
    case SDL_GAMEPAD_TYPE_PS5:
        return QStringLiteral("playstation");
    case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_PRO:
    case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_LEFT:
    case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_RIGHT:
    case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_PAIR:
        return QStringLiteral("switch");
    default:
        return QStringLiteral("xbox");
    }
}

void InputHub::setGameMode(bool on)
{
    if (gameMode_ == on)
        return;
    gameMode_ = on;
    heldNav_.clear();
    keyboard_ = 0;
    emit gameModeChanged();
}

void InputHub::navPress(const QString& action)
{
    if (gameMode_)
        return;
    emit nav(action);
}

void InputHub::rumble(int player, int ms)
{
    if (player >= 0 && player < int(open_.size()))
        SDL_RumbleGamepad(open_[player].pad, 0x6000, 0x9000, Uint32(ms));
}

bool InputHub::keyEvent(int key, bool pressed, bool autoRepeat)
{
    if (gameMode_) {
        if (key == Qt::Key_Escape) {
            if (pressed && !autoRepeat)
                emit pauseRequested();
            return true;
        }
        const uint16_t bit = KeyBit(key);
        if (!bit)
            return false;
        if (pressed)
            keyboard_ |= bit;
        else
            keyboard_ &= ~bit;
        updatePads();
        return true;
    }
    if (!pressed)
        return false;
    QString a;
    switch (key) {
    case Qt::Key_Up: case Qt::Key_W: a = QStringLiteral("up"); break;
    case Qt::Key_Down: case Qt::Key_S: a = QStringLiteral("down"); break;
    case Qt::Key_Left: case Qt::Key_A: a = QStringLiteral("left"); break;
    case Qt::Key_Right: case Qt::Key_D: a = QStringLiteral("right"); break;
    case Qt::Key_Return: case Qt::Key_Enter: case Qt::Key_Space: case Qt::Key_J: a = QStringLiteral("accept"); break;
    case Qt::Key_Escape: case Qt::Key_Backspace: case Qt::Key_K: a = QStringLiteral("back"); break;
    case Qt::Key_Tab: case Qt::Key_U: a = QStringLiteral("option"); break;
    case Qt::Key_Q: case Qt::Key_PageUp: a = QStringLiteral("tabLeft"); break;
    case Qt::Key_E: case Qt::Key_PageDown: a = QStringLiteral("tabRight"); break;
    default: return false;
    }
    navPress(a);
    return true;
}

void InputHub::updatePads()
{
    uint16_t p[2] = {keyboard_, 0};
    for (size_t i = 0; i < open_.size() && i < 2; i++)
        p[i] |= PadBits(open_[i].pad);
    pads_[0].store(p[0]);
    pads_[1].store(p[1]);
}

void InputHub::poll()
{
    if (!ok_)
        return;
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        switch (e.type) {
        case SDL_EVENT_GAMEPAD_ADDED:
            openPad(e.gdevice.which);
            break;
        case SDL_EVENT_GAMEPAD_REMOVED:
            closePad(e.gdevice.which);
            break;
        case SDL_EVENT_GAMEPAD_BUTTON_DOWN: {
            if (gameMode_) {
                if (e.gbutton.button == SDL_GAMEPAD_BUTTON_BACK || e.gbutton.button == SDL_GAMEPAD_BUTTON_GUIDE)
                    emit pauseRequested();
                break;
            }
            QString a;
            switch (e.gbutton.button) {
            case SDL_GAMEPAD_BUTTON_DPAD_UP: a = QStringLiteral("up"); break;
            case SDL_GAMEPAD_BUTTON_DPAD_DOWN: a = QStringLiteral("down"); break;
            case SDL_GAMEPAD_BUTTON_DPAD_LEFT: a = QStringLiteral("left"); break;
            case SDL_GAMEPAD_BUTTON_DPAD_RIGHT: a = QStringLiteral("right"); break;
            case SDL_GAMEPAD_BUTTON_SOUTH: a = QStringLiteral("accept"); break;
            case SDL_GAMEPAD_BUTTON_EAST: case SDL_GAMEPAD_BUTTON_BACK: a = QStringLiteral("back"); break;
            case SDL_GAMEPAD_BUTTON_NORTH: a = QStringLiteral("option"); break;
            case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER: a = QStringLiteral("tabLeft"); break;
            case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER: a = QStringLiteral("tabRight"); break;
            case SDL_GAMEPAD_BUTTON_START: a = QStringLiteral("start"); break;
            default: break;
            }
            if (!a.isEmpty()) {
                navPress(a);
                if (a == QLatin1String("up") || a == QLatin1String("down") || a == QLatin1String("left") ||
                    a == QLatin1String("right")) {
                    heldNav_ = a;
                    heldSince_ = lastRepeat_ = NowMs();
                }
            }
            break;
        }
        case SDL_EVENT_GAMEPAD_BUTTON_UP:
            heldNav_.clear();
            break;
        default:
            break;
        }
    }

    updatePads();

    if (gameMode_ || open_.empty())
        return;
    // the first controller's left stick steers the menus too, with the same repeat as a held d-pad
    const int lx = SDL_GetGamepadAxis(open_.front().pad, SDL_GAMEPAD_AXIS_LEFTX);
    const int ly = SDL_GetGamepadAxis(open_.front().pad, SDL_GAMEPAD_AXIS_LEFTY);
    const int sx = lx < -20000 ? -1 : lx > 20000 ? 1 : 0;
    const int sy = ly < -20000 ? -1 : ly > 20000 ? 1 : 0;
    if (sx != stickDir_[0] || sy != stickDir_[1]) {
        stickDir_[0] = sx;
        stickDir_[1] = sy;
        QString a = sy < 0 ? QStringLiteral("up") : sy > 0 ? QStringLiteral("down")
                    : sx < 0 ? QStringLiteral("left") : sx > 0 ? QStringLiteral("right") : QString();
        if (!a.isEmpty()) {
            navPress(a);
            heldNav_ = a;
            heldSince_ = lastRepeat_ = NowMs();
        } else {
            heldNav_.clear();
        }
    }
    if (!heldNav_.isEmpty()) {
        const qint64 now = NowMs();
        if (now - heldSince_ > 380 && now - lastRepeat_ > 95) {
            lastRepeat_ = now;
            navPress(heldNav_);
        }
    }
}
