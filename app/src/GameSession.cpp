#include "GameSession.h"

#include "EmuCore.h"
#include "InputHub.h"
#include "Settings.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMetaObject>
#include <QHash>
#include <QList>
#include <QSettings>
#include <QTemporaryDir>

#include <algorithm>
#include <cmath>
#include <map>

#include <SDL3/SDL.h>
#include <gekkonet.h>
#include <windows.h>
#include <timeapi.h>

#include <chrono>
#include <cstdio>
#include <vector>

using Clock = std::chrono::steady_clock;

// what a netplay session needs, given by playNet (or by --net-test), or by playLink for a server's match
struct NetConfig {
    int side = 1;              // 1 left (P1), 2 right (P2)
    int localPort = 7101;
    std::string peer;          // host:port
    int delay = 2;             // input delay, frames
    int testFrames = 0;        // --net-test: frames to play with scripted inputs, then stop
    QString testLog;           // --net-test: state hashes every 300 frames
    bool link = false;         // a server's match: status lines for its GameProcess
    QString mode;              // link: "kombat" or "sparring"
    QString match;             // link: its id
    QString sessionDir;        // link: game.log
    std::string autoplay;      // link, tests: the local player's script ("attack", "idle")
};

// ---------------------------------------------------------------------------------------------------------- FrameBox

void FrameBox::put(const uint8_t* src, int w, int h, int pitch)
{
    QMutexLocker lock(&m_);
    if (img_.width() != w || img_.height() != h)
        img_ = QImage(w, h, QImage::Format_RGB32);
    // FBNeo writes 0x00RRGGBB; QImage's RGB32 wants the top byte at 0xFF
    for (int y = 0; y < h; y++) {
        const uint32_t* s = reinterpret_cast<const uint32_t*>(src + size_t(y) * pitch);
        uint32_t* d = reinterpret_cast<uint32_t*>(img_.scanLine(y));
        for (int x = 0; x < w; x++)
            d[x] = s[x] | 0xFF000000u;
    }
    serial_++;
}

bool FrameBox::take(QImage& out, quint64& serial)
{
    QMutexLocker lock(&m_);
    if (serial_ == serial || img_.isNull())
        return false;
    out = img_.copy();
    serial = serial_;
    return true;
}

// --------------------------------------------------------------------------------------------------------- helpers

