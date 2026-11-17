# Story teaser

`make_teaser.py` cuts `Media/teaser.gif` and `Media/teaser.mp4` from two OBS recordings of the editor, `Videos/2026-11-17 14-59-17.mp4` and `Videos/2026-11-17 15-09-02.mp4`. The recordings aren't in the repo.

Each shot in `SHOTS` names a recording, a start and end time, and a crop. A shot whose recording is missing becomes a grey "SHOT NEEDED" card, so it still builds.

```
python Media/teaser/make_teaser.py
```

## Wanted shots

- Nikki fighting in Space. Carl stands in for now.
- A ricochet arrow (key 1) bouncing off a wall.
