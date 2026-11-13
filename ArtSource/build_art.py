"""Cut the downloaded art into the frames Unreal imports.

Inputs live in ArtSource (Nikki/source, packs/). Output goes to ArtSource/Import,
plus the arena floor, which is written over the texture's import source so a
Reimport in the editor picks it up.
"""
import random
from pathlib import Path
from PIL import Image, ImageDraw

ROOT = Path(__file__).parent
OUT = ROOT / "Import"
PACK = ROOT / "packs" / "0x72" / "0x72_DungeonTilesetII_v1.7" / "frames"
FLAIL_SHEET = ROOT / "packs" / "flail-dcss" / "flail-DCSS.png"
FLOOR_TARGET = ROOT.parent / "Project_Riot" / "Content" / "Art" / "T_ArenaFloor.png"
CURSOR_PACK = ROOT / "packs" / "kenney-cursor-pixel"
CURSOR_TARGET = ROOT.parent / "Project_Riot" / "Content" / "Slate" / "Cursors"

NIKKI_SHEETS = {"Idle": "sprite.png", "Run": "run.png", "Strike": "use_f.png", "Death": "death.png"}
NIKKI_FRAME = (16, 24)

RED_FLAIL_CELL = (128, 64, 160, 96)
FLOOR_PIXELS = 512
WALL_PIXELS = 16
WALL_BACKING = (28, 22, 30, 255)
# Darker, cooler floor so the glowing characters carry the contrast.
FLOOR_TONE = (0.62, 0.58, 0.74)

# Hardware cursors per hero: Kenney's white-with-black-outline tiles, tinted and scaled up.
CURSOR_SCALE = 3
HERO_CURSORS = {"Nikki": (255, 105, 190)}
CURSOR_TILES = {"Pointer": "tile_0048.png", "Sword": "tile_0106.png"}


def split_sheet(sheet_path, frame_size, prefix):
    sheet = Image.open(sheet_path).convert("RGBA")
    width, height = frame_size
    for index in range(sheet.width // width):
        sheet.crop((index * width, 0, (index + 1) * width, height)).save(OUT / f"{prefix}_{index}.png")


OUTLINE = (17, 17, 17, 255)
HEADSET = (88, 127, 127)
HAND = {(157, 99, 65), (78, 49, 32)}
SMEAR = (255, 255, 255)
CLEAR = (0, 0, 0, 0)


def head_top_row(frame):
    """First row of the head outline: a run of four dark pixels starting at x=4."""
    for y in range(frame.height):
        if all(frame.getpixel((x, y))[:3] == OUTLINE[:3] for x in range(4, 8)):
            return y
    return 0


def remove_staff(frame):
    """Erase the staff from a strike frame, keeping her hand and the swing smear.

    Above the head, everything right of x=11 is staff. Below it, the body's right
    edge is the outline pixel just past the headset line; anything further right
    is staff unless it is part of the hand (or the outline around the hand).
    """
    top = head_top_row(frame)
    for y in range(frame.height):
        if y < top:
            for x in range(11, frame.width):
                if frame.getpixel((x, y))[:3] != SMEAR:
                    frame.putpixel((x, y), CLEAR)
            continue

        row = [frame.getpixel((x, y))[:3] for x in range(frame.width)]
        headset = [x for x, colour in enumerate(row) if colour == HEADSET]
        edge = (max(headset) + 1) if headset else 12
        if y == top:
            edge = 12
        for x in range(edge + 1, frame.width):
            colour = row[x]
            near_hand = any(row[n] in HAND for n in (x - 1, x, x + 1) if 0 <= n < frame.width)
            if colour == SMEAR or colour in HAND or near_hand:
                continue
            frame.putpixel((x, y), CLEAR)
        if row[edge] not in (SMEAR,) and frame.getpixel((edge, y))[3]:
            frame.putpixel((edge, y), OUTLINE)
    return frame


def export_nikki():
    for anim, file_name in NIKKI_SHEETS.items():
        split_sheet(ROOT / "Nikki" / "source" / file_name, NIKKI_FRAME, f"Nikki_{anim}")
    for index in range(6):
        path = OUT / f"Nikki_Strike_{index}.png"
        remove_staff(Image.open(path).convert("RGBA")).save(path)


def export_pyro():
    for index in range(4):
        Image.open(PACK / f"imp_run_anim_f{index}.png").save(OUT / f"Pyro_Run_{index}.png")


def export_flail():
    flail = Image.open(FLAIL_SHEET).convert("RGBA").crop(RED_FLAIL_CELL)
    handle = Image.new("RGBA", flail.size)
    ball = Image.new("RGBA", flail.size)
    for y in range(flail.height):
        for x in range(flail.width):
            pixel = flail.getpixel((x, y))
            if pixel[3] == 0:
                continue
            if x >= 19 and y >= 14:
                ball.putpixel((x, y), pixel)
            elif abs(x + y - 31) <= 3 and y >= 8:
                handle.putpixel((x, y), pixel)
    handle.crop(handle.getbbox()).save(OUT / "Flail_Handle.png")
    ball.crop(ball.getbbox()).save(OUT / "Flail_Ball.png")


def export_bead():
    bead = Image.new("RGBA", (6, 6))
    draw = ImageDraw.Draw(bead)
    draw.ellipse((0, 0, 5, 5), fill=(255, 150, 40, 255), outline=(120, 30, 10, 255))
    bead.putpixel((2, 2), (255, 240, 180, 255))
    bead.save(OUT / "Pyro_Bead.png")


def export_floor():
    rng = random.Random(7)
    floors = [Image.open(PACK / f"floor_{n}.png").convert("RGBA") for n in range(1, 9)]
    weights = [12, 2, 2, 2, 1, 1, 1, 1]
    wall_front = Image.open(PACK / "wall_mid.png").convert("RGBA")

    floor = Image.new("RGBA", (FLOOR_PIXELS, FLOOR_PIXELS), WALL_BACKING)
    tiles = FLOOR_PIXELS // WALL_PIXELS
    for row in range(tiles):
        for col in range(tiles):
            edge = row in (0, tiles - 1) or col in (0, tiles - 1)
            if edge:
                tile = wall_front
            else:
                tile = rng.choices(floors, weights)[0]
            floor.alpha_composite(tile, (col * WALL_PIXELS, row * WALL_PIXELS))
    floor = Image.merge("RGBA", [band.point(lambda v, k=k: int(v * k)) for band, k in zip(floor.split()[:3], FLOOR_TONE)] + [floor.split()[3]])
    floor.save(FLOOR_TARGET)
    floor.save(ROOT / "T_ArenaFloor.png")


def export_cursors():
    CURSOR_TARGET.mkdir(parents=True, exist_ok=True)
    for hero, colour in HERO_CURSORS.items():
        for name, tile in CURSOR_TILES.items():
            cursor = Image.open(CURSOR_PACK / tile).convert("RGBA")
            pixels = [(*colour, a) if (r, g, b) == (255, 255, 255) and a else (r, g, b, a) for r, g, b, a in cursor.getdata()]
            cursor.putdata(pixels)
            size = (cursor.width * CURSOR_SCALE, cursor.height * CURSOR_SCALE)
            cursor.resize(size, Image.NEAREST).save(CURSOR_TARGET / f"{hero}_{name}.png")


def main():
    OUT.mkdir(exist_ok=True)
    export_nikki()
    export_pyro()
    export_flail()
    export_bead()
    export_floor()
    export_cursors()


if __name__ == "__main__":
    main()
