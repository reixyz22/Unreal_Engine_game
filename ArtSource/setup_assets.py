"""Build the editor assets that code can't: materials, enemy Blueprints, sprite settings.

Runs inside Unreal (editor closed):
  UnrealEditor-Cmd.exe Project_Riot.uproject -run=pythonscript -script=<this file>
Safe to re-run; every asset is rebuilt in place.
"""
import unreal

ART = "/Game/Art"
CORE = "/Game/Core"
MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()

# Theme tracks, imported separately; until they exist every theme plays the chiptune loop.
THEME_MUSIC = {"Dungeon": "/Game/Audio/BGM_Dungeon", "Space": "/Game/Audio/BGM_Space"}

# Not characters: no glow, no outline.
PLAIN_SPRITES = {"T_ArenaFloor_Sprite", "T_Player_Sprite", "T_Floor_NikkiWorld_Sprite", "T_Floor_Space_Sprite",
                 "UI_Panel_Sprite", "UI_Button_Sprite", "UI_Backdrop_Sprite"}
BRIGHT_SPRITES = {"Pyro_Bead_Sprite", "Flail_Ball_Sprite", "Cleric_Beam_Sprite", "Shock_Burst_Sprite",
                  "Shard_Mana_Sprite", "Shard_Health_Sprite"}
# Frames from build_art.py that get a texture and a sprite here.
FRAMES = ["Nikki_Guitar", "Cleric_Beam", "Chain_Link", "Shock_Burst", "Shard_Mana", "Shard_Health"] + [f"Cleric_Run_{i}" for i in range(4)] \
    + [f"Spaceman_Run_{i}" for i in range(4)] \
    + ["T_Floor_NikkiWorld", "T_Floor_Space", "UI_Panel", "UI_Button", "UI_Backdrop"]

SPRITE_HLSL = """
float W, H;
Tex.GetDimensions(W, H);
float2 Px = float2(1.0 / W, 1.0 / H);
float4 C = Texture2DSample(Tex, TexSampler, UV);
float N = Texture2DSample(Tex, TexSampler, UV + float2(Px.x, 0)).a;
N = max(N, Texture2DSample(Tex, TexSampler, UV - float2(Px.x, 0)).a);
N = max(N, Texture2DSample(Tex, TexSampler, UV + float2(0, Px.y)).a);
N = max(N, Texture2DSample(Tex, TexSampler, UV - float2(0, Px.y)).a);
float Edge = saturate(N - C.a) * OutlineA;
// Palette swap: brightness picks a color between Shadow and Light (a gradient map).
float Luma = dot(C.rgb, float3(0.299, 0.587, 0.114));
float3 Swapped = lerp(PaletteShadow, PaletteLight, Luma);
float3 Rgb = lerp(C.rgb, Swapped, PaletteStrength) * Tint * Glow;
return float4(lerp(Rgb, Outline, Edge), max(C.a * TintA, Edge));
"""

FOG_HLSL = """
float2 P = WP.xz / 520.0 + Time * float2(0.035, 0.012);
float N = 0;
float Amp = 0.55;
for (int I = 0; I < 4; I++)
{
    float2 Cell = floor(P);
    float2 F = frac(P);
    F = F * F * (3.0 - 2.0 * F);
    float A = frac(sin(dot(Cell, float2(127.1, 311.7))) * 43758.5453);
    float B = frac(sin(dot(Cell + float2(1, 0), float2(127.1, 311.7))) * 43758.5453);
    float C = frac(sin(dot(Cell + float2(0, 1), float2(127.1, 311.7))) * 43758.5453);
    float D = frac(sin(dot(Cell + float2(1, 1), float2(127.1, 311.7))) * 43758.5453);
    N += Amp * lerp(lerp(A, B, F.x), lerp(C, D, F.x), F.y);
    P = P * 2.07 + float2(5.2, 1.3);
    Amp *= 0.5;
}
float Fog = smoothstep(0.3, 0.85, N) * FogDensity;
return Scene.rgb * (1.0 - Fog) + FogColor * Fog * 0.35;
"""


def fresh_material(name):
    path = f"{ART}/{name}"
    if EAL.does_asset_exist(path):
        material = unreal.load_asset(path)
        MEL.delete_all_material_expressions(material)
        return material
    return TOOLS.create_asset(name, ART, unreal.Material, unreal.MaterialFactoryNew())


