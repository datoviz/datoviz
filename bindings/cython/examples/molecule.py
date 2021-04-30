"""
# Molecule 3D mesh

"""

from pathlib import Path
import numpy as np
import numpy.random as nr
from datoviz import canvas, run, colormap

ROOT = Path(__file__).resolve().parent.parent.parent.parent

c = canvas(show_fps=False)
panel = c.scene().panel(controller='arcball')
visual = panel.visual('mesh')
visual.load_obj(str(ROOT / "data/mesh/cas9_guide.obj"))

run()
