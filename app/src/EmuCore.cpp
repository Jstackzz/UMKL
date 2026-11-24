#include "EmuCore.h"

#include <QDir>

#include <windows.h>

template <class T>
bool EmuCore::bind(T& fn, const char* name, QString* error)
{
    fn = reinterpret_cast<T>(GetProcAddress(reinterpret_cast<HMODULE>(dll_), name));
    if (!fn && error)
        *error = QStringLiteral("the emulation core has no %1").arg(QLatin1String(name));
    return fn != nullptr;
}

bool EmuCore::load(const QString& dllPath, QString* error)
{
    unload();
    dll_ = reinterpret_cast<HINSTANCE__*>(LoadLibraryW(reinterpret_cast<LPCWSTR>(
        QDir::toNativeSeparators(dllPath).utf16())));
    if (!dll_) {
        if (error)
            *error = QStringLiteral("cannot load the emulation core %1 (error %2)").arg(dllPath).arg(GetLastError());
        return false;
    }
    bool ok = bind(init_, "FBHeadlessInit", error) && bind(exit_, "FBHeadlessExit", error) &&
              bind(setRomPath_, "FBHeadlessSetRomPath", error) && bind(loadDriver_, "FBHeadlessLoadDriver", error) &&
              bind(runFrame_, "FBHeadlessRunFrame", error) && bind(videoBuffer_, "FBHeadlessGetVideoBuffer", error) &&
              bind(videoWidth_, "FBHeadlessGetVideoWidth", error) &&
              bind(videoHeight_, "FBHeadlessGetVideoHeight", error) &&
              bind(videoPitch_, "FBHeadlessGetVideoPitch", error) &&
              bind(audioRate_, "FBHeadlessGetAudioSampleRate", error) &&
              bind(audioBuffer_, "FBHeadlessGetAudioBuffer", error) &&
              bind(audioFrames_, "FBHeadlessGetAudioFrames", error) &&
              bind(frameRate_, "FBHeadlessGetFrameRate", error) && bind(setInput_, "FBHeadlessSetInput", error) &&
              bind(stateSize_, "MKA_StateSize", error) && bind(stateSave_, "MKA_StateSave", error) &&
              bind(stateLoad_, "MKA_StateLoad", error) && bind(setPaths_, "MKA_SetPaths", error) &&
              bind(driverCount_, "MKA_DriverCount", error) && bind(driverText_, "MKA_DriverText", error) &&
              bind(driverIndex_, "MKA_DriverIndex", error) && bind(driverRom_, "MKA_DriverRom", error) &&
              bind(cheevosInit_, "FBHeadlessCheevosInit", error) &&
              bind(readRam_, "FBHeadlessReadAchievementMemory", error);
    if (!ok) {
        unload();
        return false;
    }
    init_();
    return true;
}

void EmuCore::unload()
{
    if (!dll_)
        return;
    if (exit_)
        exit_();
    FreeLibrary(reinterpret_cast<HMODULE>(dll_));
    dll_ = nullptr;
    running_ = false;
    lastPads_[0] = lastPads_[1] = 0xFFFF;
}

static QByteArray Dir8(const QString& dir)
{
    QString d = QDir::fromNativeSeparators(dir);
    if (!d.endsWith(QLatin1Char('/')))
        d += QLatin1Char('/');
    return d.toLocal8Bit();
}

bool EmuCore::startGame(const QString& romDir, const QString& nvramDir, const QString& supportDir,
                        const QString& set, QString* error)
{
    if (!dll_) {
        if (error)
            *error = QStringLiteral("the emulation core is not loaded");
        return false;
    }
    QDir().mkpath(nvramDir);
    setPaths_(Dir8(nvramDir).constData(), Dir8(supportDir).constData());
    setRomPath_(Dir8(romDir).constData());
    if (loadDriver_(set.toLatin1().constData()) != 0) {
        if (error)
            *error = QStringLiteral("%1 did not start: its ROM zip is missing or incomplete in %2")
                         .arg(set, QDir::toNativeSeparators(romDir));
        return false;
    }
    cheevosInit_();
    running_ = true;
    return true;
}

void EmuCore::setPads(uint16_t p1, uint16_t p2)
{
    const uint16_t pads[2] = {p1, p2};
    for (int p = 0; p < 2; p++) {
        const uint16_t changed = pads[p] ^ lastPads_[p];
        if (!changed)
            continue;
        for (int b = 0; b < pad::kButtons; b++)
            if (changed & (1u << b))
                setInput_(p, b, (pads[p] >> b) & 1);
        lastPads_[p] = pads[p];
    }
}

int EmuCore::driverIndex(const QString& set) const
{
    return dll_ ? driverIndex_(set.toLatin1().constData()) : -1;
}

QString EmuCore::driverText(int index, int what) const
{
    return dll_ ? QString::fromLatin1(driverText_(index, what)) : QString();
}

bool EmuCore::driverRom(int index, int rom, Rom* out) const
{
    if (!dll_)
        return false;
    const char* name = nullptr;
    unsigned len = 0, crc = 0, type = 0;
    if (!driverRom_(index, rom, &name, &len, &crc, &type))
        return false;
    out->name = QString::fromLatin1(name ? name : "");
    out->length = len;
    out->crc = crc;
    out->type = type;
    return true;
}
