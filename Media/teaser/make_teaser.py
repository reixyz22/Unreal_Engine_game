"""Cuts the story teaser gif (and an mp4) from OBS recordings.

    python Media/teaser/make_teaser.py

Edit SHOTS below: each clip names a recording and its start and end time. A clip whose
recording is missing becomes a grey "SHOT NEEDED" card, so the teaser still builds.
Needs ffmpeg and Pillow.

Rule for this file: no plot past the first stage. The captions tease the setup only.
"""

import shutil
import subprocess
import sys
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent.parent
VIDEOS = Path.home() / "Videos"
WORK = HERE / "_work"          # scratch frames and segments; gitignored
OUT_GIF = ROOT / "Media" / "teaser.gif"
OUT_MP4 = ROOT / "Media" / "teaser.mp4"

W, H, FPS = 640, 360, 12
YELLOW = (255, 214, 64)
WHITE = (240, 240, 240)
FONT = "C:/Windows/Fonts/consolab.ttf"

# OBS recordings of the editor (1920 x 1080). A: first take, used for Nikki's cutscene
# with its dialog, the shop purchase and the map to Space. B: second take, everything else.
TAKE_A = VIDEOS / "2026-11-17 14-59-17.mp4"
TAKE_B = VIDEOS / "2026-11-17 15-09-02.mp4"
# Public domain: NASA, "Artemis I Launch (Press Site)", images.nasa.gov, 2022.
LAUNCH = VIDEOS / "nasa_launch" / "artemis1_launch_small.mp4"

# Crops of the editor window: the whole game viewport, or tighter on the hut room
# (room, phone and dialog box) so the cutscene reads at gif size.
# Every crop is 16:9, so each shot fills the whole gif with no bars.
VIEWPORT = "1526:858:4:141"
# Room, phone and dialog box together, for the moments the UI is the point.
ROOM = "1100:620:210:380"
# Tighter, for Carl's small "[E] Text your crush" prompt.
ROOM_CLOSE = "720:405:480:400"
# Zoomed until the room touches the gif's edges; found per frame, since the hut camera moves.
ROOM_FILL = "room"
# Carl and the top wall, so the bouncing arrow is big enough to follow.
BOUNCE = "1100:620:300:135"


def at(stamp):
    """'1:32.5' -> 92.5 seconds."""
    minutes, seconds = stamp.split(":")
    return int(minutes) * 60 + float(seconds)


def clip(source, start, end, note, crop=VIEWPORT, speed=1.0):
    return {"kind": "clip", "source": Path(source), "start": at(start), "seconds": at(end) - at(start),
            "note": note, "crop": crop, "speed": speed}


def card(text, seconds=1.1):
    return {"kind": "card", "text": text, "seconds": seconds}


def montage(*clips):
    return {"kind": "montage", "clips": clips}


SHOTS = [
    clip(TAKE_A, "0:01.0", "0:03.0", "Nikki asleep, phone buzzes", ROOM),
    clip(TAKE_A, "0:04.0", "0:08.0", "Carl's text types out", ROOM, speed=2.0),
    clip(TAKE_A, "0:09.0", "0:11.5", "'Ugh. Not this guy again.' BLOCKED", ROOM),
    clip(TAKE_A, "0:12.0", "0:14.8", "BEEP BEEP BEEP, 'I guess I have to get ready.'", ROOM),
    clip(TAKE_A, "0:16.0", "0:17.6", "Mace out of bed, WHAM", ROOM_FILL),
    clip(TAKE_B, "0:16.6", "0:17.8", "Closet, POOF", ROOM_FILL),
    card("TWO ADVENTURERS"),
    card("GO ON AN ANNOYING JOURNEY"),
    card("TO PAY THE BILLS"),
    clip(TAKE_B, "0:56.6", "0:58.0", "Carl: [E] Text your crush", ROOM_CLOSE),
    clip(TAKE_B, "1:06.5", "1:08.5", "The dog wakes Carl", ROOM_FILL),
    clip(TAKE_A, "1:12.0", "1:14.3", "Map: Nikki walks to Space"),
    clip(LAUNCH, "0:56.0", "0:56.5", "A real rocket launch", crop=None),
    clip(TAKE_B, "0:49.0", "0:51.0", "Nikki fighting in Space"),
    clip(TAKE_B, "0:29.5", "0:31.0", "The shop"),
    clip(TAKE_A, "0:54.0", "0:55.5", "Buying Lucky Charms"),
    clip(TAKE_B, "1:42.5", "1:45.0", "Carl hovers ???"),
    montage(
        clip(TAKE_B, "1:58.6", "1:59.4", "Two arrow kills in a row"),
        clip(TAKE_B, "2:04.4", "2:05.2", "Shots into a group by the wall"),
        clip(TAKE_B, "2:39.9", "2:41.5", "Arrow off the top wall into the pack", BOUNCE),
        clip(TAKE_B, "2:42.0", "2:42.8", "Crowded wave in Space"),
    ),
    card("NEW GAME COMING SOON", 1.4),
]


def font(size):
    try:
        return ImageFont.truetype(FONT, size)
    except OSError:
        return ImageFont.load_default()


def run(*args):
    subprocess.run(["ffmpeg", "-v", "error", "-y", *map(str, args)], check=True)


