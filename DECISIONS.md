# Decisions

Running log of the calls I made on this project. Newest at the bottom. Each entry says what I picked, what I gave up, and why.

---

## 001: Unreal C++ on an 8GB machine

**Date:** 2026-11-05
**Status:** Accepted

### Context

I'm building this in about 30 hours for an application deadline. My PC is a Ryzen 7 3700X, an RTX 3050, and 8GB of RAM. Epic recommends 32GB. Upgrading tonight isn't an option.

On 8GB, the editor, the C++ compiler and the shader compiler all fight over memory. That means swapping, slow builds, or the editor dying mid-compile. I still want Unreal and C++, so the job is to cut everything around them that eats memory without cutting the game.

### Decision

- **Project:** Blank template, no starter content, no ray tracing. I left the quality preset on Maximum at creation, but the rendering overrides below are what actually matter.
- **Rendering:** Lumen off (Dynamic GI and Reflections set to None), regular shadow maps instead of virtual shadow maps, editor scalability on Low.
- **Compiling:** UnrealBuildTool capped at 4 parallel actions in `BuildConfiguration.xml`, so one build can't eat all my RAM. A fixed 16GB pagefile on my SSD as a safety net.
- **No IDE running.** Visual Studio is installed only for its compiler and never opened. I compile from the editor:
  - `.cpp` body change: Live Coding (Ctrl+Alt+F11).
  - Header change (new member, new UPROPERTY, new class): close the editor and reopen the `.uproject` so it rebuilds. Live Coding on header changes causes fake bugs, so I don't do it.
- **Nothing else open while compiling.** That includes the browser.
- **Package early.** I'll package a throwaway build while the game is tiny, so the first slow cook and any packaging bugs show up while I still have time to fix them.

### Consequences

- The game doesn't use Unreal's flagship rendering. For a 2D arena game that's not a real loss.
- Full rebuilds are slower than on a normal dev machine, and every header change costs an editor restart. I batch header changes to keep that rare.
- Real numbers from my machine (filled in as I go):
  - First shader compile: __ min
  - First C++ build: __ min
  - Live Coding change: __ s
  - First packaged build: __ min

---

## 002: 2D with Paper2D

**Date:** 2026-11-05
**Status:** Accepted

### Context

I want a top-down arena game in the style of Brotato: one player, one square arena, enemies coming at you. I want it to look good fast, with art I can make myself in the time I have.

### Options I looked at

- **Paper2D sprites** (picked). True 2D pixel art, unlit, cheap to render.
- **2.5D with flat 3D shapes.** No art cost and the safest engine path, but it's not the look I want.
- **Paper2D tilemaps.** Built for levels and rooms. I have one arena and no levels, so it's overhead.

### Decision

- Paper2D sprites and flipbooks with an orthographic camera.
- Art is drawn at native pixel size and every sprite uses **Pixels Per Unit = 0.25**, so 1 art pixel is always 4 world units. The player (16px) and the arena (512px) stay at the same pixel density, so nothing looks mismatched.
- Textures use Paper2D texture settings (nearest filtering, no mipmaps) and the project has anti-aliasing, auto exposure, bloom and motion blur off, so the pixel art stays crisp and the colors stay what I drew.

### Consequences

- Epic hasn't put much work into Paper2D in years. Rough edges are mine to work around and write up.
- Sprites are unlit, so I don't need lights at all. That also helps with decision 001.

---

## 003: How I use AI on this project

**Date:** 2026-11-05
**Status:** Accepted

I own every line in this repo. The workflow:

1. I name the next feature.
2. Claude gives me at least 3 ways to build it, each with its tradeoff.
3. I pick one and write the pseudocode.
4. Claude corrects it or pitches an algorithm tweak. I decide if the tweak is worth it over the simple version.
5. Claude turns my pseudocode into C++, and I read every line before it goes in.

When I let Claude write something tedious without my pseudocode, the commit message says so.

---

## 004: Installing the engine through Epic's preview launcher

**Date:** 2026-11-05
**Status:** Accepted

### Context

Before I wrote a line of code, the engine wouldn't install.

- I had to clear drive space first just to fit the engine.
- In the regular Epic Games Launcher, the Library page loaded without the Engine Versions row. Once that row did show up, the **+** to add an engine version was grayed out, and the Install Engine button did nothing.
- What I tried, none of which worked:
  - checked for a dialog hiding behind the window
  - signed out and back in
  - fully exited from the system tray and relaunched
  - switched panels and came back
  - restarted the PC
  - ran the launcher as administrator
  - cleared the launcher's web cache
- Then I worked through the fixes from Epic's forum thread on this exact bug: "sign in later," filling in the account's first and last name, hiding and unhiding the library, and a clean reinstall. None of them worked either.
- I made a brand new Epic account with every name field filled in, to rule out my account being the problem. Same result. That told me it wasn't my account. It was the launcher itself on my machine, and the forum thread had people stuck on it for a month.

### Options I looked at

- **Build Unreal from the GitHub source.** Official, but it needs over 100GB of disk I had just cleared out, and building the whole engine on 8GB of RAM would take hours. Not possible inside the deadline.
- **Wait on Epic support.** Official, but replies take days.
- **Epic's new launcher (Preview)** (picked). It's a separate app, and installing it replaces the old launcher. One forum post said it deleted someone's projects, but I had zero projects, so that risk didn't apply to me.

### Decision

I installed the preview launcher and installed UE 5.8 from it. It worked on the first try.

### Consequences

- I'm on preview software for the launcher. The project lives in git, so a launcher problem can't cost me work.
- If anyone cloning this repo hits the same grayed out **+**, the preview launcher is the way around it.
- Time lost to this: __ hours.

---

## 005: Top-down game on Paper2D's side-view axes

**Date:** 2026-11-06
**Status:** Accepted

### Context

Paper2D is built for side-scrollers. It draws sprites standing up in the X/Z plane. My game is top-down.

### Options I looked at

- **Lay sprites flat and look down the Z axis.** That fights Paper2D on every sprite and every spawn.
- **Keep sprites upright and lay out the game in X/Z** (picked).
- **Drop Paper2D for 3D quads.** That reopens decision 002.

### Decision

X is screen-right, Z is screen-up, and the camera looks along Y. Nothing in the game uses gravity, so it doesn't matter that "up" is the Z axis.

### Consequences

- There's zero rotation math anywhere.
- Anyone reading the code needs to know Z means "up the screen," not height. The player class says that once in a comment.
