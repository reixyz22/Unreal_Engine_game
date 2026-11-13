# Decisions

The calls I made on this project, newest at the bottom. Each entry gives what I picked, what I gave up, and why.

## Contents

1. [001 · Running Unreal C++ on an 8GB machine: cutting memory use around the editor](#001-running-unreal-c-on-an-8gb-machine)
2. [002 · Paper2D sprites at a fixed 4 world units per art pixel](#002-paper2d-sprites-at-a-fixed-pixel-scale)
3. [003 · How AI fits in: I design gameplay logic, AI builds UI glue](#003-how-ai-fits-in)
4. [004 · Installing the engine through Epic's preview launcher after the old one broke](#004-installing-the-engine-through-epics-preview-launcher)
5. [005 · A top-down game on Paper2D's side-view X/Z axes](#005-a-top-down-game-on-paper2ds-side-view-axes)
6. [006 · Right-click: attack an enemy, or walk to the ground; out-of-range attacks walk into range first](#006-right-click-attacks-or-walks)
7. [007 · UI drawn from C++ on the HUD canvas instead of UMG widgets](#007-ui-drawn-from-c-on-the-hud-canvas)
8. [008 · Glow and fog through one sprite material and one post-process material](#008-glow-and-fog-through-two-materials)

---

## 001: Running Unreal C++ on an 8GB machine

**2026-11-05**

**Context.** I had about 30 hours, a Ryzen 7 3700X, an RTX 3050 and 8GB of RAM. Epic recommends 32GB. The editor, the C++ compiler and the shader compiler all compete for that memory.

**Decision.**
- Blank template with no starter content. Lumen, ray tracing and virtual shadow maps are off, and editor scalability is set to Low.
- UnrealBuildTool is capped at 4 parallel actions, with a fixed 16GB pagefile as a safety net.
- No IDE runs. Header changes get a full build with the editor closed. Body-only changes use Live Coding.

**Cost.** Header changes cost an editor restart, so I batch them.

---

## 002: Paper2D sprites at a fixed pixel scale

**2026-11-05**

**Options.** I compared Paper2D sprites (picked), flat 3D shapes, and Paper2D tilemaps. Tilemaps are built for levels, and I have one arena.

**Decision.**
- Every sprite imports at the same density, and the code scales them to 4 world units per art pixel (`ArenaArt.h`). That way the characters and the arena never mismatch.
- Textures use nearest filtering and no mipmaps, and anti-aliasing and motion blur are off.

**Cost.** Epic has not invested in Paper2D in years, so its rough edges are mine to work around.

---

## 003: How AI fits in

**2026-11-05, revised 2026-11-13**

- **Gameplay logic** (movement, combat, enemy AI, waves): I design it and write pseudocode. Claude turns it into C++, and I review every line.
- **UI, art pipeline and engine hookups:** Claude builds these and I steer the result. I'm primarily a backend developer, and this is client glue rather than game logic.

---

## 004: Installing the engine through Epic's preview launcher

**2026-11-05**

**Context.** In the regular Epic launcher, the **+** for adding an engine version was grayed out. None of these fixed it: reinstalling, signing out, clearing caches, running as admin, or the fixes in Epic's forum thread. A brand new account had the same problem, so the cause was the launcher, not my account.

**Options.**
- Building from source needed over 100GB of disk and hours of compiling on 8GB of RAM.
- Waiting on support would take days.
- The preview launcher (picked) worked on the first try.

**Cost.** The launcher is preview software. The project lives in git, so a launcher problem can't cost me work.

---

## 005: A top-down game on Paper2D's side-view axes

**2026-11-06**

**Options.**
- Lay sprites flat. This fights Paper2D on every sprite.
- Keep sprites upright and play in X/Z (picked).
- Drop Paper2D for 3D quads.

**Decision.** X is screen-right and Z is screen-up. The camera looks along Y. There is no gravity, so "up = Z" costs nothing.

**Cost.** Readers must know Z means up the screen, not height.

---

## 006: Right-click attacks or walks

**2026-11-13**

- Right-clicking an enemy throws the auto attack.
- If the enemy is out of range, Nikki walks into range and the auto fires on arrival. The walk point is fixed at click time, even if the enemy moves. That keeps the code simple, and the input buffer feels generous.
- Right-clicking the ground walks there. WASD cancels both.
- Hovering an enemy outlines it in red and turns the cursor into a sword, so it's always clear when a click means attack.

---

## 007: UI drawn from C++ on the HUD canvas

**2026-11-13**

**Options.**
- UMG widget Blueprints (the usual choice): polished, but every element is an editor asset.
- An `AHUD` subclass drawing directly to the canvas (picked).

**Why.** Health bars, the wave banner and the mana bar are all rectangles, text and sprite icons. The canvas does that in a few draw calls with no assets. Each bar reads its `UHealthComponent`, so new enemy types get bars for free.

**Cost.** No visual layout editor. If the UI grows into menus, UMG is the move.

---

## 008: Glow and fog through two materials

**2026-11-13**

**Decision.**
- One shared sprite material adds glow and the hover outline to every character and projectile.
- One post-process material on the camera adds drifting fog anchored to the world.
- Bloom and a shared color grade pull the different art packs into one palette.

**Cost.** This reverses 002's "bloom off". The look is now graded and not exactly as drawn. Both materials are generated by `ArtSource/setup_assets.py`, so the look is reproducible.