def custom_node(material, code, output_type, inputs, x):
    node = MEL.create_material_expression(material, unreal.MaterialExpressionCustom, x, 0)
    node.set_editor_property("code", code)
    node.set_editor_property("output_type", output_type)
    pins = []
    for name in inputs:
        pin = unreal.CustomInput()
        pin.set_editor_property("input_name", name)
        pins.append(pin)
    node.set_editor_property("inputs", pins)
    return node


def vector_param(material, name, default, y):
    node = MEL.create_material_expression(material, unreal.MaterialExpressionVectorParameter, -600, y)
    node.set_editor_property("parameter_name", name)
    node.set_editor_property("default_value", default)
    return node


def scalar_param(material, name, default, y):
    node = MEL.create_material_expression(material, unreal.MaterialExpressionScalarParameter, -600, y)
    node.set_editor_property("parameter_name", name)
    node.set_editor_property("default_value", default)
    return node


def mask(material, source, channels, x, y):
    node = MEL.create_material_expression(material, unreal.MaterialExpressionComponentMask, x, y)
    for channel in "rgba":
        node.set_editor_property(channel, channel in channels)
    MEL.connect_material_expressions(source, "", node, "")
    return node


def build_sprite_material():
    material = fresh_material("M_ArenaSprite")
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    material.set_editor_property("two_sided", True)

    texture = MEL.create_material_expression(material, unreal.MaterialExpressionTextureObjectParameter, -600, -200)
    texture.set_editor_property("parameter_name", "SpriteTexture")
    texture.set_editor_property("texture", unreal.load_asset(f"{ART}/Pyro_Run_0"))
    uv = MEL.create_material_expression(material, unreal.MaterialExpressionTextureCoordinate, -600, -50)
    tint = MEL.create_material_expression(material, unreal.MaterialExpressionVertexColor, -600, 50)
    outline = MEL.create_material_expression(material, unreal.MaterialExpressionVectorParameter, -600, 200)
    outline.set_editor_property("parameter_name", "OutlineColor")
    outline.set_editor_property("default_value", unreal.LinearColor(0, 0, 0, 0))
    glow = MEL.create_material_expression(material, unreal.MaterialExpressionScalarParameter, -600, 350)
    glow.set_editor_property("parameter_name", "Glow")
    glow.set_editor_property("default_value", 1.35)
    shadow = vector_param(material, "PaletteShadow", unreal.LinearColor(0, 0, 0, 1), 500)
    light = vector_param(material, "PaletteLight", unreal.LinearColor(1, 1, 1, 1), 650)
    strength = scalar_param(material, "PaletteStrength", 0.0, 800)

    node = custom_node(material, SPRITE_HLSL, unreal.CustomMaterialOutputType.CMOT_FLOAT4,
                       ["Tex", "UV", "Tint", "TintA", "Outline", "OutlineA", "Glow",
                        "PaletteShadow", "PaletteLight", "PaletteStrength"], -300)
    # Color pins carry RGB only, so each alpha comes in on its own pin.
    for source, out, pin in ((texture, "", "Tex"), (uv, "", "UV"), (tint, "", "Tint"), (tint, "A", "TintA"),
                             (outline, "", "Outline"), (outline, "A", "OutlineA"), (glow, "", "Glow"),
                             (shadow, "", "PaletteShadow"), (light, "", "PaletteLight"), (strength, "", "PaletteStrength")):
        MEL.connect_material_expressions(source, out, node, pin)

    MEL.connect_material_property(mask(material, node, "rgb", -100, -50), "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    MEL.connect_material_property(mask(material, node, "a", -100, 100), "", unreal.MaterialProperty.MP_OPACITY_MASK)
    MEL.recompile_material(material)
    EAL.save_loaded_asset(material, False)
    return material


def build_bright_instance(parent):
    path = f"{ART}/MI_ArenaSprite_Bright"
    if EAL.does_asset_exist(path):
        instance = unreal.load_asset(path)
    else:
        instance = TOOLS.create_asset("MI_ArenaSprite_Bright", ART, unreal.MaterialInstanceConstant,
                                      unreal.MaterialInstanceConstantFactoryNew())
    instance.set_editor_property("parent", parent)
    MEL.set_material_instance_scalar_parameter_value(instance, "Glow", 3.0)
    EAL.save_loaded_asset(instance, False)
    return instance


def build_fog_material():
    material = fresh_material("M_ArenaFog")
    material.set_editor_property("material_domain", unreal.MaterialDomain.MD_POST_PROCESS)
    material.set_editor_property("blendable_location", unreal.BlendableLocation.BL_SCENE_COLOR_BEFORE_BLOOM)

    scene = MEL.create_material_expression(material, unreal.MaterialExpressionSceneTexture, -600, 0)
    scene.set_editor_property("scene_texture_id", unreal.SceneTextureId.PPI_POST_PROCESS_INPUT0)
    world = MEL.create_material_expression(material, unreal.MaterialExpressionWorldPosition, -600, 150)
    time = MEL.create_material_expression(material, unreal.MaterialExpressionTime, -600, 300)

    fog_color = vector_param(material, "FogColor", unreal.LinearColor(0.45, 0.32, 0.7, 1), 450)
    fog_density = scalar_param(material, "FogDensity", 0.35, 600)
    node = custom_node(material, FOG_HLSL, unreal.CustomMaterialOutputType.CMOT_FLOAT3,
                       ["Scene", "WP", "Time", "FogColor", "FogDensity"], -300)
    MEL.connect_material_expressions(fog_color, "", node, "FogColor")
    MEL.connect_material_expressions(fog_density, "", node, "FogDensity")
    MEL.connect_material_expressions(scene, "Color", node, "Scene")
    MEL.connect_material_expressions(world, "", node, "WP")
    MEL.connect_material_expressions(time, "", node, "Time")
    MEL.connect_material_property(node, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    MEL.recompile_material(material)
    EAL.save_loaded_asset(material, False)
    return material


def apply_sprite_materials(sprite_material, bright):
    for path in EAL.list_assets(ART, recursive=False):
        asset = unreal.load_asset(path)
        name = asset.get_name()
        if isinstance(asset, unreal.PaperSprite) and name not in PLAIN_SPRITES:
            geometry = asset.get_editor_property("render_geometry")
            geometry.set_editor_property("geometry_type", unreal.SpritePolygonMode.SOURCE_BOUNDING_BOX)
            asset.set_editor_property("render_geometry", geometry)
            asset.set_editor_property("default_material", bright if name in BRIGHT_SPRITES else sprite_material)
            EAL.save_loaded_asset(asset, False)
        elif isinstance(asset, unreal.PaperFlipbook):
            asset.set_editor_property("default_material", sprite_material)
            EAL.save_loaded_asset(asset, False)


def reimport_floor():
    task = unreal.AssetImportTask()
    task.filename = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_content_dir() + "Art/T_ArenaFloor.png")
    task.destination_path = ART
    task.replace_existing = True
    task.automated = True
    task.save = True
    TOOLS.import_asset_tasks([task])


def fit_floor_sprite():
    sprite = unreal.load_asset(f"{ART}/T_ArenaFloor_Sprite")
    texture = unreal.load_asset(f"{ART}/T_ArenaFloor")
    size = unreal.Vector2D(texture.blueprint_get_size_x(), texture.blueprint_get_size_y())
    sprite.set_editor_property("source_uv", unreal.Vector2D(0, 0))
    sprite.set_editor_property("source_dimension", size)
    EAL.save_loaded_asset(sprite, False)


def import_frames():
    folder = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir() + "../ArtSource/Import/")
    tasks = []
    for name in FRAMES:
        task = unreal.AssetImportTask()
        task.filename = folder + name + ".png"
        task.destination_path = ART
        task.replace_existing = True
        task.automated = True
        task.save = False
        tasks.append(task)
    TOOLS.import_asset_tasks(tasks)

    for name in FRAMES:
        texture = unreal.load_asset(f"{ART}/{name}")
        texture.set_editor_property("filter", unreal.TextureFilter.TF_NEAREST)
        texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
        texture.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
        texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_PIXELS2D)
        EAL.save_loaded_asset(texture, False)

        sprite_path = f"{ART}/{name}_Sprite"
        sprite = unreal.load_asset(sprite_path) if EAL.does_asset_exist(sprite_path) else             TOOLS.create_asset(f"{name}_Sprite", ART, unreal.PaperSprite, unreal.PaperSpriteFactory())
        sprite.set_editor_property("source_texture", texture)
        sprite.set_editor_property("source_uv", unreal.Vector2D(0, 0))
        sprite.set_editor_property("source_dimension", unreal.Vector2D(texture.blueprint_get_size_x(), texture.blueprint_get_size_y()))
        if name.startswith("T_Floor_"):
            # Theme floors must match the level floor's scale (4 world units per pixel, DECISIONS.md 002).
            level_floor = unreal.load_asset(f"{ART}/T_ArenaFloor_Sprite")
            sprite.set_editor_property("pixels_per_unreal_unit", level_floor.get_editor_property("pixels_per_unreal_unit"))
        EAL.save_loaded_asset(sprite, False)


