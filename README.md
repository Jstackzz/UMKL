# MK Arcade Online

The arcade Mortal Kombat games (Mortal Kombat, Mortal Kombat II, Mortal Kombat 3, Ultimate Mortal Kombat 3) in one
controller-driven app that feels like a game's own launcher: a console-style menu, the games playing in the same
window, and rollback netplay. The player brings their own ROM sets; nothing from the games is in this repository.

The menus follow the idea of *Mortal Kombat: Legacy Kollection*, simpler: a title screen, a carousel of the four games
whose selected card plays that game's own attract mode (and blurs it across the whole screen), a row of modes, online
hub tiles, tabbed settings, an on-screen keyboard, a pause menu, all steered with a controller (or the keyboard).

## Status (2026-11-24)

| piece | state |
|---|---|
| Emulation core (`core/`, FBNeo's Midway drivers) | done: every MK1 to UMK3 set FBNeo has (Y-Unit, T-Unit, Wolf Unit), from the player's zips |
| Determinism | checked: drawn and undrawn frames, saving and loading every frame, replays from a saved state all give identical states (`tools/py/coretest.py`) |
| Rollback netplay (GekkoNet) | done and checked on all four games: two copies at 0 frames input delay re-ran 1,000 to 1,300 frames each (rollbacks 12 deep) and agreed at every confirmed checkpoint |
| App shell | done: Qt Quick menus, game thread with sound (SDL3) and frame pacing, controllers (SDL3) and keyboard |
| Menus | title, home carousel, online hub, sign-in, matchmaking, lobbies, leaderboard, direct connect, settings, in-game pause; Training and Jukebox are marked SOON |
| Local play | Arcade (1P) and Versus (2P); START puts in a coin and joins |
| Online without a server | Direct Connect (host or join by address) |
| Ranked, Kasual, Lobbies, Online Practice, Leaderboard | done, on the MK3D server (the same accounts): checked end to end on a local copy of the server, two copies of the app playing by themselves (UMK3 and MK1 Kasual, an MK2 ranked set of 3, an MK3 lobby, a UMK3 practice room). The server change that adds the arcade games (`mk3d-rollback/server`) is live on the Oracle server since 2026-11-24 |
| WaveNet | left out: the two WaveNet prototypes (`umk3w`, `umk3wp`) are not in the core |

## Build

```
tools\build.cmd
```

builds the core and the app into `build\Release` and puts the Qt runtime beside `build\Release\app\MKArcade.exe`.
Then `powershell -File tools\package.ps1` makes the players' zip, `dist\MK-Arcade-Online-<version>-windows.zip`: the
app, the core, the Qt and Visual C++ runtimes (nothing to install), an empty `roms` folder, a readme
(`tools\package\README.txt`) and the licenses. Players unzip it, add their ROM sets and double-click `MKArcade.exe`.
Needs the VS 2026 Build Tools, Git (for its Perl) and Qt 6.12 msvc2022_64 (the MK3D project's copy is found by
itself). Toolchain and sources: [docs/SETUP.md](docs/SETUP.md).

## Play

Start `MKArcade.exe`. The ROM folder defaults to the `UMKL` folder on the Desktop (or `roms` beside the app); change it
in Settings, LIBRARY. Needed: `mk.zip`, `mk2.zip`, `mk3.zip`, `umk3.zip` (MAME or FBNeo sets; the core finds ROMs by
CRC, whatever their names). F11 switches full screen and window.

| | controller | keyboard |
|---|---|---|
| move | d-pad or left stick | arrows or WASD |
| high punch, low punch | X / □, A / ✕ | U, J |
| high kick, low kick | Y / △, B / ○ | O, L |
| block | LB or RB | I or Space |
| run (MK3, UMK3) | LT or RT | K |
| coin and start | START | Enter |
| pause | BACK / VIEW | Esc |

In the menus: d-pad to move, A to choose, B to go back, LB / RB to switch games or tabs, Y for settings.

## Online

ONLINE on a game's menu opens its hub. Everything but Direct Connect is on the MK3D server (`129.80.13.245:7777`, its
certificate pinned by fingerprint in `Settings.h`), with the MK3D launcher's accounts: sign in there, or make an
account. The server keeps the two products apart (the hello's `product`): arcade players see only the arcade games,
counts, lobbies and records, and MK3D players only theirs.

- **Ranked**: first to 3 match wins takes the set; Elo moves once per set; the leaderboard ranks by it.
- **Kasual**: anyone, with rematches as long as both want them.
- **Lobbies**: a room with a 5-letter code (and a password if wanted), chat; both players ready starts the match.
- **Online Practice**: a private room for two, joined by its code; endless fights, no records. Leaving the match
  un-readies the player, so no new one starts until they are ready again.
- **Leaderboard**: each game's ranked board.

The online client is the MK3D launcher's (`app/src/online`: ServerClient, MatchController, UdpRendezvous for the UDP
punch, TunnelBridge and the relay for players behind strict NATs). MatchController drives a game process through the
protocol's `start` / `park` and status lines; here `GameProcess` answers them from `GameSession` in the same process.
A match: both games boot with no battery RAM and join both players by a fixed script (about 2,400 frames, a second
undrawn, the same on both PCs), then rollback netplay starts on the select screen, where each player picks with their
own controller. The result comes from the games' own round counters in RAM (`kRoundRam` in `GameSession.cpp`, found
with `tools/py/ramscan.py`): 2 wins for a side, standing for 30 frames, is the match.