namespace {

// sleeps to a point in time on a high resolution waitable timer, then spins the last half millisecond
class Pacer {
public:
    Pacer() { timer_ = CreateWaitableTimerExW(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS); }
    ~Pacer() { if (timer_) CloseHandle(timer_); }
    void until(Clock::time_point t)
    {
        for (;;) {
            const auto left = t - Clock::now();
            if (left <= Clock::duration::zero())
                return;
            if (left > std::chrono::microseconds(1500) && timer_) {
                LARGE_INTEGER due;
                due.QuadPart = -LONGLONG(std::chrono::duration_cast<std::chrono::nanoseconds>(
                                             left - std::chrono::microseconds(500)).count() / 100);
                SetWaitableTimer(timer_, &due, 0, nullptr, nullptr, FALSE);
                WaitForSingleObject(timer_, INFINITE);
            } else {
                YieldProcessor();
            }
        }
    }

private:
    HANDLE timer_ = nullptr;
};

// An arcade game wants coins. Pressing START plays two coins (UMK3 takes 2 credits to start, or to continue), then
// start, so a player never sees a credit screen; the result is ordinary input, so online both games get it the same.
class CoinPilot {
public:
    uint16_t apply(uint16_t raw)
    {
        const bool start = raw & pad::Start;
        if (start && !lastStart_ && t_ == 0)
            t_ = 1;
        lastStart_ = start;
        raw &= ~pad::Start;
        if (t_ > 0) {
            if (t_ <= 4 || (t_ >= 9 && t_ <= 12))
                raw |= pad::Coin;
            else if (t_ >= 26 && t_ <= 30)
                raw |= pad::Start;
            if (++t_ > 30)
                t_ = 0;
        }
        return raw;
    }

private:
    int t_ = 0;
    bool lastStart_ = false;
};

// A set started without its battery RAM shows "CMOS INVALID ... ANY BUTTON TO CONTINUE". Both players' BLOCK for a
// few frames now and then over the first seconds takes it on; by frame number, so it is the same on both PCs.
uint16_t BootPilot(uint64_t frame, bool freshNvram)
{
    if (!freshNvram || frame < 120 || frame > 1500)
        return 0;
    return (frame % 150) < 4 ? pad::Block : 0;
}

// Online both games join both players before netplay starts, with the same inputs on both PCs: BLOCK past the CMOS
// screen, coins for both, player 1 starts, then player 2 joins on player 1's select screen (pressed three times, for a
// screen that takes a moment to listen). Netplay's first frame is that select screen, where each player picks with their
// own controller. The same steps as tools/py/ramscan.py's boot_and_select, up to the picks.
struct JoinPilot {
    static constexpr uint64_t kBoot = 1700;
    static constexpr uint64_t kLength = kBoot + 4 + 30 + 4 + 240 + 3 * (4 + 30 + 4 + 60) + 120;
    static void at(uint64_t f, uint16_t& p1, uint16_t& p2)
    {
        p1 = p2 = 0;
        if (f < kBoot) {
            p1 = p2 = (f >= 120 && f <= 1500 && f % 150 < 4) ? uint16_t(pad::Block) : uint16_t(0);
            return;
        }
        f -= kBoot;
        if (f < 4) { p1 = p2 = pad::Coin; return; }
        f -= 4;
        if (f < 30) return;
        f -= 30;
        if (f < 4) { p1 = pad::Start; return; }
        f -= 4;
        if (f < 240) return;
        f -= 240;
        for (int i = 0; i < 3; i++) {
            if (f < 4) { p2 = pad::Coin; return; }
            f -= 4;
            if (f < 30) return;
            f -= 30;
            if (f < 4) { p2 = pad::Start; return; }
            f -= 4;
            if (f < 60) return;
            f -= 60;
        }
    }
};

// Where each game counts the rounds each player has won (FBNeo's "All RAM", found by tools/py/ramscan.py; MK1's are in
// each player's own block, with --reset, as it clears them about 3 s after the match, and it sets the loser's to 255
// after a round lost without a win): an online match's result is read from them. A player with 2 wins has won.
struct RoundRam {
    const char* set;
    uint32_t p1, p2;
};
const RoundRam kRoundRam[] = {
    {"umk3", 0xC462, 0xC464},
    {"mk3", 0xC458, 0xC45A},
    {"mk2", 0xC36E, 0xC370},
    {"mk", 0xA264, 0xA2C0},
};
const RoundRam* RoundRamFor(const QString& set)
{
    for (const RoundRam& r : kRoundRam)
        if (set == QLatin1String(r.set))
            return &r;
    return nullptr;
}

// how long a player's 2 wins must stand before they are the result: far beyond what a rollback reaches back
constexpr int kResultFrames = 30;
// after the result the match's end plays on in both games (the winner's finisher, their pose) until both park
constexpr auto kAfterResult = std::chrono::seconds(6);

// tests (GameSession::setAutoplay): what an online match's local player presses, by netplay frame. Both pick the
// fighter under their cursor; then "attack" walks at the other and punches (as tools/py/ramscan.py's fight does) and
// "idle" stands still, so the attacker wins.
uint16_t Autoplay(const std::string& how, int side, uint64_t f)
{
    if ((f >= 10 && f < 14) || (f >= 74 && f < 78))
        return pad::HighPunch;
    if (how != "attack" || f < 140)
        return 0;
    const int t = int(f % 60);
    if (t < 36)
        return side == 1 ? pad::Right : pad::Left;
    if ((t >= 36 && t < 39) || (t >= 44 && t < 47))
        return pad::HighPunch;
    if (t >= 52 && t < 55)
        return pad::LowPunch;
    return 0;
}

// MK3 and UMK3 keep their hidden fighters' unlocks in CMOS, where their own EJB menus put them: UMK3's ENABLE HIDDENS
// writes slots 1Fh, 20h and 21h (Mileena, Ermac, Classic Sub-Zero), MK3's SMOKE slot 10h, all with the value 0EDB0h.
// A slot is four CMOS words of a byte each, high byte first, at word 800h + 4 * slot; words 8B0h-8B1h hold the one's
// complement of the sum of the bytes of words 800h-8AFh, which the game checks at power-up. The game copies the
// unlocks to RAM at power-up, so a change made after it booted counts from its next boot (a reset).
enum class Unlock { Nothing, Done, NotYet };
Unlock UnlockHidden(const EmuCore& core, const QString& set)
{
    static const QHash<QString, QList<int>> kSlots = {{QStringLiteral("umk3"), {0x1F, 0x20, 0x21}},
                                                      {QStringLiteral("mk3"), {0x10}}};
    const auto hidden = kSlots.find(set);
    if (hidden == kSlots.end())
        return Unlock::Nothing;
    constexpr uint32_t kCmos = 0x01400000 / 8, kEnable = 0x01480000 / 8;  // byte addresses
    auto rd = [&](int w) { return int(core.cpuReadWord(kCmos + uint32_t(w) * 2) & 0xFF); };
    auto wr = [&](int w, int v) {
        core.cpuWriteWord(kEnable, 0);  // opens CMOS for one write, as the game does
        core.cpuWriteWord(kCmos + uint32_t(w) * 2, uint16_t(v));
    };
    auto sum = [&] {
        int s = 0;
        for (int w = 0x800; w < 0x8B0; w++)
            s += rd(w);
        return s;
    };
    if (((~sum()) & 0xFFFF) != (rd(0x8B0) << 8 | rd(0x8B1)))
        return Unlock::NotYet;  // not set up yet: a first boot restores the factory settings
    static const int kUnlocked[4] = {0x00, 0x00, 0xED, 0xB0};
    bool changed = false;
    for (int slot : *hidden)
        for (int i = 0; i < 4; i++) {
            const int w = 0x800 + slot * 4 + i;
            if (rd(w) != kUnlocked[i]) {
                wr(w, kUnlocked[i]);
                changed = true;
            }
        }
    if (!changed)
        return Unlock::Nothing;
    const int c = (~sum()) & 0xFFFF;
    wr(0x8B0, c >> 8);
    wr(0x8B1, c & 0xFF);
    return Unlock::Done;
}
// a first boot has restored its factory settings by then (MK3 and UMK3 at frame 116), and no game takes a START yet
constexpr uint64_t kUnlockFrame = 300;

// how colourful a picture is (Hasler and Suesstrunk), so a card shows a fighter rather than a black fade
double Colorfulness(const uint8_t* px, int w, int h, int pitch)
{
    double srg = 0, syb = 0, srg2 = 0, syb2 = 0, lum = 0;
    int n = 0;
    for (int y = 8; y < h - 8; y += 4) {
        const uint8_t* row = px + size_t(y) * pitch;
        for (int x = 0; x < w; x += 4) {
            const double b = row[x * 4 + 0], g = row[x * 4 + 1], r = row[x * 4 + 2];
            const double rg = r - g, yb = 0.5 * (r + g) - b;
            srg += rg;
            syb += yb;
            srg2 += rg * rg;
            syb2 += yb * yb;
            lum += 0.3 * r + 0.59 * g + 0.11 * b;
            n++;
        }
    }
    if (!n)
        return 0;
    const double mrg = srg / n, myb = syb / n;
    const double vrg = srg2 / n - mrg * mrg, vyb = syb2 / n - myb * myb;
    const double c = std::sqrt(std::max(0.0, vrg + vyb)) + 0.3 * std::sqrt(mrg * mrg + myb * myb);
    const double bright = lum / n / 255.0;
    return c * std::min(1.0, bright * 3.0);
}

uint32_t Fnv(const uint8_t* p, size_t n)
{
    uint64_t h = 1469598103934665603ull;
    size_t i = 0;
    for (; i + 8 <= n; i += 8) {
        uint64_t v;
        memcpy(&v, p + i, 8);
        h = (h ^ v) * 1099511628211ull;
    }
    for (; i < n; i++)
        h = (h ^ p[i]) * 1099511628211ull;
    return uint32_t(h ^ (h >> 32));
}

// --net-test: a busy pattern of moves for a side, repeatable from the frame number
uint16_t Scripted(int side, uint64_t frame)
{
    uint16_t b = 0;
    if (frame % 600 < 3)
        b |= pad::Start;
    const int phase = int((frame / (7 + side * 3)) % 6);
    static const uint16_t dirs[] = {pad::Left, pad::Right, pad::Up, pad::Down, pad::Right, pad::Left};
    b |= dirs[phase];
    if ((frame + side * 5) % 11 < 3)
        b |= uint16_t(pad::HighPunch << ((frame / 11 + side) % 6));
    return b;
}

struct Audio {
    SDL_AudioStream* stream = nullptr;
    int rate = 48000;
    float gain = -1;
    bool open(int r)
    {
        rate = r;
        SDL_AudioSpec spec{SDL_AUDIO_S16, 2, r};
        stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr);
        if (!stream)
            return false;
        SDL_ResumeAudioStreamDevice(stream);
        return true;
    }
    void close()
    {
        if (stream)
            SDL_DestroyAudioStream(stream);
        stream = nullptr;
    }
    void setGain(float g)
    {
        if (stream && g != gain) {
            SDL_SetAudioStreamGain(stream, g);
            gain = g;
        }
    }
    void put(const int16_t* s, int frames)
    {
        if (stream && s && frames > 0)
            SDL_PutAudioStreamData(stream, s, frames * 4);
    }
    int queuedFrames() const { return stream ? SDL_GetAudioStreamQueued(stream) / 4 : 0; }
    void clear()
    {
        if (stream)
            SDL_ClearAudioStream(stream);
    }
};

}  // namespace

