"""
Showing a ultra-high resolution mesh of a human brain, acquired with a 7 Tesla MRI.

The data is not yet publicly available.

Data courtesy of Anneke Alkemade et al.:

    Alkemade A, Pine K, Kirilina E, Keuken MC, Mulder MJ, Balesar R, Groot JM, Bleys RLAW,
    Trampel R, Weiskopf N, Herrler A, Möller HE, Bazin P-L and Forstmann BU (2020)

    "7 Tesla MRI Followed by Histological 3D Reconstructions in Whole-Brain Specimens"

    Front. Neuroanat. 14:536838
    doi: 10.3389/fnana.2020.536838

Acknowledgements to Pierre-Louis Bazin and Julia Huntenburg for data access.

"""

from pathlib import Path
import numpy as np

from datoviz import canvas, run, colormap

c = canvas(show_fps=True, width=1024, height=768)
panel = c.panel(controller='arcball')
visual = panel.visual('mesh', transform='auto')

ROOT = Path(__file__).parent.parent.parent.parent
pos = np.load(ROOT / "data/mesh/brain_highres.vert.npy")
faces = np.load(ROOT / "data/mesh/brain_highres.faces.npy")

visual.data('pos', pos)
visual.data('index', faces.ravel())
visual.data('clip', np.array([0, 0, 1, 1]))

gui = c.gui("GUI")
@gui.control("slider_float", "clip", vmin=-1, vmax=+1, value=+1)
def on_change(value):
    visual.data('clip', np.array([0, 0, 1, value]))

run()
