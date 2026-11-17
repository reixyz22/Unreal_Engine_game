"""Placeholder art for the story scenes: Nikki's hut, the village, the phone, the world map.

Everything here is drawn by script or recolored from the CC0 packs, and written to
ArtSource/Import/Story. To replace a piece with real art, save a PNG with the same
name over it and re-run setup_assets.py; nothing in the code changes.
"""
import math
import random
from pathlib import Path
from PIL import Image, ImageDraw

from build_art import OUT as IMPORT, OUTLINE, PACK, outline
from stage_table import NIKKI_ROUTE

STORY = IMPORT / "Story"
FLOOR = 768           # floor textures are 768 px square, 4 world units per pixel
CLEAR = (0, 0, 0, 0)

# Hut interior, in floor pixels. Scenes in C++ use the same numbers times 4.
HUT_HALF = (72, 56)
WALL = 8


def new(size, fill=CLEAR):
    image = Image.new("RGBA", size, fill)
    return image, ImageDraw.Draw(image)


def save(image, name):
    STORY.mkdir(parents=True, exist_ok=True)
    image.save(STORY / f"{name}.png")


def recolor(image, mapping):
    image = image.convert("RGBA")
    image.putdata([(*mapping.get(p[:3], p[:3]), p[3]) for p in image.getdata()])
    return image


# ---------------------------------------------------------------- Nikki in pyjamas

PJ = {(242, 238, 241): (150, 196, 255), (121, 119, 120): (92, 126, 214)}
BOLT = (255, 226, 80)
CAP = (150, 196, 255)
CAP_DARK = (92, 126, 214)
OUTLINE = (17, 17, 17)


def add_nightcap(frame):
    """A floppy nightcap with a pom, so the pyjamas read from across the room."""
    pixels = frame.load()
    # Her hair spans columns 4-11; the mace swing can rise above it, so only look there.
    top = next(y for y in range(frame.height) if any(pixels[x, y][3] for x in range(4, 12)))
    if top < 4:
        return
    rows = ((top - 1, 4, 11), (top - 2, 6, 11), (top - 3, 8, 12))
    for y, left, right in rows:
        for x in range(left, right + 1):
            edge = x in (left, right)
            pixels[x, y] = (*(OUTLINE if edge else CAP), 255)
    pixels[7, top - 1] = (*CAP_DARK, 255)
    pixels[9, top - 2] = (*BOLT, 255)
    for x, y in ((13, top - 3), (13, top - 4), (14, top - 4)):
        pixels[x, y] = (255, 255, 255, 255)


def export_pyjamas():
    """Her normal frames with the outfit swapped for blue pyjamas with lightning bolts."""
    for anim, count in (("Idle", 6), ("Run", 4), ("Strike", 6)):
        for index in range(count):
            frame = recolor(Image.open(IMPORT / f"Nikki_{anim}_{index}.png"), PJ)
            pixels = frame.load()
            for y in range(frame.height):
                for x in range(frame.width):
                    # A sparse diagonal pattern of yellow on the pyjama cloth reads as bolts.
                    if pixels[x, y][:3] == (150, 196, 255) and (x + 2 * y) % 5 == 0:
                        pixels[x, y] = (*BOLT, 255)
            add_nightcap(frame)
            save(frame, f"Nikki_PJ_{anim}_{index}")


# ---------------------------------------------------------------- hut

def plank_floor(draw, box, base=(132, 92, 58)):
    left, top, right, bottom = box
    draw.rectangle(box, fill=(*base, 255))
    rng = random.Random(5)
    for y in range(top, bottom, 8):
        draw.line((left, y, right, y), fill=(98, 66, 42, 255))
        offset = rng.randrange(0, 24)
        for x in range(left + offset, right, 24):
            draw.line((x, y, x, min(y + 8, bottom)), fill=(98, 66, 42, 255))


