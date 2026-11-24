#pragma once
// The emulation core, mka_fbneo.dll (core/), loaded at run time. FBNeo keeps its machine in globals, so one loaded
// copy runs one game: a new game unloads the DLL and loads it again, which starts it from clean globals.
#include <QString>

#include <cstdint>

struct HINSTANCE__;

// The arcade inputs of one player, as bits. Their order is FBNeo's headless button order, so bit i is button i.
namespace pad {
enum : uint16_t {
    Up = 1 << 0,
    Down = 1 << 1,
    Left = 1 << 2,
    Right = 1 << 3,
    HighPunch = 1 << 4,  // FBNeo "fire 1"
    Block = 1 << 5,      // "fire 2"
    HighKick = 1 << 6,   // "fire 3"
    LowPunch = 1 << 7,   // "fire 4"
    LowKick = 1 << 8,    // "fire 5"
    Run = 1 << 9,        // "fire 6" (MK3, UMK3)
    Start = 1 << 10,
    Coin = 1 << 11,
};
constexpr int kButtons = 12;
constexpr uint16_t kAttacks = HighPunch | Block | HighKick | LowPunch | LowKick | Run;
}  // namespace pad

class EmuCore {
public:
    ~EmuCore() { unload(); }

    // loads the DLL; false with *error set when it or one of its functions is missing
    bool load(const QString& dllPath, QString* error);
    void unload();
    bool loaded() const { return dll_ != nullptr; }

    // after load(): the folders the core reads ROM zips from and keeps battery RAM in, then a set by short name
    bool startGame(const QString& romDir, const QString& nvramDir, const QString& supportDir, const QString& set,
                   QString* error);
    bool running() const { return running_; }

    void setPads(uint16_t p1, uint16_t p2);
    void runFrame(bool draw) { runFrame_(draw ? 1 : 0); }

    const uint8_t* video() const { return videoBuffer_(); }
    int width() const { return videoWidth_(); }
    int height() const { return videoHeight_(); }
    int pitch() const { return videoPitch_(); }
    const int16_t* audio() const { return reinterpret_cast<const int16_t*>(audioBuffer_()); }
    int audioFrames() const { return audioFrames_(); }
    int sampleRate() const { return audioRate_(); }
    double fps() const { return frameRate_() / 100.0; }

    uint32_t stateSize() const { return stateSize_(); }
    uint32_t saveState(void* dst, uint32_t cap) const { return stateSave_(dst, cap); }
    bool loadState(const void* src, uint32_t len) const { return stateLoad_(src, len) == 0; }

    // the main RAM as FBNeo's achievement map lays it out (flat, from 0)
    uint32_t readRam(uint32_t addr, uint8_t* out, uint32_t n) const { return readRam_(addr, out, n); }
    // the main CPU's memory by byte address, through the game's own handlers (CMOS behind its write-enable latch)
    uint16_t cpuReadWord(uint32_t byteAddr) const { return uint16_t(cpuRead_(byteAddr)); }
    void cpuWriteWord(uint32_t byteAddr, uint16_t value) const { cpuWrite_(byteAddr, value); }
    // the machine's reset button: the game boots again, keeping its CMOS
    void reset() const { reset_(); }

    // the driver tables (valid while no game runs, or for the running game)
    int driverIndex(const QString& set) const;
    QString driverText(int index, int what) const;
    struct Rom {
        QString name;
        uint32_t length = 0, crc = 0, type = 0;
    };
    bool driverRom(int index, int rom, Rom* out) const;

private:
    template <class T>
    bool bind(T& fn, const char* name, QString* error);

    HINSTANCE__* dll_ = nullptr;
    bool running_ = false;
    uint16_t lastPads_[2] = {0xFFFF, 0xFFFF};

    int (*init_)() = nullptr;
    void (*exit_)() = nullptr;
    void (*setRomPath_)(const char*) = nullptr;
    int (*loadDriver_)(const char*) = nullptr;
    void (*runFrame_)(int) = nullptr;
    const unsigned char* (*videoBuffer_)() = nullptr;
    int (*videoWidth_)() = nullptr;
    int (*videoHeight_)() = nullptr;
    int (*videoPitch_)() = nullptr;
    int (*audioRate_)() = nullptr;
    const short* (*audioBuffer_)() = nullptr;
    int (*audioFrames_)() = nullptr;
    int (*frameRate_)() = nullptr;
    void (*setInput_)(int, int, int) = nullptr;
    unsigned (*stateSize_)() = nullptr;
    unsigned (*stateSave_)(void*, unsigned) = nullptr;
    int (*stateLoad_)(const void*, unsigned) = nullptr;
    void (*setPaths_)(const char*, const char*) = nullptr;
    int (*driverCount_)() = nullptr;
    const char* (*driverText_)(int, int) = nullptr;
    int (*driverIndex_)(const char*) = nullptr;
    int (*driverRom_)(int, int, const char**, unsigned*, unsigned*, unsigned*) = nullptr;
    void (*cheevosInit_)() = nullptr;
    unsigned (*readRam_)(unsigned, unsigned char*, unsigned) = nullptr;
    unsigned (*cpuRead_)(unsigned) = nullptr;
    void (*cpuWrite_)(unsigned, unsigned) = nullptr;
    void (*reset_)() = nullptr;
};