// ----------------------------------------------------------------------------------------------------- GameSession

GameSession::GameSession(InputHub* input, QObject* parent) : QObject(parent), input_(input)
{
    if (!SDL_WasInit(SDL_INIT_AUDIO))
        SDL_InitSubSystem(SDL_INIT_AUDIO);
}

GameSession::~GameSession()
{
    link_ = 0;  // closing the app: nothing to tell
    stop();
}

void GameSession::post(std::function<void()> f) { QMetaObject::invokeMethod(this, std::move(f), Qt::QueuedConnection); }

void GameSession::fail(quint64 generation, const QString& why)
{
    post([this, generation, why] {
        if (generation != generation_.load())
            return;
        error_ = why;
        emit errorChanged();
        emit ended(why);
    });
}

void GameSession::play(const QString& set, const QString& mode) { launch(set, mode, nullptr); }

void GameSession::playNet(const QString& set, int side, int localPort, const QString& peer, int delay)
{
    auto net = std::make_shared<NetConfig>();
    net->side = side == 2 ? 2 : 1;
    net->localPort = localPort;
    net->peer = peer.trimmed().toStdString();
    net->delay = qBound(0, delay, 8);
    launch(set, QStringLiteral("net"), net);
}

void GameSession::playNetTest(const QString& set, int side, int localPort, const QString& peer, int delay,
                              int frames, const QString& log)
{
    auto net = std::make_shared<NetConfig>();
    net->side = side == 2 ? 2 : 1;
    net->localPort = localPort;
    net->peer = peer.trimmed().toStdString();
    net->delay = qBound(0, delay, 8);
    net->testFrames = qMax(1, frames);
    net->testLog = log;
    launch(set, QStringLiteral("net"), net);
}

