MK Arcade Online @VERSION@
==========================

The arcade Mortal Kombat games (Mortal Kombat, Mortal Kombat II, Mortal Kombat 3, Ultimate Mortal Kombat 3) in one
app, played with a controller or the keyboard: Arcade, Versus, and online with rollback netcode (Ranked, Kasual,
Lobbies, Online Practice, the leaderboard, Direct Connect).


GETTING STARTED
---------------

1. Unzip this folder somewhere of its own (the Desktop, Documents, a games folder). Do not run it from inside the zip.
2. Put your own ROM sets in the "roms" folder:  mk.zip  mk2.zip  mk3.zip  umk3.zip
   (MAME or FBNeo sets). The games are not included. Already have them somewhere else? Point the app at that folder
   in SETTINGS, LIBRARY.
3. Double-click MKArcade.exe.

Nothing else needs installing: everything the app needs is in this folder.

The first time, Windows may say "Windows protected your PC". The app is not signed with a paid certificate, which is
all that message means: click "More info", then "Run anyway". (On Windows 11 with Smart App Control turned on, Windows
blocks apps that are not signed and gives no "Run anyway".)

The first time you play online, Windows Firewall asks about MKArcade: click "Allow", or online matches may not connect.


CONTROLS
--------

                      Controller                 Keyboard
  Move                d-pad or left stick        arrows or W A S D
  High punch          X / Square                 U
  Low punch           A / Cross                  J
  High kick           Y / Triangle               O
  Low kick            B / Circle                 L
  Block               LB or RB                   I or Space
  Run (MK3, UMK3)     LT or RT                   K
  Coin and start      START                      Enter
  Pause               BACK / VIEW                Esc

In the menus: d-pad or arrows to move, A or Enter to choose, B or Esc to go back, LB / RB (Q / E) to change game or
tab, Y (Tab) for settings. F11 switches between full screen and a window.

Local play: ARCADE for one player, VERSUS for two controllers. START puts in a coin and joins.


ONLINE
------

Pick a game, then ONLINE. Ranked, Kasual, Lobbies, Online Practice and the leaderboard need an account on the MK3D
Online server: sign in with your MK3D account, or make one there (CREATE ACCOUNT). Direct Connect needs no account:
one player hosts, the other types the host's address (the host forwards the game port, 7101 UDP, on their router).

  Ranked           first to 3 match wins takes the set; your Elo gives your rank.
  Kasual           anyone, with rematches.
  Lobbies          a room with a 5-letter code (and a password if you want one), with chat.
  Online Practice  a private room for you and a friend, joined by its code: fight as long as you like.

Each player picks their fighter on the game's select screen with their own controller.

Settings, ONLINE has your region, the highest ping to search with, input delay, and "Hide my IP address from
opponents" (matches then go through the server).


WHERE THINGS ARE
----------------

Your settings, your sign-in and the games' saved settings: %APPDATA%\MK Arcade Online
To start over, close the app and delete that folder.


LICENSES
--------

See THIRD-PARTY.txt and the licenses folder. The emulation core (mka_fbneo.dll) is FinalBurn Neo's, under its
non-commercial license: this app is free, and may not be sold.
