# Reproducible build

Blender 4.5.3 LTS, Python 3.11+ with NumPy and Pillow for the material-map and catalog scripts.

The four source builders in `source/` are unmodified snapshots from the repository commit recorded in the manifest. `build_all.py` executes only their modelling sections, replacing small-scale primitive helpers and explicitly excluding all original write/export tails. No original repository file is overwritten.

Run from the unpacked package directory:

```
python build/make_maps.py
blender -b -t 8 --python build/build_all.py -- vehicles
blender -b -t 8 --python build/build_all.py -- characters
blender -b -t 8 --python build/build_all.py -- racks
blender -b -t 8 --python build/build_all.py -- interiors
blender -b -t 8 --python build/polish_exports.py
blender -b -t 4 --python build/validate.py
blender -b -t 4 --python build/validate_enclosures.py
python build/make_catalog.py
```

Each category command also accepts one optional asset ID. `polish_exports.py` accepts a list of IDs after `--`. The final polish step removes degenerate faces, circularizes the vehicle wheels after the source proportion changes, refreshes the FBX/GLB files and makes the vehicle glazing and studio lighting suitable for review.

Do not run two writers for the same asset at the same time. Set your Blender executable path according to your installation. Rebuilding writes the generated asset folders in this package. Source files are untouched.