quint64 GameSession::playLink(const LinkStart& s)
{
    auto net = std::make_shared<NetConfig>();
    net->side = s.side == 2 ? 2 : 1;
    net->localPort = s.localPort;
    net->peer = s.peer.trimmed().toStdString();
    net->delay = qBound(0, s.delay, 8);
    net->link = true;
    net->mode = s.mode;
    net->match = s.match;
    net->sessionDir = s.sessionDir;
    net->autoplay = autoplay_.toStdString();
    linkEnded_ = true;  // a match before this one (none should run) says nothing more
    launch(s.set, QStringLiteral("link"), net);
    link_ = generation_.load();
    linkMatch_ = s.match;
    linkMode_ = s.mode;
    linkResult_ = false;
    linkEnded_ = false;
    park_ = false;
    const QString p1 = s.p1Name.isEmpty() ? QStringLiteral("PLAYER 1") : s.p1Name;
    const QString p2 = s.p2Name.isEmpty() ? QStringLiteral("PLAYER 2") : s.p2Name;
    linkNames_ = p1 + QStringLiteral("  VS  ") + p2;
    emit stateChanged();
    emit linkStarted(s.set);
    return link_;
}

void GameSession::parkLink(quint64 link)
{
    if (link == 0)
        return;
    if (link == link_ && running_ && worker_.joinable()) {
        park_ = true;  // the game thread stops once the match's end has played; onLinkDone says "ready"
        return;
    }
    // it stopped already (by itself, or its player left): back at the hold point now
    if (link == link_) {
        linkEnded_ = true;
        stop();
    }
    emit linkStatus(QStringLiteral("ready"), link);
    emit linkParked();
}

void GameSession::dropLink(quint64 link)
{
    if (link == 0 || link != link_)
        return;
    linkEnded_ = true;
    stop();
}

void GameSession::onLinkLine(quint64 generation, const QString& line)
{
    if (generation != link_)
        return;
    if (line.startsWith(QLatin1String("result")))
        linkResult_ = true;
    else if (line.startsWith(QLatin1String("desync")) || line.startsWith(QLatin1String("disconnected")) ||
             line.startsWith(QLatin1String("sparring_over")) || line.startsWith(QLatin1String("error")))
        linkEnded_ = true;
    emit linkStatus(line, generation);
}

void GameSession::onLinkDone(quint64 generation, bool byItself)
{
    if (generation != link_ || generation != generation_.load())
        return;
    if (worker_.joinable())
        worker_.join();
    running_ = false;
    if (park_.exchange(false)) {
        const quint64 link = link_;
        link_ = 0;
        set_.clear();
        mode_.clear();
        linkNames_.clear();
        emit stateChanged();
        emit linkStatus(QStringLiteral("ready"), link);
        emit linkParked();
        return;
    }
    // stopped by itself: its last status stands (a result, desync, disconnected) until the server's park
    if (byItself)
        linkEnded_ = true;
    emit stateChanged();
}

void GameSession::launch(const QString& set, const QString& mode, std::shared_ptr<NetConfig> net)
{
    stop();
    set_ = set;
    mode_ = mode;
    error_.clear();
    netStatus_ = net ? (net->testFrames > 0 ? QStringLiteral("Connecting") : QStringLiteral("Loading the match"))
                     : QString();
    netPing_ = netRollback_ = 0;
    lastNet_ = net;
    paused_ = false;
    stop_ = false;
    running_ = true;
    const quint64 gen = ++generation_;
    emit errorChanged();
    emit stateChanged();
    emit netChanged();
    worker_ = std::thread([this, set, mode, net, gen] { run(set, mode, net, gen); });
}

