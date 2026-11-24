// What the Midway-only core leaves out and the shared burner code still names: CD images (no CD system is built),
// hard disk images (only Killer Instinct has one) and artwork overlays (bezels, .lay files: burn_artwork.cpp needs
// plutosvg, and the games draw their own frame). Linked into mka_fbneo.dll, subject to the FB Neo license.
#include "burner.h"
#include "burnint.h"

// --- artwork: never active, the driver's own picture and aspect are used as they are
INT32 BurnArtworkActive() { return 0; }
INT32 BurnArtworkSetLayered(INT32) { return 0; }
void BurnArtworkInit() {}
void BurnArtworkExit() {}
void BurnArtworkFrameBegin() {}
void BurnArtworkFrameEnd() {}
void BurnArtworkScan(INT32) {}
INT32 BurnArtworkGetDisplaySize(INT32*, INT32*) { return 0; }
INT32 BurnArtworkGetAspect(INT32*, INT32*) { return 0; }
INT32 BurnArtworkGetBaseSize(INT32*, INT32*) { return 0; }
INT32 BurnArtworkSetTargetSize(INT32, INT32) { return 0; }
void BurnArtworkNativeSizeChanged() {}
INT32 BurnOutputFind(const char*) { return -1; }
void BurnOutputSetHandle(INT32, INT32) {}
void BurnOutputSet(const char*, INT32) {}
void BurnOutputSetIndexed(const char*, INT32, INT32) {}
INT32 BurnOutputGet(const char*) { return 0; }
void BurnOutputReset() {}

namespace {
INT32 NoCd() { return 1; }
INT32 NoCdPlay(UINT8, UINT8, UINT8) { return 1; }
INT32 NoCdSector(INT32, char*) { return 0; }
INT32 NoCdData(INT32, UINT8*) { return 0; }
UINT8* NoCdToc(INT32) { return nullptr; }
UINT8* NoCdQ() { return nullptr; }
INT32 NoCdVolume(double) { return 0; }
INT32 NoCdLba() { return 0; }
INT32 NoCdSound(INT16*, INT32) { return 0; }
INT32 NoCdScan(INT32, INT32*) { return 0; }
INT32 NoCdSettings(InterfaceInfo*) { return 0; }
}  // namespace

struct CDEmuDo cdimgDo = {NoCd,     NoCd,     NoCd,       NoCdPlay, NoCdSector, NoCdData,     NoCdToc,
                          NoCdQ,    NoCdVolume, NoCdLba,  NoCdSound, NoCdScan,  NoCdSettings, _T("no CD")};

// no CD game is ever identified (cdlist.cpp is left out)
TCHAR* CDInfo_GamePrefix() { return nullptr; }
TCHAR* CDInfo_Text(INT32) { return nullptr; }

// no hard disk images: every lookup misses
INT32 BurnHDDGetPath(INT32, const TCHAR*, TCHAR* pszPath)
{
    if (pszPath)
        pszPath[0] = 0;
    return 1;
}