def export_hut_floor():
    floor, draw = new((FLOOR, FLOOR), (10, 8, 12, 255))
    cx = cy = FLOOR // 2
    hx, hy = HUT_HALF
    draw.rectangle((cx - hx - WALL, cy - hy - WALL, cx + hx + WALL, cy + hy + WALL), fill=(74, 50, 40, 255), outline=OUTLINE)
    plank_floor(draw, (cx - hx, cy - hy, cx + hx, cy + hy))
    draw.rectangle((cx - hx, cy - hy, cx + hx, cy - hy + 3), fill=(60, 40, 30, 255))  # wall shadow
    # A window with moonlight on the back wall.
    draw.rectangle((cx - 8, cy - hy - 7, cx + 8, cy - hy - 1), fill=(120, 160, 220, 255), outline=OUTLINE)
    draw.line((cx, cy - hy - 7, cx, cy - hy - 1), fill=OUTLINE)
    # Rug.
    draw.ellipse((cx - 22, cy - 2, cx + 22, cy + 26), fill=(170, 60, 110, 255), outline=(90, 30, 60, 255))
    draw.ellipse((cx - 14, cy + 4, cx + 14, cy + 20), outline=(230, 150, 190, 255))
    floor.save(IMPORT / "T_Floor_Hut.png")


def bed(sleeper=False, broken=False):
    image, draw = new((24, 36))
    draw.rectangle((1, 1, 22, 34), fill=(120, 80, 50, 255), outline=OUTLINE)       # frame
    draw.rectangle((3, 3, 20, 11), fill=(240, 236, 228, 255), outline=(170, 160, 150, 255))  # pillow
    draw.rectangle((2, 13, 21, 33), fill=(214, 88, 150, 255))                        # blanket
    for y in range(16, 33, 5):
        draw.line((2, y, 21, y), fill=(170, 60, 116, 255))
    if sleeper:
        # Her pyjama frame (nightcap and all), so the swing frames match.
        head = Image.open(STORY / "Nikki_PJ_Idle_0.png").convert("RGBA").crop((0, 4, 16, 18))
        image.alpha_composite(head, (4, 0))
        draw.rectangle((2, 13, 21, 15), fill=(214, 88, 150, 255))
    if broken:
        image, draw = new((30, 38))
        left = bed().crop((0, 0, 12, 36)).rotate(9, expand=True, resample=Image.NEAREST)
        right = bed().crop((12, 0, 24, 36)).rotate(-9, expand=True, resample=Image.NEAREST)
        image.alpha_composite(left, (0, 0))
        image.alpha_composite(right, (14, 0))
        for x, y in ((13, 6), (15, 12), (12, 18), (16, 24), (13, 30)):
            draw.point((x, y), fill=OUTLINE)
    return image


def bed_swing(index):
    """Nikki sitting up under the covers mid-swing: her pyjama strike frame, cut at the waist."""
    image = bed()
    torso = Image.open(STORY / f"Nikki_PJ_Strike_{index}.png").convert("RGBA").crop((0, 4, 16, 20))
    image.alpha_composite(torso, (4, 0))
    ImageDraw.Draw(image).rectangle((2, 16, 21, 18), fill=(214, 88, 150, 255))
    return image


def export_hut_props():
    save(bed(), "Bed")
    save(bed(sleeper=True), "Bed_Nikki")
    for index in range(6):
        save(bed_swing(index), f"Bed_Nikki_Swing_{index}")
    save(bed(broken=True), "Bed_Broken")

    stand, draw = new((14, 14))
    draw.rectangle((0, 2, 13, 13), fill=(110, 72, 46, 255), outline=OUTLINE)
    draw.rectangle((5, 4, 9, 10), fill=(30, 30, 40, 255), outline=OUTLINE)          # the phone
    draw.rectangle((6, 5, 8, 8), fill=(120, 220, 255, 255))
    save(stand, "Nightstand")

    closet, draw = new((24, 30))
    draw.rectangle((0, 0, 23, 29), fill=(96, 62, 44, 255), outline=OUTLINE)
    draw.line((12, 1, 12, 28), fill=OUTLINE)
    draw.rectangle((2, 2, 10, 27), outline=(130, 90, 62, 255))
    draw.rectangle((14, 2, 21, 27), outline=(130, 90, 62, 255))
    draw.point((10, 15), fill=(240, 200, 90, 255))
    draw.point((14, 15), fill=(240, 200, 90, 255))
    save(closet, "Closet")

    door, draw = new((18, 12))
    draw.rectangle((0, 0, 17, 11), fill=(150, 104, 60, 255), outline=OUTLINE)
    draw.point((14, 6), fill=(240, 200, 90, 255))
    save(door, "Door")

    arrow, draw = new((9, 9))
    draw.polygon([(0, 2), (8, 2), (4, 8)], fill=(255, 220, 70, 255), outline=OUTLINE)
    save(arrow, "Arrow")

    alert, draw = new((9, 11))
    draw.rectangle((0, 0, 8, 8), fill=(255, 255, 255, 255), outline=OUTLINE)
    draw.line((4, 2, 4, 4), fill=(220, 40, 60, 255))
    draw.point((4, 6), fill=(220, 40, 60, 255))
    draw.point((4, 10), fill=(255, 255, 255, 255))
    save(alert, "Alert")

    poof, draw = new((20, 20))
    for cx, cy, r in ((7, 8, 6), (13, 9, 6), (10, 13, 6), (10, 6, 5)):
        draw.ellipse((cx - r, cy - r, cx + r, cy + r), fill=(245, 245, 250, 255))
    save(outline(poof, (170, 170, 190, 255)), "Poof")


