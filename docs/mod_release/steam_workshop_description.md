# [5.0.1] Barony Extended Multiplayer (1-15 Online/LAN)

This mod expands Barony multiplayer from the vanilla 1-4 range to support up to 15 players in Online and LAN lobbies.

It includes lobby, join, save/load, and gameplay-tuning support for larger groups while keeping normal 1-4 play intact.

## What This Mod Does

- Expands multiplayer lobby capacity to 15 players.
- Adds host `# Players` control in lobby `Game Settings`.
- Enables existing lobby paging controls so all player slots can be viewed/managed.
- Improves join handling for larger lobbies and save-based joins.
- Extends save/load support for expanded player slots.
- Applies overflow-only gameplay tuning for 5+ players while preserving baseline 1-4 behavior.
- Keeps local splitscreen capped at 4 players.

## How Scaling Works (5+ Players)

- Scaling is overflow-only: vanilla 1-4 player balance is preserved.
- For 5-15 players, map generation/resource tuning is adjusted for larger groups.
- The mod scales map pressure and economy inputs (such as room/spawn behavior and loot/gold/food distribution) to keep large-lobby runs playable.

## Installation (Safe Method)

Important: every player in the lobby must install this mod (same build/version), not just the host. Players without the mod are likely to crash, fail joins, or desync.

1. Close Barony and Steam.
2. Find your Barony install folder.
3. Copy the full Barony folder to a new folder (example: `Barony-15p`).
4. Extract this mod package.
5. Copy the mod files into the copied Barony folder.
6. Replace files when prompted (root executable and runtime library files for your platform).
7. Launch the game from the copied/modded folder.

This keeps your original Barony install untouched.

## How To Use (Host)

1. Launch the modded executable.
2. Select `Play Game`.
3. Select `Online` or `LAN`.
4. Host a lobby.
5. On the host player card, open `Game Settings`.
6. Set `# Players` to the lobby size you want (up to 15).
7. Use lobby page arrows to view extra player slots.
8. Start once everyone is ready.

## Troubleshooting

- Only seeing 1-4 players:
  - You likely launched the wrong executable, or files were not replaced in the copied folder.
- Cannot find all players in a big lobby:
  - Look for page arrows on the far left and far right sides of the lobby screen (easy to miss at first).
- Join rejected / lobby full / failed join:
  - Make sure every player is on the same Barony version and same mod build.
  - Recreate the lobby after changing `# Players`.
- High-player instability:
  - 5+ player sessions are supported, but edge-case desync/instability can still occur.
  - If this happens, restart lobby, reduce player count, and avoid rapid join/leave churn.
- Save load/join issues:
  - Host should load the save first, then players join.
  - All players must use matching game + mod versions.
- Steam update restored files:
  - Re-copy mod files into your copied mod folder (do not overwrite your original install).
- Please report issues:
  - Even known rough edges need reports with repro steps. We cannot fix bugs we are not aware of.

## How To Report Bugs

Post bug reports here:
[Steam Workshop bug/feedback page](https://steamcommunity.com/sharedfiles/filedetails/?id=3436129755&searchtext=)

Please include:

- Game version and mod version/date
- Platform/store (Steam/Epic/other)
- Network mode (Online/LAN)
- Player count at time of issue
- Host or client role
- Exact reproduction steps
- Any logs/screenshots if available