def import_sounds():
    """Import the WAVs from build_sounds.py into /Game/Audio and return them by name."""
    folder = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir() + "../ArtSource/Sounds/")
    names = ["SFX_ChainThrow", "SFX_ChainHit", "SFX_BeamCharge", "SFX_BeamFire", "SFX_Strum"]
    tasks = []
    for name in names:
        task = unreal.AssetImportTask()
        task.filename = folder + name + ".wav"
        task.destination_path = "/Game/Audio"
        task.replace_existing = True
        task.automated = True
        task.save = True
        tasks.append(task)
    TOOLS.import_asset_tasks(tasks)
    return {name: unreal.load_asset(f"/Game/Audio/{name}") for name in names}


def import_music():
    """Import the theme tracks (made by build_sounds.py) and make them loop."""
    folder = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir() + "../ArtSource/Music/")
    tasks = []
    for path in THEME_MUSIC.values():
        name = path.rsplit("/", 1)[1]
        task = unreal.AssetImportTask()
        task.filename = folder + name + ".wav"
        task.destination_path = "/Game/Audio"
        task.replace_existing = True
        task.automated = True
        task.save = False
        tasks.append(task)
    TOOLS.import_asset_tasks(tasks)
    for path in THEME_MUSIC.values():
        track = unreal.load_asset(path)
        track.set_editor_property("looping", True)
        EAL.save_loaded_asset(track, False)


