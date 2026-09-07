"""Editor entry for materials, lighting and World Partition. Requires Unreal Editor."""
from __future__ import annotations
import json
import os
from pathlib import Path
import editor_lighting
import editor_materials
import editor_world_partition

report = {
    'ok': False,
    'warnings': [],
    'shader_compile': 'NOT_VERIFIED',
    'visual_verified': False,
}
try:
    editor_materials.create_master_materials(report)
    editor_lighting.apply_day_night(report, mode=os.environ.get('MAI_LIGHTING_MODE', 'day'))
    editor_world_partition.configure_streaming(report)
    report['ok'] = True
except Exception as error:
    report['error'] = str(error)
    report['ok'] = False
path = Path(os.environ['MAI_SCRIPT_REPORT'])
path.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
if not report['ok']:
    raise SystemExit(str(report.get('error', 'Editor visual script failed')))