# ---------------------------------------------------------------- Carl

CARL_COLOURS = {(238, 142, 46): (44, 40, 58), (250, 203, 62): (72, 66, 96),
                (75, 167, 71): (62, 96, 210), (61, 115, 79): (40, 62, 150)}


def carl_frame(anim, index):
    """The pack's elf with dark hair and a blue tunic."""
    return recolor(Image.open(PACK / f"elf_m_{anim}_anim_f{index}.png"), CARL_COLOURS)


def export_carl():
    for anim, name in (("idle", "Idle"), ("run", "Run")):
        for index in range(4):
            save(carl_frame(anim, index), f"Carl_{name}_{index}")
    Image.open(PACK / "weapon_arrow.png").save(STORY / "Arrow_Shot.png")
    Image.open(PACK / "weapon_bow.png").save(STORY / "Bow.png")

    # His bed with him in it, for the "back to bed" beat.
    asleep = bed()
    asleep.alpha_composite(carl_frame("idle", 0).crop((0, 9, 16, 19)), (4, 2))
    ImageDraw.Draw(asleep).rectangle((2, 13, 21, 15), fill=(214, 88, 150, 255))
    save(asleep, "Bed_Carl")

    dog, draw = new((16, 12))
    draw.rectangle((2, 4, 11, 9), fill=(196, 150, 96, 255), outline=OUTLINE)       # body
    draw.rectangle((9, 1, 14, 6), fill=(196, 150, 96, 255), outline=OUTLINE)       # head
    draw.rectangle((9, 0, 10, 2), fill=(120, 84, 52, 255))                         # ear
    draw.point((13, 3), fill=OUTLINE)                                              # eye
    draw.point((14, 5), fill=(230, 90, 110, 255))                                  # tongue
    draw.line((1, 3, 2, 5), fill=(196, 150, 96, 255))                              # tail
    for x in (3, 6, 9):
        draw.line((x, 10, x, 11), fill=OUTLINE)
    save(dog, "Dog")


# ---------------------------------------------------------------- phone

def export_phone():
    phone, draw = new((84, 150))
    draw.rounded_rectangle((0, 0, 83, 149), radius=8, fill=(24, 22, 30, 255), outline=OUTLINE)
    draw.rounded_rectangle((5, 12, 78, 134), radius=3, fill=(232, 236, 244, 255))
    draw.rectangle((5, 12, 78, 24), fill=(60, 110, 220, 255))   # app header bar
    draw.ellipse((37, 138, 46, 147), outline=(90, 90, 100, 255))
    draw.rectangle((34, 5, 49, 6), fill=(70, 70, 80, 255))
    save(phone, "Phone_UI")

    # Carl: the pack's elf with dark hair and a blue tunic, proudly holding up a Pyro like a fish.
    carl = carl_frame("idle", 0)
    save(carl, "NPC_Carl")
    pfp, draw = new((32, 32))
    draw.ellipse((0, 0, 31, 31), fill=(150, 210, 240, 255), outline=OUTLINE)
    pfp.alpha_composite(carl.crop((0, 8, 16, 28)), (3, 10))
    pyro = Image.open(IMPORT / "Pyro_Run_0.png").convert("RGBA").transpose(Image.FLIP_TOP_BOTTOM)
    pfp.alpha_composite(pyro, (15, 6))
    draw.line((14, 20, 20, 8), fill=(252, 203, 163, 255))       # his arm, holding it by the tail
    mask, mask_draw = new((32, 32))
    mask_draw.ellipse((0, 0, 31, 31), fill=(255, 255, 255, 255))
    pfp.putalpha(Image.composite(pfp.split()[3], Image.new("L", (32, 32), 0), mask.split()[3]))
    save(pfp, "Carl_Pfp")


