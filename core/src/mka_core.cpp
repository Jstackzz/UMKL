// MK Arcade Online's bridge into FBNeo's headless core: whole-machine states kept in memory (what rollback saves and
// loads every frame), the driver and ROM tables for checking a player's ROM sets, and the paths the core writes to.
// Linked into mka_fbneo.dll with FBNeo, so subject to the FB Neo license (external/FBNeo/src/license.txt).
#include "burner.h"
#include "fbheadless.h"

#include <cstring>

namespace {

enum class Mode { Measure, Save, Load };

Mode g_mode = Mode::Measure;
unsigned char* g_buf = nullptr;
size_t g_cap = 0;
size_t g_pos = 0;
bool g_overflow = false;
unsigned g_areas = 0;

INT32 __cdecl ScanArea(struct BurnArea* pba)
{
    const size_t n = pba->nLen;
    g_areas++;
    if (g_mode == Mode::Measure) {
        g_pos += n;
        return 0;
    }
    if (g_pos + n > g_cap) {
        g_overflow = true;
        g_pos += n;
        return 0;
    }
    if (g_mode == Mode::Save)
        memcpy(g_buf + g_pos, pba->Data, n);
    else
        memcpy(pba->Data, g_buf + g_pos, n);
    g_pos += n;
    return 0;
}

// One pass over every area the driver scans (RAM, NVRAM, CPU and sound chip states, driver variables), in the same
// order each time; returns the bytes it covered, 0 when the buffer was too small.
size_t Scan(Mode mode, unsigned char* buf, size_t cap)
{
    g_mode = mode;
    g_buf = buf;
    g_cap = cap;
    g_pos = 0;
    g_overflow = false;
    g_areas = 0;
    INT32(__cdecl * previous)(struct BurnArea*) = BurnAcb;
    BurnAcb = ScanArea;
    INT32 min = 0;
    BurnAreaScan(ACB_FULLSCAN | (mode == Mode::Load ? ACB_WRITE : ACB_READ), &min);
    BurnAcb = previous;
    return g_overflow ? 0 : g_pos;
}

void CopyDir(TCHAR* dst, const char* src)
{
    if (!src || !*src)
        return;
    size_t n = strlen(src);
    if (n > MAX_PATH - 2)
        n = MAX_PATH - 2;
    memcpy(dst, src, n);
    if (src[n - 1] != '/' && src[n - 1] != '\\')
        dst[n++] = '/';
    dst[n] = 0;
}

// The driver at index, made active only for the call when no game runs (the tables are read through the active
// driver); -1 when out of range or while a game runs and index is another driver.
template <class F>
auto WithDriver(int index, F f, decltype(f()) fallback) -> decltype(f())
{
    if (index < 0 || (UINT32)index >= nBurnDrvCount)
        return fallback;
    if (bDrvOkay && (UINT32)index != nBurnDrvActive)
        return fallback;
    const UINT32 saved = nBurnDrvActive;
    nBurnDrvActive = (UINT32)index;
    auto r = f();
    nBurnDrvActive = saved;
    return r;
}

}  // namespace

extern "C" {

unsigned MKA_Version(void) { return 1; }

// --- whole-machine states, in memory
unsigned MKA_StateSize(void)
{
    return bDrvOkay ? (unsigned)Scan(Mode::Measure, nullptr, 0) : 0;
}

unsigned MKA_StateSave(void* dst, unsigned cap)
{
    if (!bDrvOkay || !dst)
        return 0;
    return (unsigned)Scan(Mode::Save, (unsigned char*)dst, cap);
}

int MKA_StateLoad(const void* src, unsigned len)
{
    if (!bDrvOkay || !src)
        return 1;
    // ScanArea only reads from the buffer when loading
    const size_t n = Scan(Mode::Load, (unsigned char*)const_cast<void*>(src), len);
    return n == len ? 0 : 1;
}

unsigned MKA_StateAreas(void) { return g_areas; }

// --- where the core keeps a set's battery RAM (MK's settings and audits) and its support files
void MKA_SetPaths(const char* nvram_dir, const char* support_dir)
{
    CopyDir(szAppEEPROMPath, nvram_dir);
    if (support_dir && *support_dir) {
        TCHAR base[MAX_PATH] = {};
        CopyDir(base, support_dir);
        _sntprintf(szAppHiscorePath, MAX_PATH, _T("%shiscores/"), base);
        _sntprintf(szAppSamplesPath, MAX_PATH, _T("%ssamples/"), base);
        _sntprintf(szAppCheatsPath, MAX_PATH, _T("%scheats/"), base);
        _sntprintf(szAppArtworkPath, MAX_PATH, _T("%sartwork/"), base);
        _sntprintf(szAppOstPath, MAX_PATH, _T("%sost/"), base);
        _sntprintf(szAppHDDPath, MAX_PATH, _T("%shdd/"), base);
        _sntprintf(szAppBlendPath, MAX_PATH, _T("%sblend/"), base);
        _sntprintf(szAppIpsPath, MAX_PATH, _T("%sips/"), base);
    }
}

// --- the driver table: what sets this core knows, and what each one's ROMs are
int MKA_DriverCount(void) { return (int)nBurnDrvCount; }

// what: 0 short name ("umk3"), 1 full name, 2 parent's short name ("" for a parent), 3 system/board, 4 year,
// 5 manufacturer
const char* MKA_DriverText(int index, int what)
{
    static const UINT32 kinds[] = {DRV_NAME, DRV_FULLNAME, DRV_PARENT, DRV_SYSTEM, DRV_DATE, DRV_MANUFACTURER};
    if (what < 0 || what >= (int)(sizeof(kinds) / sizeof(kinds[0])))
        return "";
    const char* s = WithDriver(index, [&]() { return (const char*)BurnDrvGetTextA(kinds[what]); }, (const char*)"");
    return s ? s : "";
}

int MKA_DriverIndex(const char* name)
{
    if (!name)
        return -1;
    for (UINT32 i = 0; i < nBurnDrvCount; i++) {
        const char* s = MKA_DriverText((int)i, 0);
        if (s && strcmp(s, name) == 0)
            return (int)i;
    }
    return -1;
}

// ROM rom of driver index: its file name, length, CRC32 and FBNeo type flags; 0 past the last ROM
int MKA_DriverRom(int index, int rom, const char** name, unsigned* length, unsigned* crc, unsigned* type)
{
    return WithDriver(index, [&]() -> int {
        BurnRomInfo ri = {};
        if (BurnDrvGetRomInfo(&ri, (UINT32)rom))
            return 0;
        char* n = nullptr;
        if (BurnDrvGetRomName(&n, (UINT32)rom, 0))
            n = nullptr;
        if (name)
            *name = n ? n : "";
        if (length)
            *length = ri.nLen;
        if (crc)
            *crc = ri.nCrc;
        if (type)
            *type = ri.nType;
        return 1;
    }, 0);
}

}  // extern "C"
