# Story teaser

`make_teaser.py` cuts `Media/teaser.gif` from one OBS recording. Shots without footage show up as grey "SHOT NEEDED" cards, so it builds either way.

## Recording

Record one take and save it as `Videos/teaser_take.mp4`. Order doesn't matter; the start times in `SHOTS` pick the moments out.

1. A fresh save, as Nikki: the phone buzzes, the text, the block, the alarm, right-clicking the bed, the closet.
2. A fresh save, as Carl: texting the crush, the block, back to bed, the dog.
3. Quick gameplay, a few seconds each: Nikki's chain, Carl's ricochet and volley, Nikki's strum, two different stage themes, walking the world map, and the shop open on the map.

Skip the town. It isn't in the teaser.

## Building

Set each clip's start and length in `SHOTS`, then run:

```
python Media/teaser/make_teaser.py
```
