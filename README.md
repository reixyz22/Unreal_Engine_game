# Project Riot (working title)

![Gameplay](Media/gameplay.gif)

A top-down wave-survival arena game in Unreal Engine 5.8, built with C++ and Paper2D. Nikki, a pink pop star with a ball and chain, holds off growing waves of Pyros; rounds survived is the score.

- **Play it (Windows download):** <ITCH_URL>
- **Why it's built the way it is:** [DECISIONS.md](DECISIONS.md), my running log of design decisions

## What's in the code

- `AArenaPlayerPawn`: Enhanced Input WASD movement, diagonals clamped so they aren't faster, frame-rate independent speed, and a camera that stays centered on Nikki. Right-click a Pyro to throw the ball and chain; it can't be thrown again until it returns to her hand.
- `AArenaChainBall`: flies to the target and back, piercing every Pyro on its path and hitting each one at most once per throw.
- `AArenaEnemy`: Pyros hold a casting range, spread apart from each other into an arc, and float slow, dodgeable beads (`AArenaProjectile`).
- `UHealthComponent`: shared by Nikki and Pyros; health plus i-frames so converging hits can't stack in one moment.
- `AArenaGameMode`: spawns each wave at the arena edges, grows it every round, and restarts when Nikki falls.
- C++ holds the behavior. Blueprint subclasses only plug in assets and tuning (animations, sprites, damage, speeds), so no asset paths are hardcoded in code.

## How it's built

This is my first Unreal project; my earlier engine work is in Godot. It's built AI-assisted: Claude lays out options for each feature, I make the design calls, and I audit the code. The details are in [DECISIONS.md](DECISIONS.md) entry 003.

## Credits

- Music: "chiptune police loop" by obscure music (CC0), from [OpenGameArt](https://opengameart.org/content/chiptune-police-loop)
- Art: Nikki from KingBell's Pixel Art Sprite Mixer (CC BY 4.0); Pyros, floor and walls from 0x72's DungeonTileset II (CC0); the flail from Dungeon Crawl Stone Soup (CC0). Full list in [ArtSource/CREDITS.md](ArtSource/CREDITS.md).