def flipbook(name, sprite_names, fps):
    path = f"{ART}/{name}"
    book = unreal.load_asset(path) if EAL.does_asset_exist(path) else         TOOLS.create_asset(name, ART, unreal.PaperFlipbook, unreal.PaperFlipbookFactory())
    frames = []
    for sprite_name in sprite_names:
        frame = unreal.PaperFlipbookKeyFrame()
        frame.set_editor_property("sprite", unreal.load_asset(f"{ART}/{sprite_name}"))
        frame.set_editor_property("frame_run", 1)
        frames.append(frame)
    book.set_editor_property("frames_per_second", fps)
    book.set_editor_property("key_frames", frames)
    EAL.save_loaded_asset(book, False)
    return book


def spawn_rule(enemy_bp, first_round, weight):
    rule = unreal.EnemySpawnRule()
    rule.set_editor_property("type", enemy_bp.generated_class())
    rule.set_editor_property("first_round", first_round)
    rule.set_editor_property("weight", weight)
    return rule


def colour(r, g, b):
    return unreal.LinearColor(r, g, b, 1.0)


def skin(enemy_bp, shadow, light, strength, anim=None, projectile=None):
    entry = unreal.EnemySkin()
    if projectile:
        entry.set_editor_property("projectile_tint", projectile)
    entry.set_editor_property("type", enemy_bp.generated_class())
    if anim:
        entry.set_editor_property("anim", anim)
    entry.set_editor_property("shadow", shadow)
    entry.set_editor_property("light", light)
    entry.set_editor_property("strength", strength)
    return entry


def build_themes(pyro, cleric, music, spaceman):
    """One Blueprint per theme; each overrides only what differs from UArenaTheme's defaults."""
    dungeon = blueprint("BP_Theme_Dungeon", unreal.ArenaTheme)
    set_defaults(dungeon, display_name=unreal.Text("Dungeon"), music=music["Dungeon"],
                 floor_tint=colour(0.85, 0.82, 0.78), fog_color=colour(0.8, 0.87, 1.0), fog_density=0.45)

    space = blueprint("BP_Theme_Space", unreal.ArenaTheme)
    set_defaults(space, display_name=unreal.Text("Space"), music=music["Space"],
                 # Kept below the bloom threshold so the white corridor doesn't wash out.
                 floor_sprite=unreal.load_asset(f"{ART}/T_Floor_Space_Sprite"),
                 floor_tint=colour(0.52, 0.54, 0.6), fog_color=colour(0.08, 0.12, 0.38), fog_density=0.4,
                 menu_tint=colour(0.75, 0.85, 1.0),
                 # Pyros become William's spacemen: their own art, so no palette swap.
                 enemy_skins=[skin(pyro, colour(0, 0, 0), colour(1, 1, 1), 0.0, spaceman, projectile=colour(0.25, 1.0, 0.35)),
                              skin(cleric, colour(0.02, 0.0, 0.02), colour(0.95, 0.1, 0.15), 0.95)])

    nikki = blueprint("BP_Theme_NikkiWorld", unreal.ArenaTheme)
    set_defaults(nikki, display_name=unreal.Text("Nikki's World"), music=music["NikkiWorld"],
                 floor_sprite=unreal.load_asset(f"{ART}/T_Floor_NikkiWorld_Sprite"),
                 floor_tint=colour(1.0, 1.0, 1.0), fog_color=colour(0.45, 0.32, 0.7), fog_density=0.35,
                 menu_tint=colour(1.0, 0.8, 0.92),
                 enemy_skins=[skin(pyro, colour(0.01, 0.01, 0.02), colour(1.0, 0.2, 0.6), 0.85, projectile=colour(0.03, 0.03, 0.04)),
                              skin(cleric, colour(0.01, 0.01, 0.02), colour(1.0, 0.2, 0.6), 0.85)])
    return [dungeon, space, nikki]


