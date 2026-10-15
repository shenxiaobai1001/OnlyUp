import unreal

ASSET_PATH = "/Game/ModAsset/TaiQiu/caizhi"
ASSET_NAME = "M_TaiQiuBall"
ASSET_FULL = f"{ASSET_PATH}/{ASSET_NAME}"
DEFAULT_TEX = "/Game/ModAsset/TaiQiu/Tuxture/1_yellow"
PARAM_NAME = "BallTexture"


def main():
    if unreal.EditorAssetLibrary.does_asset_exist(ASSET_FULL):
        unreal.EditorAssetLibrary.delete_asset(ASSET_FULL)

    factory = unreal.MaterialFactoryNew()
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    material = asset_tools.create_asset(ASSET_NAME, ASSET_PATH, unreal.Material, factory)
    if not material:
        unreal.log_error("[TaiQiu] Failed to create M_TaiQiuBall")
        return

    tex_expr = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionTextureSampleParameter2D, -384, 0
    )
    tex_expr.set_editor_property("parameter_name", PARAM_NAME)

    default_tex = unreal.EditorAssetLibrary.load_asset(DEFAULT_TEX)
    if default_tex:
        tex_expr.set_editor_property("texture", default_tex)

    unreal.MaterialEditingLibrary.connect_material_property(
        tex_expr, "RGB", unreal.MaterialProperty.MP_BASE_COLOR
    )

    # Mild roughness so balls don't look pure plastic-white.
    rough = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionConstant, -384, 200
    )
    rough.set_editor_property("r", 0.45)
    unreal.MaterialEditingLibrary.connect_material_property(
        rough, "", unreal.MaterialProperty.MP_ROUGHNESS
    )

    unreal.MaterialEditingLibrary.layout_material_expressions(material)
    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_asset(ASSET_FULL, only_if_is_dirty=False)
    unreal.log(f"[TaiQiu] Created material {ASSET_FULL} with param {PARAM_NAME}")


if __name__ == "__main__":
    main()
