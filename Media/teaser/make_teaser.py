"""Cuts the story teaser gif (and an mp4) from OBS recordings.

    python Media/teaser/make_teaser.py

Edit SHOTS below: each clip names a recording, a start time and a length. A clip whose
recording is missing becomes a grey "SHOT NEEDED" card, so the teaser always builds and
doubles as an animatic. Needs ffmpeg and Pillow.

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
OUT_MP4 = HERE / "_work" / "teaser.mp4"

W, H, FPS = 640, 360, 12
YELLOW = (255, 214, 64)
WHITE = (240, 240, 240)
FONT = "C:/Windows/Fonts/consolab.ttf"
PORTRAITS = ROOT / "ArtSource" / "Import" / "Story"

# The take William records (see README in this folder for the shot list).
TAKE = VIDEOS / "teaser_take.mp4"


def clip(source, start, seconds, note, crop=None):
    return {"kind": "clip", "source": Path(source), "start": start, "seconds": seconds, "note": note, "crop": crop}


def card(text, seconds=1.8):
    return {"kind": "card", "text": text, "seconds": seconds}


def textbox(speaker, text, portrait, seconds=2.6):
    return {"kind": "textbox", "speaker": speaker, "text": text, "portrait": portrait, "seconds": seconds}


def montage(*clips):
    return {"kind": "montage", "clips": clips}


# Times are placeholders until the take exists; set them from the recording.
SHOTS = [
    clip(TAKE, 0.0, 2.6, "Nikki asleep. Phone buzzes, Carl's text slides up"),
    clip(TAKE, 0.0, 1.4, "BLOCKED stamp"),
    clip(TAKE, 0.0, 1.6, "Alarm: BEEP BEEP BEEP, 'Ugh. I guess I have to get ready.'"),
    clip(TAKE, 0.0, 1.4, "Right-click the bed: mace flies, WHAM"),
    clip(TAKE, 0.0, 1.2, "Closet: pyjamas to work clothes, POOF"),
    card("TWO YOUNG ADVENTURERS"),
    clip(TAKE, 0.0, 2.0, "Carl: 'Text your crush'"),
    clip(TAKE, 0.0, 1.4, "Carl is blocked, back to bed"),
    clip(TAKE, 0.0, 1.4, "The dog wakes him"),
    card("FORCED ON AN ANNOYING JOURNEY"),
    textbox("Sarah", "The job pays $5600.", "NPC_Sarah"),
    card("TO PAY THE BILLS"),
    montage(
        clip(TAKE, 0.0, 0.5, "Stage: Nikki's chain mid-wave"),
        clip(TAKE, 0.0, 0.5, "Carl's ricochet arrows"),
        clip(TAKE, 0.0, 0.5, "World map, walking to a stop"),
        clip(TAKE, 0.0, 0.5, "A different theme's stage"),
        clip(TAKE, 0.0, 0.5, "The shop on the map"),
        clip(TAKE, 0.0, 0.5, "Carl's volley"),
        clip(TAKE, 0.0, 0.6, "Nikki's guitar strum"),
    ),
    card("PROJECT RIOT", 2.6),
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


def render_textbox(shot, out):
    # A dialog box on its own, like the game's, with the line typing on.
    frames, total = [], round(shot["seconds"] * FPS)
    portrait = None
    path = PORTRAITS / f"{shot['portrait']}.png"
    if path.exists():
        portrait = Image.open(path).convert("RGBA")
        scale = 96 // max(portrait.size)
        portrait = portrait.resize((portrait.width * scale, portrait.height * scale), Image.NEAREST)
    for i in range(total):
        frame = Image.new("RGB", (W, H), (8, 6, 12))
        draw = ImageDraw.Draw(frame)
        box = (40, 200, W - 40, 330)
        draw.rectangle(box, fill=(20, 16, 28), outline=YELLOW, width=3)
        left = box[0] + 16
        if portrait:
            frame.paste(portrait, (left, box[1] + (box[3] - box[1] - portrait.height) // 2), portrait)
            left += portrait.width + 16
        draw.text((left, box[1] + 14), shot["speaker"].upper(), font=font(20), fill=YELLOW)
        draw.text((left, box[1] + 48), shot["text"][: i * 2], font=font(26), fill=WHITE)
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


def render_clip(shot, out):
    if not shot["source"].exists():
        render_missing(shot, out)
        return
    # Letterboxed, like a trailer: the game fills the middle band.
    crop = f"crop={shot['crop']}," if shot["crop"] else ""
    band = H - 2 * 24
    filters = f"{crop}scale={W}:{band}:force_original_aspect_ratio=decrease:flags=neighbor,pad={W}:{H}:(ow-iw)/2:(oh-ih)/2,fps={FPS}"
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
        elif shot["kind"] == "textbox":
            render_textbox(shot, next_out())
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