def encode_frames(frames, out):
    """Writes a list of PIL frames as one segment."""
    folder = out.with_suffix("")
    folder.mkdir(parents=True, exist_ok=True)
    for i, frame in enumerate(frames):
        frame.save(folder / f"{i:04d}.png")
    run("-framerate", FPS, "-i", folder / "%04d.png", "-c:v", "libx264", "-pix_fmt", "yuv420p", "-r", FPS, out)


def centered(draw, text, size, color, y=None):
    f = font(size)
    box = draw.textbbox((0, 0), text, font=f)
    x = (W - (box[2] - box[0])) // 2
    y = (H - (box[3] - box[1])) // 2 if y is None else y
    draw.text((x + 3, y + 3), text, font=f, fill=(0, 0, 0))
    draw.text((x, y), text, font=f, fill=color)


def render_card(shot, out):
    # Black, then the caption types on, holds, and snaps out.
    frames, total = [], round(shot["seconds"] * FPS)
    text = shot["text"]
    for i in range(total):
        frame = Image.new("RGB", (W, H), (0, 0, 0))
        shown = text[: min(len(text), i * 3)]
        centered(ImageDraw.Draw(frame), shown, 34 if len(text) < 24 else 28, YELLOW)
        frames.append(frame)
    encode_frames(frames, out)


def render_missing(shot, out):
    frames = []
    for _ in range(round(shot["seconds"] * FPS)):
        frame = Image.new("RGB", (W, H), (60, 60, 66))
        draw = ImageDraw.Draw(frame)
        centered(draw, "SHOT NEEDED", 30, WHITE, 120)
        centered(draw, shot["note"][:58], 16, (200, 200, 200), 180)
        frames.append(frame)
    encode_frames(frames, out)


def room_box(frame):
    """The hut room's walls in a 1920 x 1080 editor frame: (left, top, right, bottom).

    Counts bright pixels per column and per row. Only the room fills a whole column or row;
    pop-up text, the hero, and the dialog box's border lines are too short to pass.
    """
    gray = frame.convert("L").point(lambda v: 1 if v > 60 else 0)
    width, height = gray.size
    pixels = gray.load()
    columns = [sum(pixels[x, y] for y in range(200, 1000)) for x in range(width)]
    rows = [sum(pixels[x, y] for x in range(width)) for y in range(200, 800)]
    filled = [x for x, count in enumerate(columns) if count > 200]
    left, right = filled[0], filled[-1]
    top = 200 + next(y for y, count in enumerate(rows) if count > 300)
    return left, top, right, top + (right - left) * 4 // 5


def render_room_fill(shot, out):
    """Zooms each frame until the room touches the gif's edges.

    Done per frame because the hut camera snaps to follow the hero mid-shot.
    """
    folder = WORK / f"{out.stem}_src"
    folder.mkdir(parents=True)
    run("-ss", shot["start"], "-t", shot["seconds"], "-i", shot["source"], "-vf", f"fps={FPS}", folder / "%04d.png")
    frames = []
    for path in sorted(folder.glob("*.png")):
        frame = Image.open(path).convert("RGB")
        left, top, right, bottom = room_box(frame)
        crop_height = (right - left) * 9 // 16
        # The room is taller than 16:9: keep the top (bed, nightstand, closet), lose the door.
        y = top + max(0, (bottom - top - crop_height) // 4)
        frames.append(frame.crop((left, y, right, y + crop_height)).resize((W, H), Image.LANCZOS))
    encode_frames(frames, out)


def render_clip(shot, out):
    if not shot["source"].exists():
        render_missing(shot, out)
        return
    if shot["crop"] == ROOM_FILL:
        render_room_fill(shot, out)
        return
    crop = f"crop={shot['crop']}," if shot["crop"] else ""
    speed = f"setpts=PTS/{shot['speed']}," if shot["speed"] != 1.0 else ""
    filters = f"{speed}{crop}scale={W}:{H},fps={FPS}"
    run("-ss", shot["start"], "-t", shot["seconds"], "-i", shot["source"], "-vf", filters,
        "-an", "-c:v", "libx264", "-pix_fmt", "yuv420p", out)


def render_flash(out):
    encode_frames([Image.new("RGB", (W, H), (255, 255, 255))], out)


def main():
    if not shutil.which("ffmpeg"):
        sys.exit("ffmpeg not found on PATH")
    if WORK.exists():
        shutil.rmtree(WORK)
    WORK.mkdir(parents=True)

    segments = []

    def next_out():
        out = WORK / f"seg{len(segments):03d}.mp4"
        segments.append(out)
        return out

    for shot in SHOTS:
        if shot["kind"] == "card":
            render_card(shot, next_out())
        elif shot["kind"] == "clip":
            render_clip(shot, next_out())
        elif shot["kind"] == "montage":
            # Flash cuts: a white frame between each quick clip.
            for piece in shot["clips"]:
                render_flash(next_out())
                render_clip(piece, next_out())

    listing = WORK / "segments.txt"
    listing.write_text("".join(f"file '{s.as_posix()}'\n" for s in segments))
    run("-f", "concat", "-safe", "0", "-i", listing, "-c:v", "libx264", "-pix_fmt", "yuv420p", "-r", FPS, OUT_MP4)
    palette = WORK / "palette.png"
    run("-i", OUT_MP4, "-vf", "palettegen=max_colors=96", palette)
    run("-i", OUT_MP4, "-i", palette, "-lavfi", "paletteuse=dither=bayer:bayer_scale=4", OUT_GIF)
    print(f"wrote {OUT_GIF} and {OUT_MP4}")


if __name__ == "__main__":
    main()
