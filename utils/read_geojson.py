import json
from pathlib import Path
import sys
import numpy as np

json_path = Path(sys.argv[1])


with open(json_path, 'r') as f:
    data = json.load(f)

polygons = []

for p in data['features']:
    if "Polygon" not in p["geometry"]["type"]:
        continue
    for coords in p["geometry"]["coordinates"]:
        pos = np.array(coords, dtype=np.float64).squeeze()
        assert pos.ndim == 2
        polygons.append(pos)

point_path = json_path.with_suffix(".polypoints.bin")
segment_path = json_path.with_suffix(".polylengths.bin")
np.vstack(polygons).astype(np.float64).ravel().tofile(point_path)
# print(np.array([p.shape[0] for p in polygons]).astype(np.uint32))
np.array([p.shape[0] for p in polygons]).astype(np.uint32).tofile(segment_path)
print(f"{point_path} and {segment_path} successfully created.")
