# Balatront

Balatront is a two-level 2D twin-stick shooter built with the course `GamesEngineeringBase` framework. The first level uses a fixed tile map; its portal appears after 120 seconds and leads to a procedurally generated infinite map. Surviving another 120 seconds wins the game.

## Build and run

1. Open `Balatront.sln` in Visual Studio 2022 with Desktop development with C++ and a Windows SDK installed.
2. Select `x64` and either `Debug` or `Release`, then build the solution.
3. Start the game from Visual Studio. The build copies the complete `Resources` tree beside the executable.

The executable reads configuration and assets from its adjacent `Resources` directory. Edit the repository copies under `Resources`, rebuild, and restart the game to apply changes.

## Controls

- `WASD` or the left analogue stick: move
- Hold the left mouse button or move the right analogue stick: fire the manual bomb in one of eight directions
- `Space` or controller `A`: area-of-effect attack (press once per attack)
- `Esc`: pause or resume
- `F1`: toggle gameplay and collision diagnostics
- `F5`: save at any time
- `F9`: load the save and resume in a paused state
- `Enter`: start or restart

The player also fires automatically at the closest enemy. Attack-speed upgrades affect both automatic fire and the manual eight-direction attack. AOE upgrades increase the number of targets.

## World and progression

Water blocks the player but does not block enemies. Roads increase player movement speed by 1.5 times. Lava remains passable and deals continuous damage. Health pickups appear near the player and disappear if they are not collected in time.

The fixed level is loaded from `Resources/tiles.txt`. The infinite level is generated deterministically from its current seed, so revisiting an area in the same run produces the same terrain without writing additional map files.

The final score is:

`enemy kills x 10 + collected upgrades x 100 + remaining health x 5`

A victory receives an `A`, `S`, `SS`, or `SSS` rank. Dying before the end displays the defeat screen without a rank.

## Configuration

- `Resources/gameplay.txt` contains player, enemy, projectile, AOE, upgrade, level, pickup, and lava values.
- `Resources/Sprites/sprites.txt` contains image paths, frame dimensions, animation counts, playback rates, display sizes, and orientation settings.
- `Resources/tiles.txt` contains the fixed 32 by 32 pixel tile map.

The local save slot is `savegame.dat` beside the executable. Saves store the level, timers, procedural seed, player state, enemies, projectiles, pickups, upgrades, and portal state.

`GamesEngineeringBase.h` is course framework code. The input extension adds four stick-value getters and fixes right-stick zero-length normalization and its deadzone constant. Game code uses this framework for controller input without calling XInput directly. The first connected controller controls both sticks and the AOE button.
