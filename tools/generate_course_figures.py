#!/usr/bin/env python3
"""
Regenerate the course's vector schematics, or verify them with --check.

The SVGs are authored teaching illustrations, not Vulkan captures. Their panels are included inline
by Markdown snippets so they inherit the documentation theme and stack on small screens.
"""

from __future__ import annotations

import argparse
import json
import xml.etree.ElementTree as ET
from pathlib import Path

from course_figure_geometry import figures as geometry_figures
from course_figure_math import EQUATIONS
from course_figure_math import figures as equation_figures
from course_figure_surfaces import figures as surface_figures

ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / 'docs/images/gpu-graphics'
FIGURES = [
    dict(
        id='fig-vertex-byte-layout',
        chapter='06-vertex-buffers',
        number=2,
        title='Vertex bytes and shader inputs',
        panels=['vertex-bytes', 'vertex-fetch'],
        caption='Each 20-byte vertex holds two position floats followed by three '
        'color floats. Offsets locate fields within a record; the stride '
        'reaches the next record.',
        purpose='See the bytes behind stride, attribute offsets, formats and shader locations.',
        limits='Rows depict consecutive records in one continuous buffer. This '
        'ordinary vertex format has no std140 padding.',
    ),
    dict(
        id='fig-indexed-square',
        chapter='07-index-buffers',
        number=1,
        title='Four vertices, two triangles',
        panels=['indexed-square', 'index-stream'],
        caption='Two triples of indices form two triangles. Both triples refer to '
        'the same complete vertex records at corners 0 and 2.',
        purpose='See how index reuse connects triangles without duplicating vertex records.',
        limits='The byte comparison counts uploaded payload only, excluding Vulkan '
        'object and allocation overhead.',
    ),
    dict(
        id='fig-model-view-transform',
        chapter='09-matrices',
        number=2,
        title='Object, world and view coordinates',
        panels=['cube-object', 'cube-world', 'camera-view'],
        caption='The model changes the cube’s pose. The view expresses that same '
        'scene relative to the camera.',
        purpose='Distinguish changing an object’s pose from expressing the scene '
        'relative to a camera.',
        limits='At time zero the model tilts the cube. The side view shows how the camera '
        'becomes the coordinate origin while the cube stays in front of it.',
    ),
    dict(
        id='fig-perspective-divide',
        chapter='09-matrices',
        number=3,
        title='Perspective and division by w',
        panels=['perspective-rays', 'perspective-divide'],
        caption='Equal sizes appear different at different distances: doubling the '
        'distance doubles w and halves the size after division.',
        purpose='Connect the camera rays, the projection matrix and the GPU’s perspective divide.',
        limits='The rays compare apparent size. The equations show why division by distance '
        'makes a farther object smaller; the virtual plane is not a clipping plane.',
    ),
    dict(
        id='fig-face-uv-map',
        chapter='13-texture-upload',
        number=1,
        title='A face’s UV coordinates',
        panels=['face-uv-map'],
        caption='Each face has its own UV square. The rasterizer interpolates these '
        'coordinates across its two triangles.',
        purpose='See how two triangles cover a full face with continuous UV coordinates.',
        limits='This is a UV-coordinate map, not screen orientation. The diagnostic '
        'color is red=u, green=v, blue=0.',
    ),
    dict(
        id='fig-nearest-repeat',
        chapter='14-texture-sampling',
        number=2,
        title='Nearest filtering and repeat addressing',
        panels=['nearest-texel', 'repeat-addressing'],
        caption='Nearest filtering selects one texel. Repeat addressing wraps UVs '
        'outside the unit square.',
        purpose='Separate which texel is selected from how coordinates outside the '
        'texture are handled.',
        limits='Magnified coordinate schematics of the canonical 64-by-64 '
        'checkerboard, not new rendered output.',
    ),
    dict(
        id='fig-lighting-vectors',
        chapter='15-lighting',
        number=1,
        title='Directions at a surface point',
        panels=['light-positions', 'light-directions'],
        caption='Lighting compares unit directions at one surface point, all in view '
        'space. The halfway direction lies between the light and eye '
        'directions.',
        purpose='Make the normal, light, eye and halfway vectors concrete before '
        'reading their GLSL expressions.',
        limits='Illustrative surface point (0,0,-3), not a selected cube fragment. '
        'The normal and eye directions coincide in this example.',
    ),
    dict(
        id='fig-lighting-components',
        chapter='15-lighting',
        number=2,
        title='Ambient, diffuse, specular and their sum',
        panels=['lighting-ambient', 'lighting-diffuse', 'lighting-specular', 'lighting-combined'],
        caption='Ambient and diffuse scale the surface color; white specular is '
        'added on top. All four panels use the same scale.',
        purpose='See what each lighting term contributes and how the terms combine.',
        limits='An illustrative sphere with constant linear albedo, evaluated with '
        'this chapter’s shader formula and coefficients. It is not the '
        'course cube or a Vulkan capture. No per-panel normalization.',
    ),
]


def main():
    """Generate or verify the inventory of accessible course figure panels."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        '--check',
        action='store_true',
        help='fail if committed SVGs differ from regenerated results',
    )
    parser.add_argument(
        '--manifest',
        type=Path,
        help='write a review inventory outside the published documentation',
    )
    args = parser.parse_args()
    assets = {**geometry_figures(), **surface_figures(), **equation_figures()}
    expected = [panel for f in FIGURES for panel in f['panels']] + list(EQUATIONS)
    if len(expected) != len(set(expected)) or set(expected) != set(assets):
        raise ValueError('Figure inventory and generated panels disagree')
    ids = set()
    for name, svg in assets.items():
        # The source is trusted SVG generated above, not an external XML document.
        root = ET.fromstring(svg)  # noqa: S314
        assert root.get('role') == 'img' and root.get('aria-labelledby'), name
        assert root.tag == '{http://www.w3.org/2000/svg}svg', name
        for element in root.iter():
            ident = element.get('id')
            if ident:
                if ident in ids:
                    raise ValueError(f'Duplicate SVG id {ident}')
                ids.add(ident)
            if element.tag.endswith('script') or any(
                k.lower().startswith('on') for k in element.attrib
            ):
                raise ValueError(f'Unexpected active SVG content in {name}')
        dest = OUTPUT / (name + '.svg')
        if args.check:
            if not dest.is_file() or dest.read_text() != svg:
                raise SystemExit(f'Stale figure: {dest.relative_to(ROOT)}')
        else:
            OUTPUT.mkdir(parents=True, exist_ok=True)
            dest.write_text(svg)
    if args.manifest:
        args.manifest.parent.mkdir(parents=True, exist_ok=True)
        args.manifest.write_text(
            json.dumps(
                {
                    'figures': FIGURES,
                    'panel_count': len(assets) - len(EQUATIONS),
                    'equation_count': len(EQUATIONS),
                },
                indent=2,
                ensure_ascii=False,
            )
            + '\n'
        )
    print(
        f'Course figures: {len(FIGURES)} figures, {len(assets) - len(EQUATIONS)} SVG panels, '
        f'{len(EQUATIONS)} equations ' + ('verified' if args.check else 'generated')
    )


if __name__ == '__main__':
    main()
