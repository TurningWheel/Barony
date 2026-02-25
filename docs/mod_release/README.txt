BARONY EXTENDED MULTIPLAYER
README.txt

IMPORTANT
- Install this mod into a COPY of Barony.
- Do not overwrite your only/original game install.
- Use the package that matches your platform/store.

WINDOWS (STEAM, EPIC, OR NO-DRM)
1. Exit Barony and your launcher (Steam/Epic).
2. Open your Barony install folder.
3. Steam path helper: Steam Library -> Barony -> Manage -> Browse local files.
4. Copy the full Barony folder to a new folder (example: C:\Games\Barony-15p).
5. Extract the matching mod package for your platform/store.
6. Copy mod files into the copied Barony folder.
7. Replace files when prompted (.exe and .dll files in the game root).
8. Launch the modded executable from the copied folder.

MACOS
1. Exit Barony and Steam.
2. In Steam: Library -> Barony -> Manage -> Browse local files.
3. In Finder, go up one folder if needed so you can see Barony.app.
4. Duplicate Barony.app (or copy/paste it) and rename the copy (example: Barony-15p.app).
5. Extract the matching macOS mod package zip.
6. Right-click your copied app -> Show Package Contents.
7. Right-click the mod's Barony.app from the zip (older packages: barony.app) -> Show Package Contents.
8. In the mod app, open Contents/MacOS. Copy:
   - Barony (older packages may use lowercase barony)
   - libsteam_api.dylib
9. Paste those into your copied app's Contents/MacOS folder and replace when prompted.
10. In the mod app, open Contents/Frameworks and copy all .dylib files.
11. Paste them into your copied app's Contents/Frameworks folder and replace when prompted.
12. Launch the copied app (right-click -> Open the first time).
13. If macOS shows "cannot verify" for a dylib:
    - Click Done.
    - Open System Settings -> Privacy & Security.
    - Click Open Anyway for the blocked app/library, then launch again.

STEAM DECK
1. Exit Barony.
2. Press the Steam button -> Power -> Switch to Desktop.
3. Open Steam in Desktop Mode, then open Barony install folder:
   Steam Library -> Barony -> Manage -> Browse local files.
4. Copy the full Barony folder to a new folder.
5. Extract the matching Steam Deck/Linux mod package.
6. Copy mod files into the copied folder.
7. Replace executable/shared library files when prompted.
8. Launch `run-barony.sh` from the copied folder (Desktop Mode terminal or Gaming Mode shortcut to that script).

LINUX (DESKTOP)
1. Exit Barony and Steam.
2. Open your Barony install folder (steamapps/common/Barony).
3. Copy the full Barony folder to a new folder.
4. Extract the matching Linux/Proton mod package.
5. Copy mod files into the copied folder.
6. Replace executable/shared library files provided by the mod package.
7. Launch `run-barony.sh` in the copied folder with your normal Proton setup.

IN-GAME HOST SETUP (ALL PLATFORMS)
1. Start the modded game.
2. Click Play Game.
3. Select Online or LAN.
4. Host a lobby.
5. On the host player card, open Game Settings.
6. Set # Players to the desired size (up to 15).
7. Start after all players are ready.

QUICK TROUBLESHOOTING
- If you only see 1-4 players, you launched the wrong executable or files were not replaced.
- If you cannot see all players in big lobbies, use the paging arrows on the far left/right sides of the lobby screen (easy to miss at first).
- All players must use the same Barony version and the same mod build.
- 5+ player sessions can still hit rare desync/instability; recreate lobby and retry.

BUG REPORTS
https://steamcommunity.com/sharedfiles/filedetails/?id=3436129755&searchtext=

When reporting, include platform, player count, LAN/Online, host/client, and exact repro steps.
