"""Blender-only derived skeletal LOD preparation; original FBX and Blend files stay unchanged.
Usage: blender --background --python this_file -- --repo-root PATH
Outputs are drafts requiring skinning, silhouette and UE import inspection.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import sys


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo-root', type=Path, required=True)
    args = parser.parse_args(argv)
    repo = args.repo_root.resolve()
    source = repo / 'Unreal/Assets/characters/person-1/person-1.fbx'
    original = hashlib.sha256(source.read_bytes()).hexdigest()
    output = repo / 'Unreal/MakeYourAI/Saved/ScaffoldSource/person-1-lods'
    output.mkdir(parents=True, exist_ok=False)
    import bpy
    results = []
    for lod, target in enumerate((7500, 2000, 500)):
        bpy.ops.wm.read_factory_settings(use_empty=True)
        bpy.ops.import_scene.fbx(filepath=str(source))
        rigs = [obj for obj in bpy.context.scene.objects if obj.type == 'ARMATURE']
        meshes = [obj for obj in bpy.context.scene.objects if obj.type == 'MESH' and not obj.name.startswith('UCX_')]
        if len(rigs) != 1 or not meshes:
            raise RuntimeError('Expected one skeleton and its source meshes')
        rigs[0].data.pose_position = 'REST'
        for obj in meshes:
            obj.data.calc_loop_triangles()
        before = sum(len(obj.data.loop_triangles) for obj in meshes)
        ratio = min(1.0, target / max(1, before))
        for obj in meshes:
            bpy.ops.object.select_all(action='DESELECT')
            obj.select_set(True)
            bpy.context.view_layer.objects.active = obj
            modifier = obj.modifiers.new('Scaffold_LOD', 'DECIMATE')
            modifier.decimate_type = 'COLLAPSE'
            modifier.ratio = ratio
            modifier.use_collapse_triangulate = True
            bpy.ops.object.modifier_apply(modifier=modifier.name)
            # Normalize surviving deform weights without changing bone names.
            bones = {bone.name for bone in rigs[0].data.bones}
            indices = {group.index for group in obj.vertex_groups if group.name in bones}
            for vertex in obj.data.vertices:
                weights = [(group.group, group.weight) for group in vertex.groups if group.group in indices]
                total = sum(weight for _, weight in weights)
                if total > 0:
                    for index, weight in weights:
                        obj.vertex_groups[index].add([vertex.index], weight / total, 'REPLACE')
            obj.data.calc_loop_triangles()
        actual = sum(len(obj.data.loop_triangles) for obj in meshes)
        bpy.ops.object.select_all(action='DESELECT')
        for obj in meshes + rigs:
            obj.select_set(True)
        target_file = output / f'person-1-LOD{lod}.fbx'
        bpy.ops.export_scene.fbx(filepath=str(target_file), use_selection=True,
                                 object_types={'ARMATURE', 'MESH'}, add_leaf_bones=False,
                                 bake_anim=False, apply_unit_scale=True,
                                 axis_forward='-Z', axis_up='Y')
        results.append({'lod': lod, 'target_triangles': target, 'blender_triangles': actual,
                        'file': str(target_file.relative_to(repo)), 'bytes': target_file.stat().st_size,
                        'bones': len(rigs[0].data.bones), 'ue_import_verified': False})
    if hashlib.sha256(source.read_bytes()).hexdigest() != original:
        raise RuntimeError('Original FBX unexpectedly changed')
    report = {'schema': 1, 'source_sha256': original, 'source_unchanged': True,
              'blender_version': bpy.app.version_string, 'levels': results,
              'visual_verified': False, 'ue_lod_triangles': [None, None, None],
              'warning': 'Derived decimation drafts only. Verify weights, silhouette, orientation, screen sizes and exact imported triangle counts in UE.'}
    (output / 'lod-report.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(report, indent=2))
    return 0


if __name__ == '__main__':
    args = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []
    raise SystemExit(main(args))
