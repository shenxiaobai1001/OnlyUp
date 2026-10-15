"""
Smart collision setup for climbing maps.

Rules:
- Default mode "auto":
    * Fence / wire / net / grill-like meshes  -> 1 BOX (outline only)
    * Everything else (planes, buildings...) -> Use Complex As Simple (keeps mesh shape)
- Each mesh keeps its OWN collision (no merging nearby actors)

How to run (Editor Output Log -> Python):

  import importlib, fix_static_mesh_collision as f
  importlib.reload(f)

  # Recommended for current level
  f.fix_current_level(mode="auto")

  # Or:
  f.fix_selected(mode="auto")
  f.fix_folder("/Game/SteamRebuild/Assets/...", mode="auto")

Modes:
  auto     - fence-like -> box; others -> complex shape
  box      - force single box for all
  complex  - force complex-as-simple for all
  convex   - force 1 simplified convex hull for all
  dop26    - force 26-DOP for all

After: Ctrl+Shift+S, then View Mode -> Player Collision.
"""

from __future__ import annotations

import unreal

# Name keywords treated as "outline box only"
FENCE_NAME_KEYWORDS = (
    "fence",
    "wire",
    "barbed",
    "chainlink",
    "chain_link",
    "chain-link",
    "net",
    "grill",
    "grate",
    "lattice",
    "railing",
    "meshfence",
    "wirefence",
    "fence_wire",
    "铁丝",
    "围栏",
    "网",
)

# If the thinnest axis / longest axis is below this, treat as fence-like panel
THIN_RATIO_THRESHOLD = 0.08

# Convex fallback settings (mode="convex")
CONVEX_HULL_COUNT = 1
CONVEX_MAX_HULL_VERTS = 16
CONVEX_HULL_PRECISION = 200000


def _sm_subsystem():
    try:
        return unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    except Exception:  # noqa: BLE001
        return None


def _remove_collisions(mesh: unreal.StaticMesh) -> None:
    sub = _sm_subsystem()
    if sub:
        try:
            sub.remove_collisions(mesh)
            return
        except Exception:  # noqa: BLE001
            pass
    unreal.EditorStaticMeshLibrary.remove_collisions(mesh)


def _add_simple_shape(mesh: unreal.StaticMesh, shape) -> None:
    sub = _sm_subsystem()
    if sub:
        try:
            sub.add_simple_collisions(mesh, shape)
            return
        except Exception:  # noqa: BLE001
            pass
    unreal.EditorStaticMeshLibrary.add_simple_collisions(mesh, shape)


def _add_convex(mesh: unreal.StaticMesh) -> None:
    sub = _sm_subsystem()
    if sub:
        try:
            sub.set_convex_decomposition_collisions(
                mesh, CONVEX_HULL_COUNT, CONVEX_MAX_HULL_VERTS, CONVEX_HULL_PRECISION
            )
            return
        except Exception:  # noqa: BLE001
            pass
    unreal.EditorStaticMeshLibrary.set_convex_decomposition_collisions(
        mesh, CONVEX_HULL_COUNT, CONVEX_MAX_HULL_VERTS, CONVEX_HULL_PRECISION
    )


def _set_trace_flag(mesh: unreal.StaticMesh, flag) -> None:
    body_setup = mesh.get_editor_property("body_setup")
    if not body_setup:
        _add_simple_shape(mesh, unreal.ScriptingCollisionShapeType.BOX)
        body_setup = mesh.get_editor_property("body_setup")
    if body_setup:
        body_setup.set_editor_property("collision_trace_flag", flag)


def _mesh_name_lower(mesh: unreal.StaticMesh) -> str:
    return (mesh.get_name() or "").lower()


def _is_fence_like_by_name(mesh: unreal.StaticMesh) -> bool:
    name = _mesh_name_lower(mesh)
    path = (mesh.get_path_name() or "").lower()
    text = f"{name} {path}"
    return any(k in text for k in FENCE_NAME_KEYWORDS)


def _is_fence_like_by_bounds(mesh: unreal.StaticMesh) -> bool:
    """Thin panel / sheet detection from mesh bounds."""
    try:
        bounds = mesh.get_bounds()
        extent = bounds.box_extent
        dims = sorted([abs(extent.x), abs(extent.y), abs(extent.z)])
        longest = dims[2]
        if longest <= 1e-3:
            return False
        return (dims[0] / longest) <= THIN_RATIO_THRESHOLD
    except Exception:  # noqa: BLE001
        return False


def is_fence_like(mesh: unreal.StaticMesh) -> bool:
    return _is_fence_like_by_name(mesh) or _is_fence_like_by_bounds(mesh)


def _apply_box_outline(mesh: unreal.StaticMesh) -> None:
    _remove_collisions(mesh)
    _add_simple_shape(mesh, unreal.ScriptingCollisionShapeType.BOX)
    _set_trace_flag(mesh, unreal.CollisionTraceFlag.CTF_USE_SIMPLE_AS_COMPLEX)


def _apply_complex_shape(mesh: unreal.StaticMesh) -> None:
    # Keep mesh triangle collision for gameplay queries (plane / buildings / climbables)
    # Clear bad simple hulls so Player Collision view shows the real shape.
    _remove_collisions(mesh)
    _set_trace_flag(mesh, unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)


def _apply_convex(mesh: unreal.StaticMesh) -> None:
    _remove_collisions(mesh)
    _add_convex(mesh)
    _set_trace_flag(mesh, unreal.CollisionTraceFlag.CTF_USE_SIMPLE_AS_COMPLEX)


