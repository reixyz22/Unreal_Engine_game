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

# Not characters: no glow, no outline.
PLAIN_SPRITES = {"T_ArenaFloor_Sprite", "T_Player_Sprite"}
BRIGHT_SPRITES = {"Pyro_Bead_Sprite", "Flail_Ball_Sprite"}

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
float3 Rgb = C.rgb * Tint * Glow;
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
float Fog = smoothstep(0.3, 0.85, N);
return Scene.rgb * (1.0 - Fog * 0.35) + float3(0.45, 0.32, 0.7) * Fog * 0.12;
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

    node = custom_node(material, SPRITE_HLSL, unreal.CustomMaterialOutputType.CMOT_FLOAT4,
                       ["Tex", "UV", "Tint", "TintA", "Outline", "OutlineA", "Glow"], -300)
    # Color pins carry RGB only, so each alpha comes in on its own pin.
    for source, out, pin in ((texture, "", "Tex"), (uv, "", "UV"), (tint, "", "Tint"), (tint, "A", "TintA"),
                             (outline, "", "Outline"), (outline, "A", "OutlineA"), (glow, "", "Glow")):
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

    node = custom_node(material, FOG_HLSL, unreal.CustomMaterialOutputType.CMOT_FLOAT3, ["Scene", "WP", "Time"], -300)
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
    sprite_material = build_sprite_material()
    bright = build_bright_instance(sprite_material)
    fog = build_fog_material()
    apply_sprite_materials(sprite_material, bright)

    pyro = blueprint("BP_PyroEnemy", unreal.PyroEnemy)
    set_defaults(pyro, body_anim=unreal.load_asset(f"{ART}/FB_PyroRun"),
                 bead_sprite=unreal.load_asset(f"{ART}/Pyro_Bead_Sprite"))
    set_defaults(unreal.load_asset(f"{CORE}/BP_ArenaGameMode"), enemy_types=[pyro.generated_class()])
    set_defaults(unreal.load_asset(f"{CORE}/BP_ArenaPlayerPawn"), fog_material=fog)
    unreal.log("setup_assets: done")


main()
