"""Nikki's route on the world map, shared by the art script and the Unreal asset script.

Each stage: (display name, theme Blueprint name, map position 0..1 from the top-left, is it a placeholder).
Index 0 is the home town, which is not a combat stage. Nikki travels down the map, Carl up.
"""
CARL_ROUTE = [
    ("Elf City", None, (0.46, 0.90), True),
    ("The Far Desert", "BP_Theme_FarDesert", (0.62, 0.74), True),
    ("Space", "BP_Theme_Space", (0.40, 0.58), False),
    ("The Desert", "BP_Theme_Desert", (0.64, 0.42), True),
    ("???", "BP_Theme_NikkiWorld", (0.50, 0.10), True),
]

NIKKI_ROUTE = [
    ("Rock County", None, (0.50, 0.10), False),
    ("The Dungeon", "BP_Theme_Dungeon", (0.36, 0.26), False),
    ("The Desert", "BP_Theme_Desert", (0.64, 0.42), True),
    ("Space", "BP_Theme_Space", (0.40, 0.58), False),
    ("The Far Desert", "BP_Theme_FarDesert", (0.62, 0.74), True),
    ("Blue Elf World", "BP_Theme_ElfWorld", (0.46, 0.90), True),
]

# Played after both routes are finished. Placeholder: it starts past the first stop.
FINAL_ROUTE = [
    ("The Desert", None, (0.64, 0.42), True),
    ("Space", "BP_Theme_Space", (0.40, 0.58), True),
    ("???", "BP_Theme_Dungeon", (0.50, 0.74), True),
]
