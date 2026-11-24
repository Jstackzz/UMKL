# Toolchain and sources

## Tools (this PC, 2026-11-24)

| tool | where | for |
|---|---|---|
| VS 2026 Build Tools (MSVC 19.50), CMake, Ninja | `C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools`; `tools\vsenv.cmd` puts them on PATH | everything |
| Perl (Git for Windows' own) | `C:\Program Files\Git\usr\bin\perl.exe` | FBNeo's `gamelist.pl`, which writes the driver list |
| Qt 6.12.0 msvc2022_64 | the MK3D project's `external\qt` (the root `CMakeLists.txt` finds it), or this repo's `external\qt` | the app (Quick, QuickControls2, Network) |
| Python 3.13 | on PATH | `tools\py\coretest.py` |

## Sources in `external/` (git-ignored)

| folder | from | notes |
|---|---|---|
| `FBNeo` | `git clone --depth 1 https://github.com/finalburnneo/FBNeo` (commit 61359e1, 2026-11-23) | upstream FBNeo has no libretro port any more; its headless front end (`src/burner/headless`, made for OpenEmu) is what `core/` builds on |
| `SDL3` | copied from the MK3D project's `recomp\mka\out\deps-src\sdl3` (3.4.16): `include`, `src`, `cmake`, `build-scripts`, `CMakeLists.txt`, `LICENSE.txt` | built static |
| `GekkoNet` | copied from the MK3D project's `external\GekkoNet` (commit 0d491a4) | built static with its asio UDP adapter |

## The core build (`core/CMakeLists.txt`)

FBNeo builds with Meson or its makefiles; neither is set up here, so the core lists its sources itself: the burn
library, the 6821 PIA, CVSD board, DAC, HC55516, MSM6295, YM2151 and sample sound, the TMS34010, ADSP-2100, 6809 and
Z80 cores, the Midway group (`dcs2k`, `midtunit`, `midwunit`, `midwayic`, `narc_sound`, `williams_adpcm`,
`yawdim_sound`) and the drivers `d_yunit`, `d_tunit`, `d_wunit`, plus the headless front end and the burner files it
borrows from `burner/sdl`.

What differs from upstream, all in the build (external/FBNeo stays as cloned):

- `burner/sdl` and `burner/headless` carry a POSIX `tchar.h`; MSVC searches the folders of including files, so those
  sources compile from copies in the build folder (`shadow/`), and `core/src/mka_shim.h` (force-included) maps
  `strcasecmp` and gives `dprintf`.
- `intf/cd/cd_interface.cpp` lists no CD backend for the headless build (a zero-size array MSVC rejects): compiled from a
  copy where the `_XBOX` line also covers `BUILD_HEADLESS` (`mka_patched_copy`, which stops the build when the text it
  replaces is gone).
- Left out, with stubs in `core/src/mka_stubs.cpp`: CD images and `cdlist.cpp`, hard disk images (`burn_hdd.cpp`, `ide.cpp`:
  only Killer Instinct), artwork overlays (`burn_artwork.cpp` needs plutosvg).
- `core/src/mka_core.cpp` adds `MKA_State*` (whole-machine states in memory: one `BurnAreaScan` pass), `MKA_SetPaths`
  and the driver and ROM tables. `core/mka_fbneo.def` lists the DLL's exports.

## Facts the code relies on

- All Midway sets: 400x256 at 54.71 Hz (Y-Unit 54.70), sound at 48 kHz. State: about 2.4 MB (T-Unit, Wolf Unit),
  0.7 MB (Y-Unit); saving takes 0.7 ms, loading 0.06 ms; frames run 1,000 to 1,700 per second.
- One loaded copy of `mka_fbneo.dll` runs one game: loading a second set into the same copy crashed, so a new game
  unloads the DLL and loads it again. The library reads ROM tables from a private copy (`cache\mka_tables.dll`).
- FBNeo's buttons for these drivers: fire 1 high punch, 2 block, 3 high kick, 4 low punch, 5 low kick, 6 run.
- A set without its battery RAM stops at "CMOS INVALID ... ANY BUTTON TO CONTINUE"; the app presses BLOCK now and then
  over the first 27 seconds of every boot (online, by frame number on both PCs).
- Online, both games run their power-on by themselves before netplay's frame 0: MK2 desynced when a rollback
  reached into its first frames, whose one-time set-up its saved state does not hold. A match (and Direct Connect)
  runs on through both players joining (`JoinPilot`: BLOCK past the CMOS screen, coins, player 1 starts, player 2 joins
  on its select screen, 2,392 frames, about a second undrawn at some 2,000 frames per second), so netplay starts on
  the select screen. `--net-test` keeps the 300 frames it was checked with.
- Round wins in RAM (FBNeo's "All RAM"): UMK3 0xC462 / 0xC464, MK3 0xC458 / 0xC45A, MK2 0xC36E / 0xC370, each 0, 1, 2
  through a match; MK1 0xA264 / 0xA2C0, in each player's own block, where a player who lost a round without a win of
  their own reads 255 and both go back to 0 about 3 seconds after the match.
- The server's `match_result` status is `agreed` for a counted match and `practice` for an Online Practice one.
- Local play: START plays two coins, then start (UMK3 wants 2 credits to start); the boot's BLOCK presses stop at a
  player's first START (after frame 300), else on the select screen they picked the fighter under the cursor (MK1's
  is Kano) and confirmed UMK3's mode.
- Hidden fighters (`UnlockHidden`, GameSession.cpp): MK3's EJB menu SMOKE writes CMOS slot 10h, UMK3's ENABLE
  HIDDENS slots 1Fh-21h (Mileena, Ermac, Classic Sub-Zero), all 0EDB0h. A slot is 4 CMOS words of a byte each, high
  byte first, at word 800h + 4 * slot (byte address 0x280000 + 2 * word, after a write to 0x290000 opens it); words
  8B0h-8B1h are the one's complement of the sum of the bytes of words 800h-8AFh. The game reads them into RAM at
  power-up (UMK3: words C43Ch, C43Eh, C440h), so the app writes CMOS before the first frame, or after a first boot
  restored the factory settings (frame 300) and then resets the machine; online, at that frame on both PCs.
- The app's data: `%APPDATA%\MK Arcade Online` (settings.ini with the remembered sign-in, nvram, art: the cards'
  pictures, saved from the attract modes; online.log; sessions\<game>-<time>\game.log, the newest 20 matches).
  `MKA_DATA_DIR` puts it elsewhere (two copies on one PC).