void GameSession::stop()
{
    const quint64 link = link_;
    if (worker_.joinable()) {
        stop_ = true;
        worker_.join();
    }
    link_ = 0;
    if (running_ || !set_.isEmpty()) {
        running_ = false;
        set_.clear();
        mode_.clear();
        linkNames_.clear();
        paused_ = false;
        emit stateChanged();
    }
    if (link) {
        // the player left the online match (its pause menu, or cancelled the wait): the protocol's game closed. After
        // its result that is only an early park; in Online Practice its end
        const bool quiet = linkEnded_ || park_.exchange(false);
        linkEnded_ = false;
        if (!quiet) {
            if (linkResult_)
                emit linkStatus(QStringLiteral("ready"), link);
            else if (linkMode_ == QLatin1String("sparring"))
                emit linkStatus(QStringLiteral("sparring_over match=") + linkMatch_, link);
            else
                emit linkStatus(QStringLiteral("error exited"), link);
        }
    }
}

void GameSession::setPaused(bool p)
{
    if (mode_ == QLatin1String("net") || mode_ == QLatin1String("link"))
        return;  // the other player's game would have to stop too
    if (paused_.exchange(p) != p)
        emit stateChanged();
}

void GameSession::restart()
{
    if (running_ && mode_ != QLatin1String("net") && mode_ != QLatin1String("link"))
        restart_ = true;
}

// --------------------------------------------------------------------------------------------- the game thread

