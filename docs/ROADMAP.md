# Roadmap

## Done (2026-11-24)

1. Emulation core from FBNeo's Midway drivers, from the player's ROM zips; deterministic (draw, save, load, replay).
   The WaveNet prototypes (`umk3w`, `umk3wp`) are left out.
2. Rollback netplay with GekkoNet, checked on all four games at 0 frames delay.
3. The app: Qt Quick menus in the Legacy Kollection spirit, the games in the same window, controllers and keyboard,
   sound, pause menu, Direct Connect.
4. **Online on the MK3D server.** The server (`mk3d-rollback/server`) has the four arcade games as a second product
   ("mkarcade"): its own games, counts, lobbies and records, the same accounts. The MK3D launcher's client is in
   `app/src/online`; `GameProcess` answers MatchController in the same process. Ranked (sets to 3, Elo, leaderboard),
   Kasual (rematches), Lobbies (codes, passwords, chat), Online Practice (a practice room by code) and the
   leaderboard, each on its own screen. Results come from each game's round counters in RAM (`kRoundRam`).
   Checked end to end on a local copy of the server with two copies of the app playing by themselves.
5. The server change is live on the Oracle server (2026-11-24, with `mk3d-rollback/tools/deploy-server.ps1`). A
   restart drops who is connected for a few seconds (launchers sign back in by themselves) and loses open lobbies:
   `tools/py/server_activity.py` shows who is on first. The change itself is in `mk3d-rollback` (`server/app.py`,
   `server/protocol.py`, `server/tests/test_ranks.py`), not committed there yet.

## Next

1. **A match between two PCs** over the internet (the punch, the relay for strict NATs and hidden addresses): checked
   only on one PC so far.
2. **Training**: no timer and health refills (RAM writes on both PCs alike), back to the select screen, record and
   play back the dummy.
3. **Jukebox and sound levels**: the games' music comes from their sound ROMs, not files; watch the sound commands to
   know what plays, silence it in what the player hears and play their own songs. Master volume works now; music and
   effects apart need the sound split.
4. **Spectating** lobby matches (the server passes the players' feeds; the game would play them back) and King of the
   Hill lobbies: the protocol has both, this app offers neither yet (its hello says so).
5. **Controls page** (remapping), the player package (no ROMs, Qt runtime beside the app), an updater.
