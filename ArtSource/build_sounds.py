"""Synthesize the game's sound effects into ArtSource/Sounds (16-bit mono WAV).

Pure Python, no samples: each sound is built from sine, noise and a plucked-string model.
"""
import math
import random
import struct
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


def main():
    chain_throw()
    chain_hit()
    beam_charge()
    beam_fire()
    guitar_strum()


if __name__ == "__main__":
    main()
