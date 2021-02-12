"""
# Molecule 3D mesh

"""

import numpy as np
import numpy.random as nr
from datoviz import canvas, run, colormap

c = canvas(show_fps=False)
panel = c.panel(controller='arcball')
visual = panel.visual('mesh')
visual.load_obj("data/mesh/cas9_guide.obj")

run()
