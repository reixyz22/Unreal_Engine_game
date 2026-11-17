"""Synthesize the game's sound effects into ArtSource/Sounds (16-bit mono WAV).

Pure Python, no samples: each sound is built from sine, noise and a plucked-string model.
"""
import math
import random
import struct
import subprocess
import wave
from pathlib import Path

RATE = 44100
OUT = Path(__file__).parent / "Sounds"
rng = random.Random(3)


def write(name, samples, volume=0.8):
    peak = max(1e-9, max(abs(s) for s in samples))
    OUT.mkdir(exist_ok=True)
    with wave.open(str(OUT / f"{name}.wav"), "wb") as out:
        out.setnchannels(1)
        out.setsampwidth(2)
        out.setframerate(RATE)
        out.writeframes(b"".join(struct.pack("<h", int(s / peak * volume * 32767)) for s in samples))


def seconds(length):
    return range(int(length * RATE))


def lowpass(samples, amount):
    out, last = [], 0.0
    for s in samples:
        last += (s - last) * amount
        out.append(last)
    return out


def chain_throw():
    """A whoosh (filtered noise swelling and fading) with a few chain-link clicks."""
    length = 0.28
    noise = lowpass([rng.uniform(-1, 1) for _ in seconds(length)], 0.18)
    whoosh = [n * math.sin(math.pi * i / (length * RATE)) ** 2 for i, n in enumerate(noise)]
    for click_at in (0.03, 0.07, 0.11, 0.15):
        start = int(click_at * RATE)
        for k in range(200):
            whoosh[start + k] += 0.5 * math.sin(2 * math.pi * 2400 * k / RATE) * math.exp(-k / 40)
    write("SFX_ChainThrow", whoosh, 0.55)


def chain_hit():
    """A heavy thunk: a falling low tone under a short noise crack."""
    out = []
    for i in seconds(0.22):
        t = i / RATE
        tone = math.sin(2 * math.pi * (140 - 200 * t) * t) * math.exp(-t * 22)
        crack = rng.uniform(-1, 1) * math.exp(-t * 90)
        out.append(tone + 0.6 * crack)
    write("SFX_ChainHit", out, 0.8)


def beam_charge():
    """A rising whine with a shimmer, as long as the Cleric's warning line."""
    out, phase = [], 0.0
    length = 0.8
    for i in seconds(length):
        t = i / RATE
        phase += 2 * math.pi * (300 + 700 * (t / length) ** 2) / RATE
        shimmer = 0.6 + 0.4 * math.sin(2 * math.pi * 18 * t)
        out.append(math.sin(phase) * shimmer * min(1.0, t / 0.1))
    write("SFX_BeamCharge", out, 0.3)


def beam_fire():
    """A bright zap: a falling sawtooth sweep over a noise burst."""
    out, phase = [], 0.0
    for i in seconds(0.6):
        t = i / RATE
        phase += (1400 * math.exp(-t * 5) + 120) / RATE
        saw = 2 * (phase % 1.0) - 1
        out.append((0.7 * saw + 0.5 * rng.uniform(-1, 1) * math.exp(-t * 12)) * math.exp(-t * 4))
    write("SFX_BeamFire", lowpass(out, 0.5), 0.6)


def guitar_strum():
    """A strummed E major chord using the Karplus-Strong plucked-string model."""
    notes = [82.4, 123.5, 164.8, 207.7, 246.9, 329.6]
    length = int(1.3 * RATE)
    mix = [0.0] * length
    for index, freq in enumerate(notes):
        period = int(RATE / freq)
        buffer = [rng.uniform(-1, 1) for _ in range(period)]
        start = index * int(0.014 * RATE)
        for i in range(length - start):
            current = buffer[i % period]
            buffer[i % period] = 0.996 * 0.5 * (current + buffer[(i + 1) % period])
            mix[start + i] += current
    write("SFX_Strum", mix, 0.75)


MUSIC = Path(__file__).parent / "Music"
# Downloaded theme tracks -> the WAV name setup_assets.py imports. Needs ffmpeg on PATH.
TRACKS = {"dungeon002.ogg": "BGM_Dungeon", "outer_space.mp3": "BGM_Space"}


def convert_music():
    """Loudness-match the tracks and write mono 32 kHz WAVs (Unreal imports WAV)."""
    for source, name in TRACKS.items():
        subprocess.run(["ffmpeg", "-v", "error", "-y", "-i", str(MUSIC / source),
                        "-af", "loudnorm=I=-18:TP=-2", "-ac", "1", "-ar", "32000", str(MUSIC / f"{name}.wav")], check=True)



# ---------------------------------------------------------------- story sounds

def tone(freq, length, shape="square", decay=6.0, vibrato=0.0):
    out = []
    for i in seconds(length):
        t = i / RATE
        phase = freq * t + vibrato * math.sin(2 * math.pi * 5 * t)
        frac = phase % 1.0
        wave_value = (1.0 if frac < 0.5 else -1.0) if shape == "square" else 4 * abs(frac - 0.5) - 1
        out.append(wave_value * math.exp(-t * decay))
    return out


