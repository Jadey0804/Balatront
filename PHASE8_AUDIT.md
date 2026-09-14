# Phase 8: save and resume

Branch: `phase-8-save-load`. No commit or push in this phase.

## Controls and storage

- F5 saves during play or pause. Holding the key does not repeatedly write. It saves the last complete simulation frame without advancing that frame.
- F9 loads from the menu, during play, pause or game over. A successful load stays paused; Esc or Enter resumes.
- The HUD displays success/failure. A missing, truncated, corrupt or incompatible save does not replace the current world.
- One slot: `savegame.dat`, relative to the game's working directory (Visual Studio uses the executable output directory). Builds do not copy or overwrite save files.
- Saving writes and checks `savegame.tmp` first, then moves the previous slot to `savegame.bak` before replacing it. The backup is the previous save, not a second selectable slot. If interrupted during replacement and the main file is missing, the backup can be copied back to `savegame.dat`.

## Version 1 data

The Windows binary format writes explicit 32-bit fields, never raw object memory, pointers, images or padding. Booleans and enums are encoded as validated unsigned integers. The file contains magic/version, fixed pool capacity markers, state fields, and a trailing FNV-1a checksum. This checksum detects accidental corruption; it is not an anti-cheat signature. A format/layout change requires a version increment.

| State | Saved data |
| --- | --- |
| Session | Saved playing/paused state, elapsed time, camera position and map mode |
| Player | Current/previous position, velocity, health, speed, collision radius, invulnerability, attack interval, AOE target count and shooting cooldown |
| Level | Level number, portal open flag, position and animation time |
| Map | Fixed map dimensions and every tile ID; procedural map seed and next-level seed |
| Enemies | Every slot including dying enemies, type, health, positions, shot cooldown, animation state/time, facing, hit feedback and spawn origin |
| Spawning | Generator state, spawn accumulator and counters |
| Projectiles | Both pools, active flags, positions, velocity, remaining lifetime, damage, radius, motion fraction, allocation cursor and counts |
| Combat | AOE cooldown/feedback/target positions, kills, pickups and upgrade feedback |

Load parses into a temporary snapshot, checks types, finite numeric values, capacities, dimensions, tile IDs, active projectile counts and checksum before applying it. Animation durations are rebuilt from loaded sprite resources. Input key latches and file-operation feedback are not part of the combat snapshot.

Save format version 4 also stores manual bombs, their cooldown, health pickups, bomb-hit feedback, the player's damage-feedback animation state and the number of collected upgrades used by scoring. Changing images or balancing no longer invalidates a save by itself. Earlier development saves are rejected because they do not contain these new fields. Fixed tile data itself is saved, so reading an existing save restores its saved map layout. Current image resources are still required at startup. No shop/cards beyond the existing upgrades are introduced in this phase.

## Verification and manual acceptance

Debug x64 compiled successfully. Only the existing course tutorial C4018 warning remains. No automated test suite or interactive game run was performed. `GamesEngineeringBase.h` is unchanged.

1. In level 1 at about 45 seconds, press F5 with enemies/projectiles visible. Note TIME, position, HP, SHOT, AOE and F1 counts, or take a screenshot.
2. Play for about 10 seconds, move elsewhere and take damage. Press F9. The saved scene and values should return, paused. Resume and confirm enemies, shots and cooldowns continue.
3. Repeat in level 2 away from its spawn, including negative world coordinates. The terrain, seed and camera should return to the saved place.
4. Save before the portal opens, then after it opens. Each load should restore the corresponding portal state and animation time.
5. Close/restart the game and press F9 in the menu. Read the same save; also check F9 after dying.
6. Back up the save, then temporarily rename it, truncate a copy or change a byte. F9 should show an error and preserve the current world. Restore the original afterward.
7. In pause, check F5 and F9; hold the keys to confirm each press performs only one operation.

Build output used for compile verification: `../phase8-artifacts/Debug/Balatront.exe`. Its save is separate from the usual `../x64/Debug/savegame.dat`; use the same executable/working directory when checking persistence across restarts.
