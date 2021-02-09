from pathlib import Path

import numpy as np
import numpy.random as nr

from datoviz import canvas, run, colormap


c = canvas(show_fps=True)
panel = c.panel(controller='arcball')
visual = panel.visual('volume')

texshape = np.array([320, 456, 528])
coef = .004
shape = coef * texshape.astype(np.float32)

visual.data('pos', np.atleast_2d(-shape / 2), idx=0)
visual.data('pos', np.atleast_2d(+shape / 2), idx=1)

visual.data('texcoords', np.atleast_2d([0, 0, 0]), idx=0)
visual.data('texcoords', np.atleast_2d([1, 1, 1]), idx=1)

visual.data('length', np.atleast_2d(shape).astype(np.float32))

# HACK
visual.data('colormap', np.atleast_2d(np.array([0], dtype=np.float32)))

ROOT = Path(__file__).parent.parent.parent.parent
vol = np.fromfile(ROOT / "data/volume/atlas_25.img", dtype=np.uint16)
vol = vol.reshape(texshape)
visual.volume(vol)

run()