## Tests

```
python tools/py/coretest.py --roms <rom folder> --out <folder> umk3 mk2 mk3 mk mkla4
```

boots each set, saves pictures of its attract mode, prints its state size and speed and replays frames from a saved
state (they must come out identical).

```
MKArcade.exe --net-test set=umk3 side=1 port=7101 peer=127.0.0.1:7102 delay=0 frames=2400 log=a.txt
MKArcade.exe --net-test set=umk3 side=2 port=7102 peer=127.0.0.1:7101 delay=0 frames=2400 log=b.txt
```

play scripted inputs against each other with rollback; `a.txt` and `b.txt` must list the same state hashes.
`MKArcade.exe --shot out.png --nav start,down,accept` presses menu buttons and saves the window as a picture (with
`QSG_RENDER_LOOP=basic` when the window may be behind others, or its transitions never finish).

A whole online match on this PC, against a local copy of the server (fresh database and certificate):

```
python -I tools/py/local_server.py <mk3d-rollback> <work folder> 17777 17778     (prints READY 17777 <fingerprint>)
set MKA_DATA_DIR=<folder A> & MKArcade.exe --online-test name=testerA pass=testpass123 game=umk3 ranked=0 autoplay=attack server=127.0.0.1:17777 fp=<fingerprint> port=17101
set MKA_DATA_DIR=<folder B> & MKArcade.exe --online-test name=testerB pass=testpass123 game=umk3 ranked=0 autoplay=idle server=127.0.0.1:17777 fp=<fingerprint> port=17201
```

Each copy signs in (making its account the first time), searches, accepts and plays: `attack` walks in and punches,
`idle` stands, so the attacker wins 2-0; both print `TEST OK` once the server agreed. `ranked=1 matches=3` plays a
ranked set; `lobby=host` and `lobby=join` a lobby match; `mode=sparring codefile=<file> leave_after=25` a practice
room (its code passed through the file) that the host leaves. `tools/py/server_activity.py` shows who is on a live
server (before a deploy restarts it).

## Layout

```
core/          mka_fbneo.dll: FBNeo's emulation core with only the Midway TMS34010 drivers, its headless front end,
               and mka_core.cpp (whole-machine states in memory for rollback, the ROM tables)   [FB Neo license]
app/src/       MKArcade.exe: EmuCore (loads the DLL), GameSession (game thread, sound, pacing, GekkoNet netplay, the
               online match's join, result and status lines), GameView (the picture in QML), InputHub (controllers,
               keyboard), Library (ROM checks), Settings
app/src/online the MK3D launcher's online client (copied, marked as such), stand-ins for that launcher's own files
               (AppPaths, WinUtil, GameData, Style, GameProcess) and OnlineHub, what the menus see
app/qml/       the menus
tools/         build.cmd, vsenv.cmd; py/coretest.py, ramscan.py (finds round counters), local_server.py,
               server_activity.py
external/      FBNeo, SDL3, GekkoNet (git-ignored, docs/SETUP.md)
```

## Licenses

The app's own code (`app/`, `tools/`) is MIT. `core/` builds FinalBurn Neo, whose license is non-commercial (no
selling, no asking for donations, changes published): `mka_fbneo.dll` is a separate file the app loads, and
`core/src/*.cpp` are under that license too. GekkoNet is BSD-2-Clause, SDL3 zlib, Qt 6 LGPL-3.0.