void GameSession::run(QString set, QString mode, std::shared_ptr<NetConfig> net, quint64 generation)
{
    Settings* settings = Settings::instance();
    const bool preview = mode == QLatin1String("preview");
    const bool versus = mode == QLatin1String("versus");
    const bool test = net && net->testFrames > 0;
    const bool link = net && net->link;
    const bool sparring = link && net->mode == QLatin1String("sparring");
    timeBeginPeriod(1);

    // a server's match: what happens goes to its session's game.log, and its status lines to its GameProcess
    FILE* gameLog = nullptr;
    if (link && !net->sessionDir.isEmpty())
        gameLog = _wfopen(reinterpret_cast<const wchar_t*>((net->sessionDir + QStringLiteral("/game.log")).utf16()), L"a");
    auto note = [&](const QString& line) {
        if (!gameLog)
            return;
        const QString stamp = QDateTime::currentDateTime().toString(QStringLiteral("hh:mm:ss.zzz "));
        fprintf(gameLog, "%s%s\n", stamp.toUtf8().constData(), line.toUtf8().constData());
        fflush(gameLog);
    };
    auto say = [&](const QString& line) {
        note(QStringLiteral("status: ") + line);
        if (link)
            post([this, generation, line] { onLinkLine(generation, line); });
    };
    bool linkByItself = false;  // link: the game stopped by itself rather than for a park or its player
    auto linkDone = [&] {
        if (gameLog)
            fclose(gameLog);
        gameLog = nullptr;
        if (link)
            post([this, generation, linkByItself] { onLinkDone(generation, linkByItself); });
    };
    if (link)
        note(QStringLiteral("match %1: %2, side %3, %4 from port %5, delay %6")
                 .arg(net->match, set).arg(net->side).arg(QString::fromStdString(net->peer)).arg(net->localPort)
                 .arg(net->delay));

    // battery RAM: kept per set for local play; online both games start from none, so they boot alike
    std::unique_ptr<QTemporaryDir> netNvram;
    QString nvramDir = settings->nvramDir();
    if (net) {
        netNvram = std::make_unique<QTemporaryDir>();
        nvramDir = netNvram->path();
    }
    EmuCore core;
    QString err;
    if (!core.load(settings->coreDll(), &err) ||
        !core.startGame(settings->romDir(), nvramDir, settings->supportDir(), set, &err)) {
        say(QStringLiteral("error ") + QString(err).replace(QLatin1Char('\n'), QLatin1Char(' ')));
        fail(generation, err);
        linkByItself = true;
        linkDone();
        timeEndPeriod(1);
        return;
    }

    // the hidden fighters (MK3, UMK3): unlocked in CMOS before the game first reads it, or, on a first boot (no
    // battery RAM yet, as every online match has), once it has restored its factory settings, and then it boots again
    bool unlockLater = UnlockHidden(core, set) == Unlock::NotYet;
    auto unlockAfterBoot = [&] {
        unlockLater = false;
        if (UnlockHidden(core, set) == Unlock::Done) {
            core.reset();
            note(QStringLiteral("hidden fighters unlocked in CMOS: the game boots again"));
        }
    };

    Audio audio;
    if (!test)
        audio.open(core.sampleRate());
    const double fps = core.fps() > 1 ? core.fps() : 60.0;
    const auto period = std::chrono::duration<double>(1.0 / fps);
    const int samplesPerFrame = int(core.sampleRate() / fps);
    Pacer pacer;
    CoinPilot coins[2];

    // --- netplay setup
    // Both games first run the same power-on by themselves: a driver's first frames do one-time set-up its saved state
    // does not hold (MK2's desynced when a rollback reached back into them), so netplay's frame 0 starts after it.
    // A match (not --net-test, which keeps the short one it was checked with) runs on to both players joined: about
    // 2400 frames, a second or so undrawn, and netplay starts on the select screen.
    const bool joinFirst = net && !test;
    const uint64_t preroll = !net ? 0 : joinFirst ? JoinPilot::kLength : 300;
    const auto prerollStart = Clock::now();
    for (uint64_t f = 0; f < preroll && !stop_.load(); f++) {
        uint16_t p1 = 0, p2 = 0;
        if (joinFirst)
            JoinPilot::at(f, p1, p2);
        else
            p1 = p2 = BootPilot(f, true);
        if (joinFirst && unlockLater && f == kUnlockFrame)
            unlockAfterBoot();  // the same frame on both PCs: they stay alike
        core.setPads(p1, p2);
        core.runFrame(false);
    }
    if (net) {
        note(QStringLiteral("power-on and join: %1 frames in %2 ms")
                 .arg(preroll)
                 .arg(std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - prerollStart).count()));
        post([this, generation] {
            if (generation != generation_.load())
                return;
            netStatus_ = QStringLiteral("Waiting for the other player");
            emit netChanged();
        });
    }
    GekkoSession* gekko = nullptr;
    int localHandle = 0, remoteHandle = 1;
    uint32_t stateSize = core.stateSize();
    FILE* testLog = nullptr;
    if (net) {
        gekko_create(&gekko, GekkoGameSession);
        GekkoConfig cfg{};
        cfg.num_players = 2;
        cfg.max_spectators = 0;
        cfg.max_input_prediction_window = 12;
        cfg.input_size = sizeof(uint16_t);
        cfg.state_size = stateSize;
        cfg.desync_detection = true;
        gekko_start(gekko, &cfg);
        gekko_net_adapter_set(gekko, gekko_default_adapter(uint16_t(net->localPort)));
        GekkoNetAddress addr{};
        addr.data = (void*)net->peer.c_str();
        addr.size = unsigned(net->peer.size());
        if (net->side == 1) {
            localHandle = gekko_add_actor(gekko, GekkoLocalPlayer, nullptr);
            remoteHandle = gekko_add_actor(gekko, GekkoRemotePlayer, &addr);
        } else {
            remoteHandle = gekko_add_actor(gekko, GekkoRemotePlayer, &addr);
            localHandle = gekko_add_actor(gekko, GekkoLocalPlayer, nullptr);
        }
        gekko_set_local_delay(gekko, localHandle, uint8_t(net->delay));
        gekko_set_disconnect_timeout(gekko, 6000);
        if (test && !net->testLog.isEmpty())
            testLog = _wfopen(reinterpret_cast<const wchar_t*>(net->testLog.utf16()), L"w");
    }

    auto present = [&]() {
        frames_.put(core.video(), core.width(), core.height(), core.pitch());
        if (!test)
            emit frameReady();
    };
    auto putAudio = [&]() {
        if (audio.stream) {
            const int vol = preview ? settings->volume() * settings->menuVolume() / 100 : settings->volume();
            audio.setGain(vol / 100.0f);
            audio.put(core.audio(), core.audioFrames());
            // keep about three frames queued: drop a burst that piled up (after a stall)
            if (audio.queuedFrames() > samplesPerFrame * 10)
                audio.clear();
        }
    };

    uint64_t frame = 0;
    double bestArt = 0;
    QImage bestImg;
    auto next = Clock::now();
    bool wasPaused = false;
    bool playerStarted = false;  // local play: a player pressed START, the boot's BLOCK presses are over
    int statsTick = 0, maxRollback = 0, rolledThisTick = 0, deepestRollback = 0;
    uint64_t rolledTotal = 0;
    std::map<uint64_t, uint32_t> pendingHash;
    bool started = !net;
    QString lastStatus;
    // link: the match's result, read from the round wins on frames shown (a rollback runs a frame again before it is)
    const RoundRam* rounds = link && !sparring ? RoundRamFor(set) : nullptr;
    if (link && !sparring && !rounds)
        note(QStringLiteral("no round counters known for %1: this match reports no result").arg(set));
    int resultWinner = 0, resultFrames = 0;
    bool resultSaid = false, inMatchSaid = false;
    Clock::time_point resultAt;
    const QString matchKey = link ? QStringLiteral(" match=") + net->match : QString();

    while (!stop_.load()) {
        // the server's park: the match's end plays a few seconds after the result, then this game goes
        if (link && park_.load() && (!resultSaid || Clock::now() - resultAt >= kAfterResult)) {
            note(QStringLiteral("parked"));
            break;
        }
        if (restart_.exchange(false) && !net) {
            core.unload();
            if (!core.load(settings->coreDll(), &err) ||
                !core.startGame(settings->romDir(), nvramDir, settings->supportDir(), set, &err)) {
                fail(generation, err);
                break;
            }
            frame = 0;
            playerStarted = false;
            unlockLater = UnlockHidden(core, set) == Unlock::NotYet;
            audio.clear();
            next = Clock::now();
        }
        if (paused_.load() && !net) {
            if (!wasPaused)
                audio.clear();
            wasPaused = true;
            Sleep(10);
            next = Clock::now();
            continue;
        }
        wasPaused = false;

        if (!net) {
            if (unlockLater && frame == kUnlockFrame)
                unlockAfterBoot();
            uint16_t p1 = 0, p2 = 0;
            if (!preview) {
                const uint16_t raw1 = input_->padState(0), raw2 = versus ? input_->padState(1) : 0;
                // from a player's first START on, the game is theirs: the boot's BLOCK presses below would pick the
                // fighter under the cursor on the select screen (and UMK3's mode) for them
                if ((raw1 | raw2) & pad::Start)
                    playerStarted = true;
                p1 = coins[0].apply(raw1);
                p2 = versus ? coins[1].apply(raw2) : 0;
            }
            // every boot, not only a first one: battery RAM saved before the game had restored its settings (a
            // preview stopped early) brings the CMOS screen back, and nothing else would press past it. That screen
            // is up within 2 s (MK1 and MK2 at once, MK3 and UMK3 at frame 116), so the presses of the first 5 s
            // (frames 120 and 270) stay whatever the player does; no game takes a START that early
            const uint16_t boot = playerStarted && frame >= 300 ? 0 : BootPilot(frame, true);
            core.setPads(p1 | boot, p2 | boot);
            core.runFrame(true);
            present();
            putAudio();
            if (preview && frame % 110 == 55) {
                const double s = Colorfulness(core.video(), core.width(), core.height(), core.pitch());
                if (s > bestArt) {
                    bestArt = s;
                    bestImg = QImage(core.width(), core.height(), QImage::Format_RGB32);
                    for (int y = 0; y < core.height(); y++)
                        memcpy(bestImg.scanLine(y), core.video() + size_t(y) * core.pitch(), size_t(core.width()) * 4);
                }
            }
            frame++;
        } else {
            gekko_network_poll(gekko);
            uint16_t mine = test                    ? Scripted(net->side, frame)
                            : !net->autoplay.empty() ? Autoplay(net->autoplay, net->side, frame)
                                                     : input_->padState(0);
            mine = coins[0].apply(mine);
            gekko_add_local_input(gekko, localHandle, &mine);

            int count = 0;
            GekkoSessionEvent** sev = gekko_session_events(gekko, &count);
            for (int i = 0; i < count; i++) {
                switch (sev[i]->type) {
                case GekkoPlayerSyncing:
                    lastStatus = QStringLiteral("Syncing with the other player");
                    break;
                case GekkoPlayerConnected:
                case GekkoSessionStarted:
                    lastStatus = QStringLiteral("Connected");
                    started = true;
                    if (link && !inMatchSaid) {
                        inMatchSaid = true;
                        say(QStringLiteral("in_match") + matchKey);
                    }
                    break;
                case GekkoPlayerDisconnected:
                    if (link) {
                        linkByItself = true;
                        if (resultSaid) {
                            note(QStringLiteral("the other game left after the result"));
                        } else {
                            // Online Practice ends when one leaves; a match ends without a result
                            say((sparring ? QStringLiteral("sparring_over") : QStringLiteral("disconnected")) + matchKey);
                            fail(generation, QStringLiteral("The other player left"));
                        }
                    } else {
                        fail(generation, QStringLiteral("The other player left"));
                    }
                    stop_ = true;
                    break;
                case GekkoDesyncDetected: {
                    const auto d = sev[i]->data.desynced;
                    if (testLog)
                        fprintf(testLog, "DESYNC frame %d local %08x remote %08x\n", d.frame, d.local_checksum,
                                d.remote_checksum);
                    if (link) {
                        linkByItself = true;
                        note(QStringLiteral("desync at frame %1: local %2, remote %3")
                                 .arg(d.frame)
                                 .arg(d.local_checksum, 8, 16, QLatin1Char('0'))
                                 .arg(d.remote_checksum, 8, 16, QLatin1Char('0')));
                        if (!resultSaid)
                            say(QStringLiteral("desync frame=%1").arg(d.frame) + matchKey);
                    }
                    if (!link || !resultSaid)
                        fail(generation, QStringLiteral("The two games went out of step at frame %1").arg(d.frame));
                    stop_ = true;
                    break;
                }
                default:
                    break;
                }
            }

            int n = 0;
            GekkoGameEvent** ev = gekko_update_session(gekko, &n);
            int rolled = 0;
            for (int i = 0; i < n && !stop_.load(); i++) {
                GekkoGameEvent* e = ev[i];
                switch (e->type) {
                case GekkoSaveEvent: {
                    const uint32_t len = core.saveState(e->data.save.state, stateSize);
                    *e->data.save.state_len = len;
                    *e->data.save.checksum = Fnv(e->data.save.state, len);
                    break;
                }
                case GekkoLoadEvent:
                    core.loadState(e->data.load.state, e->data.load.state_len);
                    break;
                case GekkoAdvanceEvent: {
                    const uint16_t* in = reinterpret_cast<const uint16_t*>(e->data.adv.inputs);
                    const uint64_t f = uint64_t(e->data.adv.frame);
                    const uint16_t boot = BootPilot(preroll + f, true);
                    core.setPads(in[0] | boot, in[1] | boot);
                    const bool real = !e->data.adv.rolling_back;
                    core.runFrame(real);
                    // a checkpoint's hash is kept from the frame's last run (a rollback runs it again with the
                    // other player's real input) and written once no rollback can reach it any more
                    if (testLog && f % 300 == 0) {
                        std::vector<uint8_t> s(stateSize);
                        const uint32_t len = core.saveState(s.data(), stateSize);
                        pendingHash[f] = Fnv(s.data(), len);
                    }
                    if (real && rounds && !resultSaid) {
                        uint8_t ra = 0, rb = 0;
                        core.readRam(rounds->p1, &ra, 1);
                        core.readRam(rounds->p2, &rb, 1);
                        // MK1 marks a player who lost a round without a win of their own as 255: no wins
                        const int a = ra <= 3 ? ra : 0, b = rb <= 3 ? rb : 0;
                        const int w = a >= 2 && a > b ? 1 : b >= 2 && b > a ? 2 : 0;
                        if (w != 0 && w == resultWinner) {
                            if (++resultFrames >= kResultFrames) {
                                resultSaid = true;
                                resultAt = Clock::now();
                                note(QStringLiteral("result at frame %1").arg(f));
                                say(QStringLiteral("result winner=%1 p1_rounds=%2 p2_rounds=%3").arg(w).arg(a).arg(b) +
                                    matchKey);
                            }
                        } else {
                            resultWinner = w;
                            resultFrames = w ? 1 : 0;
                        }
                    }
                    if (real) {
                        present();
                        putAudio();
                        for (auto it = pendingHash.begin(); it != pendingHash.end();) {
                            if (f >= it->first + 40) {
                                fprintf(testLog, "frame %llu state %08x\n", (unsigned long long)it->first, it->second);
                                fflush(testLog);
                                it = pendingHash.erase(it);
                            } else {
                                ++it;
                            }
                        }
                        frame = f + 1;
                    } else {
                        rolled++;
                    }
                    break;
                }
                default:
                    break;
                }
            }
            rolledThisTick = rolled;
            rolledTotal += rolled;
            maxRollback = std::max(maxRollback, rolled);
            deepestRollback = std::max(deepestRollback, rolled);
            if (test && frame >= uint64_t(net->testFrames))
                break;
            if (++statsTick >= 27) {
                statsTick = 0;
                GekkoNetworkStats st{};
                gekko_network_stats(gekko, remoteHandle, &st);
                const int ping = int(st.avg_ping + 0.5f);
                const int rb = maxRollback;
                const QString status = started ? QStringLiteral("Online") : lastStatus.isEmpty()
                                                     ? QStringLiteral("Waiting for the other player") : lastStatus;
                maxRollback = 0;
                post([this, generation, ping, rb, status] {
                    if (generation != generation_.load())
                        return;
                    netPing_ = ping;
                    netRollback_ = rb;
                    netStatus_ = status;
                    emit netChanged();
                });
            }
        }

        // pacing: the board's own rate, nudged by the sound queue (locally) or by how far ahead of the other
        // game this one runs (online)
        double scale = 1.0;
        if (net) {
            if (gekko_frames_ahead(gekko) > 0.5f)
                scale = 1.016;
        } else if (audio.stream) {
            const int q = audio.queuedFrames();
            if (q > samplesPerFrame * 4)
                scale = 1.004;
            else if (q < samplesPerFrame * 2)
                scale = 0.996;
        }
        next += std::chrono::duration_cast<Clock::duration>(period * scale);
        const auto now = Clock::now();
        if (now - next > std::chrono::milliseconds(150))
            next = now;  // fell behind (a stall): start counting again from now
        pacer.until(next);  // a test runs in real time too, as two PCs would
        (void)rolledThisTick;
    }
    if (test)
        post([this, generation] {
            if (generation == generation_.load())
                emit ended(QStringLiteral("test finished"));
        });

    // the attract mode's most colourful moment becomes the game's card, when it beats the one kept so far
    if (preview && !bestImg.isNull()) {
        QSettings scores(settings->artDir() + "/scores.ini", QSettings::IniFormat);
        if (bestArt > scores.value(set, 0.0).toDouble() + 1.0) {
            if (bestImg.save(settings->artDir() + "/" + set + ".png")) {
                scores.setValue(set, bestArt);
                post([this, set] { emit artChanged(set); });
            }
        }
    }
    if (gekko) {
        gekko_destroy(&gekko);
        gekko_default_adapter_destroy();
    }
    if (testLog) {
        fprintf(testLog, "frames %llu, frames run again by rollbacks %llu, deepest rollback %d\n",
                (unsigned long long)frame, (unsigned long long)rolledTotal, deepestRollback);
        fclose(testLog);
    }
    audio.close();
    core.unload();
    linkDone();  // the game port is free again before its GameProcess hears of it
    timeEndPeriod(1);
}