def blueprint(name, parent_class):
    path = f"{CORE}/{name}"
    if EAL.does_asset_exist(path):
        return unreal.load_asset(path)
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", parent_class)
    return TOOLS.create_asset(name, CORE, unreal.Blueprint, factory)


def set_defaults(bp, **values):
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    defaults = unreal.get_default_object(bp.generated_class())
    for key, value in values.items():
        defaults.set_editor_property(key, value)
    EAL.save_loaded_asset(bp, False)


def main():
    reimport_floor()
    fit_floor_sprite()
    import_frames()
    sounds = import_sounds()
    cleric_run = flipbook("FB_ClericRun", [f"Cleric_Run_{i}_Sprite" for i in range(4)], 8.0)
    sprite_material = build_sprite_material()
    bright = build_bright_instance(sprite_material)
    fog = build_fog_material()
    apply_sprite_materials(sprite_material, bright)

    pyro = blueprint("BP_PyroEnemy", unreal.PyroEnemy)
    set_defaults(pyro, body_anim=unreal.load_asset(f"{ART}/FB_PyroRun"),
                 bead_sprite=unreal.load_asset(f"{ART}/Pyro_Bead_Sprite"))
    cleric = blueprint("BP_ClericEnemy", unreal.ClericEnemy)
    set_defaults(cleric, body_anim=cleric_run,
                 beam_sprite=unreal.load_asset(f"{ART}/Cleric_Beam_Sprite"),
                 mana_shard_sprite=unreal.load_asset(f"{ART}/Shard_Mana_Sprite"),
                 health_shard_sprite=unreal.load_asset(f"{ART}/Shard_Health_Sprite"),
                 beam_charge_sound=sounds["SFX_BeamCharge"], beam_fire_sound=sounds["SFX_BeamFire"])
    hud = blueprint("BP_ArenaHUD", unreal.ArenaHUD)
    set_defaults(hud, panel_texture=unreal.load_asset(f"{ART}/UI_Panel"),
                 button_texture=unreal.load_asset(f"{ART}/UI_Button"),
                 backdrop_texture=unreal.load_asset(f"{ART}/UI_Backdrop"))
    import_music()
    chiptune = unreal.load_asset("/Game/Audio/chiptune-police_loop")
    theme_music = {"Dungeon": chiptune, "Space": chiptune, "NikkiWorld": chiptune}
    for name, path in THEME_MUSIC.items():
        if EAL.does_asset_exist(path):
            theme_music[name] = unreal.load_asset(path)
    spaceman = flipbook("FB_SpacemanRun", [f"Spaceman_Run_{i}_Sprite" for i in range(4)], 8.0)
    themes = build_themes(pyro, cleric, theme_music, spaceman)
    nikki_bp = unreal.load_asset(f"{CORE}/BP_ArenaPlayerPawn")
    # Pyros fill every wave; Clerics join from wave 3 at a fifth of the wave.
    set_defaults(unreal.load_asset(f"{CORE}/BP_ArenaGameMode"),
                 spawn_rules=[spawn_rule(pyro, 1, 4.0), spawn_rule(cleric, 3, 1.0)],
                 hud_class=hud.generated_class(),
                 heroes=[nikki_bp.generated_class()],
                 themes=[theme.generated_class() for theme in themes])
    set_defaults(unreal.load_asset(f"{CORE}/BP_ArenaPlayerPawn"), fog_material=fog,
                 guitar_sprite=unreal.load_asset(f"{ART}/Nikki_Guitar_Sprite"),
                 shock_burst_sprite=unreal.load_asset(f"{ART}/Shock_Burst_Sprite"),
                 throw_sound=sounds["SFX_ChainThrow"], chain_hit_sound=sounds["SFX_ChainHit"],
                 strum_sound=sounds["SFX_Strum"],
                 chain_link_sprite=unreal.load_asset(f"{ART}/Chain_Link_Sprite"))
    unreal.log("setup_assets: done")


main()