# ---------------------------------------------------------------- town

def world_to_px(x, z):
    return FLOOR // 2 + x // 4, FLOOR // 2 - z // 4


def export_town_floor():
    rng = random.Random(21)
    floor, draw = new((FLOOR, FLOOR), (86, 140, 70, 255))
    for _ in range(2600):                                        # grass texture
        x, y = rng.randrange(FLOOR), rng.randrange(FLOOR)
        draw.point((x, y), fill=rng.choice([(74, 126, 62, 255), (104, 158, 80, 255), (96, 150, 74, 255)]))
    dirt, dirt_edge = (178, 142, 96, 255), (140, 108, 72, 255)

    def path(a, b, width=18):
        (ax, ay), (bx, by) = world_to_px(*a), world_to_px(*b)
        draw.line((ax, ay, bx, by), fill=dirt_edge, width=width + 2)
        draw.line((ax, ay, bx, by), fill=dirt, width=width)

    path((0, 1040), (0, -1500), 22)                              # main road, hut to gate
    for z in (620, -80):
        path((-420, z - 100), (420, z - 100), 14)                # side lanes to the houses
    path((0, -700), (300, -700), 14)                             # to the elder
    path((0, -760), (-300, -760), 14)                            # to the stall
    for _ in range(500):
        x, y = rng.randrange(FLOOR), rng.randrange(FLOOR)
        if floor.getpixel((x, y)) == dirt:
            draw.point((x, y), fill=(162, 128, 86, 255))

    # Fence around the village with a gate gap at the bottom of the road.
    inset = 10
    for x in range(inset, FLOOR - inset, 6):
        for y in (inset, FLOOR - inset):
            if y == FLOOR - inset and abs(x - FLOOR // 2) < 20:
                continue
            draw.rectangle((x, y - 3, x + 1, y + 2), fill=(120, 84, 52, 255))
    for y in range(inset, FLOOR - inset, 6):
        for x in (inset, FLOOR - inset):
            draw.rectangle((x - 1, y, x + 1, y + 2), fill=(120, 84, 52, 255))
    draw.line((inset, inset, FLOOR - inset, inset), fill=(150, 108, 68, 255))
    draw.line((inset, inset, inset, FLOOR - inset), fill=(150, 108, 68, 255))
    draw.line((FLOOR - inset, inset, FLOOR - inset, FLOOR - inset), fill=(150, 108, 68, 255))
    draw.line((inset, FLOOR - inset, FLOOR // 2 - 20, FLOOR - inset), fill=(150, 108, 68, 255))
    draw.line((FLOOR // 2 + 20, FLOOR - inset, FLOOR - inset, FLOOR - inset), fill=(150, 108, 68, 255))
    floor.save(IMPORT / "T_Floor_Town.png")


def house(roof, wall=(214, 196, 160), size=(48, 44), tape=False, notice=False):
    width, height = size
    image, draw = new(size)
    roof_height = height // 2
    draw.rectangle((3, roof_height, width - 4, height - 1), fill=(*wall, 255), outline=OUTLINE)
    draw.polygon([(0, roof_height), (width - 1, roof_height), (width - 8, 0), (7, 0)], fill=(*roof, 255), outline=OUTLINE)
    dark = tuple(int(c * 0.75) for c in roof)
    for y in range(4, roof_height, 4):
        draw.line((4, y, width - 5, y), fill=(*dark, 255))
    door_left = width // 2 - 5
    draw.rectangle((door_left, height - 14, door_left + 9, height - 1), fill=(120, 80, 50, 255), outline=OUTLINE)
    for window_x in (7, width - 15):
        draw.rectangle((window_x, roof_height + 4, window_x + 7, roof_height + 10), fill=(150, 200, 230, 255), outline=OUTLINE)
    if tape:
        draw.line((door_left - 3, height - 15, door_left + 12, height - 1), fill=(250, 210, 40, 255), width=2)
        draw.line((door_left + 12, height - 15, door_left - 3, height - 1), fill=(250, 210, 40, 255), width=2)
    if notice:
        draw.rectangle((door_left + 2, height - 12, door_left + 7, height - 6), fill=(250, 250, 245, 255), outline=(150, 150, 150, 255))
        draw.line((door_left + 3, height - 10, door_left + 6, height - 10), fill=(60, 60, 70, 255))
    return image


def export_town_props():
    save(house((176, 70, 60), size=(40, 38)), "House_Nikki")
    save(house((120, 110, 100), tape=True), "House_Evicted")
    save(house((84, 120, 170), notice=True), "House_Notice")
    save(house((70, 130, 90), wall=(240, 232, 214), size=(60, 52)), "House_Nice")
    save(house((150, 120, 80)), "House_Plain")

    sign, draw = new((12, 14))
    draw.rectangle((5, 6, 6, 13), fill=(110, 76, 48, 255))
    draw.rectangle((0, 0, 11, 7), fill=(206, 170, 110, 255), outline=OUTLINE)
    draw.line((2, 3, 9, 3), fill=(90, 60, 40, 255))
    draw.line((2, 5, 7, 5), fill=(90, 60, 40, 255))
    save(sign, "Sign_Post")

    yard, draw = new((14, 12))
    draw.rectangle((2, 7, 2, 11), fill=(200, 200, 200, 255))
    draw.rectangle((11, 7, 11, 11), fill=(200, 200, 200, 255))
    draw.rectangle((0, 0, 13, 7), fill=(40, 70, 170, 255), outline=OUTLINE)
    draw.line((2, 2, 11, 2), fill=(255, 255, 255, 255))
    draw.line((2, 4, 8, 4), fill=(230, 60, 60, 255))
    save(yard, "Yard_Sign")

    tree, draw = new((22, 28))
    draw.rectangle((9, 18, 12, 27), fill=(110, 76, 48, 255), outline=OUTLINE)
    for cx, cy, r in ((11, 9, 9), (6, 13, 6), (16, 13, 6)):
        draw.ellipse((cx - r, cy - r, cx + r, cy + r), fill=(52, 110, 60, 255), outline=OUTLINE)
    draw.ellipse((6, 4, 13, 11), fill=(78, 140, 74, 255))
    save(tree, "Tree")

    well, draw = new((18, 18))
    draw.ellipse((1, 4, 16, 17), fill=(130, 130, 140, 255), outline=OUTLINE)
    draw.ellipse((4, 7, 13, 14), fill=(40, 70, 130, 255), outline=OUTLINE)
    draw.rectangle((2, 0, 15, 3), fill=(150, 90, 60, 255), outline=OUTLINE)
    save(well, "Well")

    for name, signed in (("Contract", False), ("Contract_Signed", True)):
        table, draw = new((24, 16))
        draw.rectangle((0, 4, 23, 15), fill=(120, 82, 52, 255), outline=OUTLINE)
        draw.rectangle((6, 1, 17, 11), fill=(246, 238, 214, 255), outline=(150, 130, 100, 255))
        for y in (3, 5, 7):
            draw.line((8, y, 15, y), fill=(120, 110, 100, 255))
        if signed:
            draw.line((9, 9, 12, 8), fill=(220, 40, 80, 255))
            draw.line((12, 8, 15, 10), fill=(220, 40, 80, 255))
        save(table, name)

    stall, draw = new((36, 28))
    draw.rectangle((2, 12, 33, 27), fill=(130, 90, 56, 255), outline=OUTLINE)
    for x in range(0, 36, 6):
        draw.polygon([(x, 0), (x + 6, 0), (x + 6, 10), (x + 3, 13), (x, 10)],
                     fill=((220, 70, 70, 255) if (x // 6) % 2 == 0 else (245, 240, 230, 255)), outline=OUTLINE)
    save(stall, "Stall")

    gate, draw = new((44, 22))
    for x in (0, 38):
        draw.rectangle((x, 0, x + 5, 21), fill=(110, 76, 48, 255), outline=OUTLINE)
    draw.rectangle((0, 0, 43, 5), fill=(130, 90, 56, 255), outline=OUTLINE)
    save(gate, "Gate")

    # Villagers come from the 0x72 pack.
    Image.open(PACK / "wizzard_f_idle_anim_f0.png").save(STORY / "NPC_Sarah.png")
    Image.open(PACK / "dwarf_m_idle_anim_f0.png").save(STORY / "NPC_Merchant.png")


# ---------------------------------------------------------------- world map

TERRAIN = {
    None: ((96, 150, 80), (70, 120, 62)),                    # village green
    "BP_Theme_Dungeon": ((120, 112, 116), (84, 78, 84)),
    "BP_Theme_Desert": ((226, 196, 120), (196, 160, 90)),
    "BP_Theme_Space": ((40, 36, 70), (20, 18, 40)),
    "BP_Theme_FarDesert": ((214, 150, 96), (180, 112, 70)),
    "BP_Theme_ElfWorld": ((90, 150, 220), (60, 110, 190)),
}


def export_world_map():
    rng = random.Random(8)
    width, height = 256, 352
    world, draw = new((width, height), (222, 200, 150, 255))
    for _ in range(1800):                                        # parchment grain
        x, y = rng.randrange(width), rng.randrange(height)
        draw.point((x, y), fill=rng.choice([(210, 186, 134, 255), (232, 212, 164, 255)]))
    for _, theme, (u, v), _ in NIKKI_ROUTE:
        light, dark = TERRAIN[theme]
        cx, cy = u * width, v * height
        for _ in range(9):                                       # a lumpy region per stage
            ox, oy, r = rng.uniform(-22, 22), rng.uniform(-12, 12), rng.uniform(14, 26)
            draw.ellipse((cx + ox - r, cy + oy - r * 0.6, cx + ox + r, cy + oy + r * 0.6), fill=(*dark, 255))
        for _ in range(9):
            ox, oy, r = rng.uniform(-18, 18), rng.uniform(-9, 9), rng.uniform(10, 20)
            draw.ellipse((cx + ox - r, cy + oy - r * 0.6, cx + ox + r, cy + oy + r * 0.6), fill=(*light, 255))
        if theme == "BP_Theme_Space":
            for _ in range(24):
                draw.point((cx + rng.uniform(-30, 30), cy + rng.uniform(-14, 14)), fill=(255, 255, 255, 255))
    draw.rectangle((0, 0, width - 1, height - 1), outline=(96, 66, 40, 255), width=3)
    draw.rectangle((5, 5, width - 6, height - 6), outline=(150, 112, 70, 255))
    save(world, "World_Map")

    fog, draw = new((48, 48))
    for cx, cy, r in ((16, 20, 14), (30, 18, 14), (24, 30, 15), (12, 32, 10), (36, 32, 11)):
        draw.ellipse((cx - r, cy - r, cx + r, cy + r), fill=(58, 52, 74, 255))
    for cx, cy, r in ((18, 18, 8), (30, 24, 8)):
        draw.ellipse((cx - r, cy - r, cx + r, cy + r), fill=(78, 70, 98, 255))
    save(fog, "Map_Fog")

    for name, colour in (("Map_Node", (250, 206, 80)), ("Map_Node_Locked", (130, 124, 130))):
        node, draw = new((11, 11))
        draw.ellipse((0, 0, 10, 10), fill=(*colour, 255), outline=OUTLINE)
        draw.ellipse((3, 2, 5, 4), fill=(255, 255, 255, 255))
        save(node, name)


def main():
    export_pyjamas()
    export_hut_floor()
    export_hut_props()
    export_carl()
    export_phone()
    export_town_floor()
    export_town_props()
    export_world_map()


if __name__ == "__main__":
    main()
