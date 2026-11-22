"""Typeset the course's short LaTeX equations as self-contained SVG paths."""

import subprocess
import xml.etree.ElementTree as ET
from functools import cache
from pathlib import Path
from tempfile import TemporaryDirectory

from course_figure_svg import SVG

NS = 'http://www.w3.org/2000/svg'
XLINK = 'http://www.w3.org/1999/xlink'
ET.register_namespace('', NS)
ET.register_namespace('xlink', XLINK)


@cache
def _typeset(source):
    """Run LaTeX and dvisvgm on trusted, authored expressions in a temporary directory."""
    tex = (
        r'\documentclass[12pt,border=0pt]{standalone}'
        '\n'
        r'\usepackage{amsmath,amssymb}'
        '\n'
        r'\begin{document}$\displaystyle ' + source + r'$\end{document}'
        '\n'
    )
    with TemporaryDirectory(prefix='course-equation-') as folder:
        root = Path(folder)
        (root / 'equation.tex').write_text(tex)
        for command in [
            [
                'latex',
                '-no-shell-escape',
                '-halt-on-error',
                '-interaction=nonstopmode',
                'equation.tex',
            ],
            [
                'dvisvgm',
                '--no-fonts',
                '--exact',
                '--bbox=min',
                '--output=equation.svg',
                'equation.dvi',
            ],
        ]:
            # Tool arguments and equations are authored constants; no shell is used.
            result = subprocess.run(  # noqa: S603
                command, cwd=root, capture_output=True, text=True, timeout=30
            )
            if result.returncode:
                raise RuntimeError(f'{command[0]} failed: {result.stdout}\n{result.stderr}')
        return (root / 'equation.svg').read_text()


def equation_fragment(name, source, x, y, width, height):
    """Fit a typeset expression into a panel, with unique glyph IDs and themed ink."""
    root = ET.fromstring(_typeset(source))  # noqa: S314 - trusted output from local dvisvgm.
    ids = {e.get('id'): name + '-' + e.get('id') for e in root.iter() if e.get('id')}
    for element in root.iter():
        for key, value in list(element.attrib.items()):
            if key == 'id':
                element.set(key, ids[value])
            elif value.startswith('#') and value[1:] in ids:
                element.set(key, '#' + ids[value[1:]])
    _, _, natural_width, natural_height = map(float, root.get('viewBox').split())
    # Keep a common type size: a fraction must not shrink its letters relative to a short equation.
    scale = min(2.0, width / natural_width, height / natural_height)
    fitted_width, fitted_height = natural_width * scale, natural_height * scale
    root.set('x', f'{x + (width - fitted_width) / 2:.8g}')
    root.set('y', f'{y + (height - fitted_height) / 2:.8g}')
    root.set('width', f'{fitted_width:.8g}')
    root.set('height', f'{fitted_height:.8g}')
    root.set('fill', 'var(--md-default-fg-color, #253247)')
    root.set('preserveAspectRatio', 'xMidYMid meet')
    root.set('aria-hidden', 'true')
    return ET.tostring(root, encoding='unicode')


EQUATIONS = {
    'equation-mvp': (
        r'\mathbf{p}_{\mathrm{clip}}=P\,V\,M\,\mathbf{p}_{\mathrm{object}}',
        'Clip position equals projection times view times model times object position.',
    ),
    'equation-diffuse': (
        r'd=\max\!\left(0,\,\mathbf{n}\cdot\mathbf{l}\right)',
        'Diffuse is the maximum of zero and the dot product of unit normal and light directions.',
    ),
}


def figures():
    """Return the standalone equation panels included alongside the chapter prose."""
    result = {}
    for name, (tex, description) in EQUATIONS.items():
        svg = SVG(name, 'Equation', description, width=440, height=70)
        svg.raw(equation_fragment(name + '-math', tex, 16, 14, 408, 42))
        result[name] = svg.finish().replace('class="dvz-schematic"', 'class="dvz-course-math"', 1)
    return result
