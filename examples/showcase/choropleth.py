"""
# Choropleth example

Show a choropleth map.

---
tags:
  - mesh
  - shape
  - colormap
  - polygon
  - ortho
---

"""

from pathlib import Path

import geopandas as gpd
import numpy as np
import pandas as pd
from matplotlib import cm
from matplotlib.colors import Normalize
from shapely.geometry import Polygon

import datoviz as dvz

# -------------------------------------------------------------------------------------------------
# Constants
# -------------------------------------------------------------------------------------------------

CURDIR = Path(__file__).resolve().parent.parent.parent
HAS_CONTOUR = False


# -------------------------------------------------------------------------------------------------
# Get the data
# -------------------------------------------------------------------------------------------------

# Load US counties from US Census (simplified resolution)
# counties_url = "https://www2.census.gov/geo/tiger/GENZ2021/shp/cb_2021_us_county_5m.zip"
counties_url = CURDIR / 'data/misc/cb_2021_us_county_5m.zip'
counties = gpd.read_file(counties_url)

# Create a FIPS code for merging
counties['fips'] = counties['STATEFP'] + counties['COUNTYFP']

# Load unemployment data with FIPS code
unemp_url = 'https://raw.githubusercontent.com/plotly/datasets/master/fips-unemp-16.csv'
unemp = pd.read_csv(unemp_url, dtype={'fips': str})
unemp.columns = ['fips', 'rate']

# Merge to get scalar value per county
merged = counties.merge(unemp, on='fips')

# Project to a cartesian coordinate system (Albers Equal Area)
merged = merged.to_crs('EPSG:5070')

xmin, xmax = merged.bounds.minx.min(), merged.bounds.minx.max()
ymin, ymax = merged.bounds.miny.min(), merged.bounds.miny.max()
amin = np.array([[xmin, ymin]])
amax = np.array([[xmax, ymax]])

# Normalize unemployment rate to [0, 1] for colormap
field = 'rate'
merged[field] = np.log(merged[field])
norm = Normalize(vmin=merged[field].min(), vmax=merged[field].max())
cmap = cm.get_cmap('viridis')


# -------------------------------------------------------------------------------------------------
# Datoviz
# -------------------------------------------------------------------------------------------------

w, h = 800, 600
app = dvz.App()
figure = app.figure(w, h)
panel = figure.panel()
ortho = panel.ortho()

sc = dvz.ShapeCollection()
for _, row in merged.iterrows():
    geometry = row['geometry']
    color = np.array(cmap(norm(row[field])))
    color = tuple(dvz.to_byte(color, 0, 1))

    # Handle Polygon and MultiPolygon
    polygons = [geometry] if isinstance(geometry, Polygon) else list(geometry.geoms)
    for poly in polygons:
        coords = np.asarray(poly.exterior.coords, dtype=np.float64)  # shape (N, 2)
        points = -1 + 2 * (coords - amin) / (amax - amin)
        points[:, 1] *= h / float(w)
        sc.add_polygon(points, color=color, contour='joints' if HAS_CONTOUR else None)

visual = app.mesh_shape(sc, linewidth=0.5, edgecolor=(0, 0, 0, 128))
panel.add(visual)

app.run()
app.destroy()
sc.destroy()