def _apply_dop26(mesh: unreal.StaticMesh) -> None:
    _remove_collisions(mesh)
    _add_simple_shape(mesh, unreal.ScriptingCollisionShapeType.NDOP26)
    _set_trace_flag(mesh, unreal.CollisionTraceFlag.CTF_USE_SIMPLE_AS_COMPLEX)


def _apply_for_mode(mesh: unreal.StaticMesh, mode: str) -> str:
    mode = (mode or "auto").lower()
    try:
        if mode == "auto":
            if is_fence_like(mesh):
                _apply_box_outline(mesh)
                kind = "box(fence)"
            else:
                _apply_complex_shape(mesh)
                kind = "complex(shape)"
        elif mode == "box":
            _apply_box_outline(mesh)
            kind = "box"
        elif mode == "complex":
            _apply_complex_shape(mesh)
            kind = "complex"
        elif mode == "convex":
            _apply_convex(mesh)
            kind = "convex"
        elif mode == "dop26":
            _apply_dop26(mesh)
            kind = "dop26"
        else:
            unreal.log_warning(f"[FixCollision] Unknown mode '{mode}', using auto")
            return _apply_for_mode(mesh, "auto")

        unreal.EditorAssetLibrary.save_loaded_asset(mesh, only_if_is_dirty=False)
        unreal.log(f"[FixCollision] {kind}: {mesh.get_name()}")
        return "changed"
    except Exception as exc:  # noqa: BLE001
        unreal.log_error(f"[FixCollision] Failed {mesh.get_path_name()}: {exc}")
        return "failed"


def _unique_static_meshes(meshes) -> list:
    seen = set()
    result = []
    for mesh in meshes:
        if not mesh:
            continue
        path = mesh.get_path_name()
        if path in seen:
            continue
        seen.add(path)
        result.append(mesh)
    return result


def _collect_level_meshes() -> list:
    actors = unreal.EditorLevelLibrary.get_all_level_actors()
    meshes = []
    for actor in actors:
        try:
            components = actor.get_components_by_class(unreal.StaticMeshComponent)
        except Exception:  # noqa: BLE001
            components = []
        for comp in components or []:
            mesh = comp.get_editor_property("static_mesh")
            if mesh:
                meshes.append(mesh)
    return meshes


def _process_meshes(meshes, label: str, mode: str) -> None:
    meshes = _unique_static_meshes(meshes)
    total = len(meshes)
    unreal.log(f"[FixCollision] {label}: {total} mesh(es), mode={mode}")

    changed = failed = fence_count = complex_count = 0
    with unreal.ScopedSlowTask(total, f"Collision ({label})") as slow:
        slow.make_dialog(True)
        for index, mesh in enumerate(meshes):
            if slow.should_cancel():
                unreal.log_warning("[FixCollision] Cancelled")
                break
            slow.enter_progress_frame(1, f"{index + 1}/{total}: {mesh.get_name()}")

            if mode == "auto" and is_fence_like(mesh):
                fence_count += 1
            elif mode in ("auto", "complex"):
                complex_count += 1

            status = _apply_for_mode(mesh, mode)
            if status == "changed":
                changed += 1
            else:
                failed += 1

    msg = (
        f"{label}\nMode: {mode}\n\n"
        f"Changed: {changed}\nFailed: {failed}\n"
        f"Fence-like (box): ~{fence_count}\n"
        f"Shape-preserving: ~{complex_count}\n\n"
        "Each mesh keeps its own collision (no merging).\n"
        "Ctrl+Shift+S to Save All, then check Player Collision."
    )
    unreal.log(msg.replace("\n", " | "))
    unreal.EditorDialog.show_message("Smart Collision", msg, unreal.AppMsgType.OK)


def fix_selected(mode: str = "auto") -> None:
    assets = unreal.EditorUtilityLibrary.get_selected_assets()
    meshes = [a for a in assets if isinstance(a, unreal.StaticMesh)]
    if not meshes:
        unreal.EditorDialog.show_message(
            "Smart Collision",
            "Select Static Mesh asset(s) in Content Browser first.",
            unreal.AppMsgType.OK,
        )
        return
    _process_meshes(meshes, "Selected assets", mode)


def fix_folder(folder_path: str = "/Game/SteamRebuild", mode: str = "auto") -> None:
    if not folder_path.startswith("/Game"):
        unreal.log_error("[FixCollision] folder_path must start with /Game")
        return
    asset_paths = unreal.EditorAssetLibrary.list_assets(folder_path, recursive=True, include_folder=False)
    meshes = []
    for path in asset_paths:
        asset = unreal.EditorAssetLibrary.load_asset(path)
        if isinstance(asset, unreal.StaticMesh):
            meshes.append(asset)
    if not meshes:
        unreal.EditorDialog.show_message(
            "Smart Collision",
            f"No Static Mesh under:\n{folder_path}",
            unreal.AppMsgType.OK,
        )
        return
    _process_meshes(meshes, f"Folder {folder_path}", mode)


def fix_current_level(mode: str = "auto") -> None:
    meshes = _collect_level_meshes()
    if not meshes:
        unreal.EditorDialog.show_message(
            "Smart Collision",
            "No Static Meshes found in the current level.",
            unreal.AppMsgType.OK,
        )
        return
    _process_meshes(meshes, "Current level", mode)


def force_selected_box() -> None:
    """Force currently selected Content Browser meshes to a single box."""
    fix_selected(mode="box")


def force_selected_complex() -> None:
    """Force currently selected Content Browser meshes to complex shape collision."""
    fix_selected(mode="complex")


if __name__ == "__main__":
    fix_current_level(mode="auto")