def story_sfx():
    """Short UI and story sounds: phone buzz, alarm, text blip, bed smash, poof, stamp, coin, fanfare."""
    buzz = []
    for burst in range(2):
        buzz += [math.sin(2 * math.pi * 150 * i / RATE) * (1 if (i // 220) % 2 else 0.4) for i in seconds(0.22)]
        buzz += [0.0] * int(0.12 * RATE)
    write("SFX_PhoneBuzz", buzz + tone(1320, 0.12) + tone(1760, 0.2), 0.5)
    write("SFX_Blip", tone(880, 0.04, decay=30), 0.25)
    write("SFX_Select", tone(660, 0.06, decay=20) + tone(990, 0.08, decay=20), 0.35)

    smash = []
    for i in seconds(0.5):
        t = i / RATE
        smash.append(math.sin(2 * math.pi * (90 - 60 * t) * t) * math.exp(-t * 9) + rng.uniform(-1, 1) * math.exp(-t * 14))
    write("SFX_Smash", lowpass(smash, 0.4), 0.9)

    poof = lowpass([rng.uniform(-1, 1) * math.sin(math.pi * i / (0.3 * RATE)) for i in seconds(0.3)], 0.25)
    write("SFX_Poof", poof + tone(1047, 0.1, "triangle") + tone(1568, 0.25, "triangle"), 0.5)
    write("SFX_Stamp", lowpass([rng.uniform(-1, 1) * math.exp(-i / RATE * 40) for i in seconds(0.15)], 0.3), 0.9)
    alarm = []
    for _ in range(3):
        for _ in range(3):
            alarm += tone(2093, 0.07, decay=4) + [0.0] * int(0.05 * RATE)
        alarm += [0.0] * int(0.25 * RATE)
    write("SFX_Alarm", alarm, 0.35)
    write("SFX_Coin", tone(988, 0.07, decay=10) + tone(1319, 0.3, decay=8), 0.4)
    fanfare = []
    for freq, length in ((523, 0.12), (659, 0.12), (784, 0.12), (1047, 0.5)):
        fanfare += tone(freq, length, decay=3)
    write("SFX_Fanfare", fanfare, 0.5)


def note_freq(name):
    """'C4', 'F#3', or '-' for a rest."""
    if name == "-":
        return 0.0
    semitone = {"C": 0, "D": 2, "E": 4, "F": 5, "G": 7, "A": 9, "B": 11}[name[0]] + (1 if "#" in name else 0)
    return 440.0 * 2 ** ((semitone - 9) / 12 + int(name[-1]) - 4)


def chiptune(name, bpm, melody, bass, volume=0.5):
    """A two-voice loop: square-wave melody (eighth notes) over a triangle bass (half notes).
    Placeholder music, written here so each town has its own tune until real tracks arrive."""
    eighth = 60.0 / bpm / 2
    total = int(len(melody) * eighth * RATE)
    mix = [0.0] * total
    for voice, step, shape, level, decay in ((melody, eighth, "square", 0.5, 5.0), (bass, eighth * 4, "triangle", 0.8, 1.2)):
        for index, note in enumerate(voice):
            freq = note_freq(note)
            if not freq:
                continue
            start = int(index * step * RATE)
            for offset, sample in enumerate(tone(freq, step * 0.95, shape, decay)):
                if start + offset < total:
                    mix[start + offset] += sample * level
    MUSIC.mkdir(exist_ok=True)
    peak = max(abs(v) for v in mix)
    with wave.open(str(MUSIC / f"{name}.wav"), "wb") as out:
        out.setnchannels(1)
        out.setsampwidth(2)
        out.setframerate(RATE)
        out.writeframes(b"".join(struct.pack("<h", int(v / peak * volume * 32767)) for v in mix))


def placeholder_music():
    town = ("E4 G4 C5 G4 E4 G4 C5 E5 D5 B4 G4 B4 D5 - B4 - "
            "C5 A4 F4 A4 C5 A4 F5 E5 D5 C5 B4 D5 C5 - - - "
            "E4 G4 C5 G4 E5 D5 C5 E5 F5 E5 D5 B4 G4 - B4 - "
            "A4 C5 F5 A4 G4 B4 D5 G4 C5 E5 G5 E5 C5 - - -").split()
    chiptune("BGM_Town", 104, town, "C3 C3 G2 G2 F2 F2 G2 C3 C3 C3 G2 G2 F2 G2 C3 C3".split(), 0.45)
    world = ("A4 - C5 E5 A5 - E5 C5 G4 - B4 D5 G5 - D5 B4 "
             "F4 - A4 C5 F5 - C5 A4 E4 - G#4 B4 E5 - - - "
             "A4 - C5 E5 A5 - E5 C5 G4 - B4 D5 G5 - D5 B4 "
             "F4 A4 C5 F5 E5 D5 C5 B4 A4 - - - A4 - - -").split()
    chiptune("BGM_Map", 120, world, "A2 A2 G2 G2 F2 F2 E2 E2 A2 A2 G2 G2 F2 F2 A2 A2".split(), 0.45)


def main():
    convert_music()
    chain_throw()
    chain_hit()
    beam_charge()
    beam_fire()
    guitar_strum()
    story_sfx()
    placeholder_music()


if __name__ == "__main__":
    main()
