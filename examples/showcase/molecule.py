"""
# Molecule

"""

import glob
from pathlib import Path

import numpy as np

import datoviz as dvz

ROOT_DIR = Path(__file__).resolve().parent.parent.parent


def load_data():
    # comes from: https://www.rcsb.org/structure/6QZP
    import MDAnalysis as mda

    files = sorted(glob.glob(ROOT_DIR / 'data/misc/molecule/6qzp-pdb-bundle*.pdb'))
    universes = [mda.Universe(f) for f in files]

    positions = np.concatenate([u.atoms.positions for u in universes], axis=0)

    element_colors_u8 = {
        'H': (255, 255, 255),
        'C': (200, 200, 200),
        'N': (143, 143, 255),
        'O': (240, 0, 0),
        'S': (255, 200, 50),
        'P': (255, 165, 0),
        'Mg': (42, 128, 42),
        'Zn': (165, 42, 42),
    }

    colors = np.concatenate(
        [
            np.array([element_colors_u8[atom.element] for atom in u.atoms], dtype=np.uint8)
            for u in universes
        ],
        axis=0,
    )

    colors = np.concatenate([colors, np.full((len(colors), 1), 255, dtype=np.uint8)], axis=1)

    vdw_radii = {
        'H': 1.20,
        'C': 1.70,
        'N': 1.55,
        'O': 1.52,
        'S': 1.80,
        'P': 1.80,
        'Mg': 1.73,
        'Zn': 1.39,
    }

    atomic_radii = np.concatenate(
        [np.array([vdw_radii[atom.element] for atom in u.atoms]) for u in universes]
    )

    sizes = 5 + 10 * (atomic_radii - atomic_radii.min()) / (
        atomic_radii.max() - atomic_radii.min()
    )
    sizes = sizes.astype(np.float32)

    positions -= positions.mean(axis=0)
    positions /= np.max(np.linalg.norm(positions, axis=1))

    return positions, colors, sizes


data = np.load(ROOT_DIR / 'data/misc/molecule/mol.npz')
positions = data['positions']
colors = data['colors']
sizes = data['sizes']
N = len(positions)
print(f'Loaded {N} atoms')


app = dvz.App()
figure = app.figure()
panel = figure.panel()
arcball = panel.arcball()
camera = panel.camera()

visual = app.sphere(
    position=positions,
    color=colors,
    size=sizes,
    light_pos=(-5, +5, +100),
    light_params=(0.4, 0.8, 2, 32),
)
panel.add(visual)

app.run()
app.destroy()
