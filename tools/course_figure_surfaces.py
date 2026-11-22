"""
Coordinate and lighting schematics for chapters 13–15.

The lighting panels evaluate the chapter's arithmetic on an illustrative sphere.
They are mathematical illustrations, not captures from the course program.
"""

from collections import defaultdict
from math import sqrt

from course_figure_svg import SVG


def _uv_face():
    svg = SVG(
        'face-uv-map',
        'UV coordinates on one cube face',
        'A UV square with u increasing right and v increasing down. Local corners '
        '0, 1, 2, 3 have coordinates (0,0), (1,0), (1,1), (0,1). The diagonal '
        'from 0 to 2 splits it into triangles 0,1,2 and 0,2,3. This is a UV layout, '
        'not the orientation of the rendered face.',
        height=370,
    )
    svg.text(16, 26, 'One face, two triangles', size=17, bold=True)
    # The diagnostic shader returns (u, v, 0) in linear space. The SVG's additive
    # blend reproduces this coordinate gradient without adding a raster image.
    svg.raw(
        '<defs><linearGradient id="face-uv-red" '
        'color-interpolation="linearRGB"><stop stop-color="#000"/>'
        '<stop offset="1" stop-color="#f00"/></linearGradient>'
        '<linearGradient id="face-uv-green" x2="0" y2="1" color-interpolation="linearRGB">'
        '<stop stop-color="#000"/><stop offset="1" stop-color="#0f0"/>'
        '</linearGradient></defs>'
    )
    svg.rect(64, 88, 200, 200, 'url(#face-uv-red)', stroke='none')
    svg.raw(
        '<rect x="64" y="88" width="200" height="200" '
        'fill="url(#face-uv-green)" style="mix-blend-mode:screen"/>'
    )
    svg.rect(64, 88, 200, 200, 'none', stroke='ink')
    svg.line(64, 88, 264, 288, '#fff', width=2, dash=True)
    svg.text(209, 157, '0, 1, 2', fill='#182333', anchor='middle', bold=True)
    svg.text(122, 243, '0, 2, 3', fill='#182333', anchor='middle', bold=True)
    svg.text(58, 76, '0: (0, 0)')
    svg.text(263, 76, '1: (1, 0)', anchor='end')
    svg.text(58, 313, '3: (0, 1)')
    svg.text(263, 313, '2: (1, 1)', anchor='end')
    svg.arrow(64, 47, 264, 47, 'blue')
    svg.text(277, 52, 'u', fill='blue', bold=True)
    svg.arrow(34, 88, 34, 288, 'orange')
    svg.text(24, 192, 'v', fill='orange', anchor='end', bold=True)
    svg.text(160, 344, 'UV layout, not screen orientation', anchor='middle')
    return svg.finish()


def _checker(x, y):
    if x < 8 and y < 8:
        return '#f5f5f5'
    return '#ebaa37' if ((x // 8 + y // 8) & 1) else '#2350be'


def _nearest():
    svg = SVG(
        'nearest-texel',
        'Nearest filtering chooses one texel',
        'A magnified four-by-four region of the chapter’s 64-by-64 texture. '
        'Columns and rows run from 6 through 9. A sample at texel coordinates '
        '(7.75,7.25), or UV (7.75/64,7.25/64), selects column 7, row 7. '
        'The cross is the sample position and the circle is the selected texel center. '
        'Nearest filtering does not blend adjacent texels.',
        height=365,
    )
    svg.text(16, 26, 'Nearest: one texel, no blend', size=17, bold=True)
    svg.text(152, 51, 'column →', anchor='middle')
    svg.text(16, 73, 'row')
    for row in range(4):
        for col in range(4):
            svg.rect(64 + col * 44, 84 + row * 44, 44, 44, _checker(col + 6, row + 6), 'paper')
        svg.text(43, 111 + row * 44, str(row + 6), anchor='end')
        svg.text(86 + row * 44, 73, str(row + 6), anchor='middle')
    # The chosen cell is column 7, row 7; its center is half a texel from its edges.
    svg.rect(109, 129, 42, 42, 'none', '#101820')
    cx, cy = 64 + 1.5 * 44, 84 + 1.5 * 44
    sx, sy = 64 + 1.75 * 44, 84 + 1.25 * 44
    svg.line(cx, cy, sx, sy, '#182333', dash=True)
    svg.circle(cx, cy, 4, 'none', '#182333')
    svg.line(sx - 5, sy - 5, sx + 5, sy + 5, '#a51c30', width=2.5)
    svg.line(sx - 5, sy + 5, sx + 5, sy - 5, '#a51c30', width=2.5)
    svg.text(16, 291, '× sample', fill='red', bold=True)
    svg.text(134, 291, '○ chosen center')
    svg.text(16, 320, 'UV = (7.75/64, 7.25/64)')
    svg.text(16, 344, 'Selected texel: column 7, row 7')
    return svg.finish()


def _repeat():
    svg = SVG(
        'repeat-addressing',
        'Repeat addressing wraps texture coordinates',
        'Two adjacent UV unit squares show the same eight-by-eight tile pattern '
        'from the chapter’s 64-by-64 texture. Each has the white orientation tile '
        'in its upper-left corner. Samples at UV (0.30,0.30) and (1.30,0.30) '
        'both read blue texel column 19, row 19. u increases right and v down.',
        height=365,
    )
    svg.text(16, 26, 'Repeat: wrap the coordinates', size=17, bold=True)
    svg.text(160, 56, '1.30 wraps to 0.30', anchor='middle')
    start, top, side = 38, 105, 128
    for unit in range(2):
        for row in range(8):
            for col in range(8):
                svg.rect(
                    start + unit * side + col * 16,
                    top + row * 16,
                    16,
                    16,
                    _checker(col * 8, row * 8),
                    'none',
                )
        svg.rect(start + unit * side, top, side, side, 'none', 'ink')
    first = start + 0.30 * side
    second = start + 1.30 * side
    sample_y = top + 0.30 * side
    svg.arrow(second, 76, first, 76, 'red')
    for x in (first, second):
        svg.line(x, 76, x, sample_y, 'red', dash=True)
        svg.circle(x, sample_y, 5, 'none', '#fff')
        svg.line(x - 8, sample_y, x + 8, sample_y, '#fff', width=2)
        svg.line(x, sample_y - 8, x, sample_y + 8, '#fff', width=2)
    for x, label in ((start, '0'), (start + side, '1'), (start + 2 * side, '2')):
        svg.text(x, 256, label, anchor='middle')
    svg.arrow(start, 274, start + 2 * side, 274, 'blue')
    svg.text(306, 279, 'u', fill='blue', anchor='end', bold=True)
    svg.arrow(20, top, 20, top + side, 'orange')
    svg.text(12, 174, 'v', fill='orange', anchor='end', bold=True)
    svg.text(16, 316, 'Both samples read texel (19, 19).')
    svg.text(16, 342, 'The image is repeated, not enlarged.')
    return svg.finish()


def _light_positions():
    svg = SVG(
        'light-positions',
        'Light, eye and an illustrative surface point in view space',
        'A plane through view-space P=(0,0,-3), eye E=(0,0,0), and light '
        'L=(2,2,0). Horizontal s=(x+y)/sqrt(2) and vertical positive z. '
        'Dashed guides from P point toward the eye and light. These guides '
        'show positions and distances, not unit-vector lengths.',
        height=350,
    )
    svg.text(16, 26, 'Positions in view space', size=17, bold=True)
    svg.line(100, 250, 100, 80, 'green', dash=True)
    svg.line(100, 250, 260, 80, 'orange', dash=True)
    svg.line(70, 250, 130, 250, 'ink', width=4)
    svg.circle(100, 250, 4)
    svg.circle(100, 80, 5, 'green')
    svg.circle(260, 80, 5, 'orange')
    svg.text(100, 58, 'Eye E', fill='green', anchor='middle', bold=True)
    svg.text(100, 105, '(0, 0, 0)', anchor='middle')
    svg.text(260, 58, 'Light L', fill='orange', anchor='middle', bold=True)
    svg.text(260, 105, '(2, 2, 0)', anchor='middle')
    svg.text(100, 277, 'P = (0, 0, −3)', anchor='middle')
    svg.arrow(35, 230, 35, 136, 'line')
    svg.text(35, 126, '+z', anchor='middle')
    svg.arrow(180, 250, 296, 250, 'line')
    svg.text(296, 275, '+s', anchor='end')
    svg.text(160, 311, 's = (x + y) / √2', anchor='middle')
    svg.text(160, 336, 'Illustrative point, not a cube snapshot', anchor='middle')
    return svg.finish()


def _light_directions():
    svg = SVG(
        'light-directions',
        'Four unit directions at one surface point',
        'All unit vectors start at the same point P. Here n=v=(0,0,1), '
        'l=normalize(2,2,3), and h=normalize(l+v). The common normal and eye '
        'arrow points up; the light direction is 43.3 degrees to its right; '
        'the halfway direction is 21.7 degrees to its right. The dot product '
        'n dot l is about 0.728.',
        height=350,
    )
    svg.text(16, 26, 'Unit directions at P', size=17, bold=True)
    x, y, length = 100, 235, 145
    light_s, light_z = sqrt(8 / 17), 3 / sqrt(17)
    half_s, half_z = light_s, light_z + 1
    half_len = sqrt(half_s * half_s + half_z * half_z)
    svg.line(50, y, 150, y, 'ink', width=4)
    svg.arrow(x, y, x, y - length, 'green', width=3)
    svg.arrow(x, y, x + length * light_s, y - length * light_z, 'orange', width=3)
    svg.arrow(
        x, y, x + length * half_s / half_len, y - length * half_z / half_len, 'purple', width=3
    )
    svg.circle(x, y, 4)
    svg.text(88, 77, 'n = v', fill='green', anchor='middle', bold=True)
    svg.text(162, 94, 'h', fill='purple', bold=True)
    svg.text(210, 132, 'l', fill='orange', bold=True)
    svg.text(89, 256, 'P', bold=True)
    # Clockwise arc from the shared normal/eye direction to the light direction.
    radius = 53
    svg.raw(
        f'<path d="M{x},{y - radius} A{radius},{radius} 0 0 1 '
        f'{x + radius * light_s:g},{y - radius * light_z:g}" '
        'fill="none" stroke="#edac61" stroke-width="1.5"/>'
    )
    svg.text(161, 211, '43.3°', fill='orange')
    svg.text(16, 286, 'n and v coincide at this point.')
    svg.text(16, 311, 'h lies halfway between l and v.')
    svg.text(16, 336, 'All arrows are unit length; n · l ≈ 0.728.')
    return svg.finish()


def _normalize(v):
    length = sqrt(sum(c * c for c in v))
    return tuple(c / length for c in v)


def _dot(a, b):
    return sum(x * y for x, y in zip(a, b, strict=True))


def _sphere_samples(resolution=384, *, cover_boundary=False):
    """Evaluate the lighting model on a perspective sphere, including optional edge cells."""
    albedo = (0.55, 0.28, 0.12)  # Linear values; constant to isolate the lighting terms.
    samples = []
    for row in range(resolution):
        for col in range(resolution):
            qx = -0.34 + (col + 0.5) * 0.68 / resolution
            qy = 0.34 - (row + 0.5) * 0.68 / resolution
            direction = _normalize((qx, qy, -1))
            b = 3 * direction[2]
            disc = b * b - (9 - 0.9 * 0.9)
            if disc < 0:
                # A boundary cell can intersect the exact silhouette even if its
                # center misses. Use the tangent color there, then clip the cell
                # to the smooth circle. Interior samples keep their original rays.
                radial = sqrt(qx * qx + qy * qy)
                projected_radius = 0.9 / sqrt(9 - 0.9 * 0.9)
                half_diagonal = sqrt(2) * 0.34 / resolution
                if not cover_boundary or radial > projected_radius + half_diagonal:
                    continue
                factor = projected_radius * (1 - 1e-12) / radial
                direction = _normalize((qx * factor, qy * factor, -1))
                b = 3 * direction[2]
                disc = max(0, b * b - (9 - 0.9 * 0.9))
            t = -b - sqrt(disc)
            p = tuple(t * c for c in direction)
            n = _normalize((p[0], p[1], p[2] + 3))
            light = _normalize((2 - p[0], 2 - p[1], -p[2]))
            eye = _normalize(tuple(-c for c in p))
            halfway = _normalize(tuple(a + b for a, b in zip(light, eye, strict=True)))
            diffuse = max(_dot(n, light), 0)
            specular = max(_dot(n, halfway), 0) ** 32 if diffuse > 0 else 0
            ambient_rgb = tuple(0.16 * a for a in albedo)
            diffuse_rgb = tuple(0.84 * a * diffuse for a in albedo)
            specular_rgb = (0.35 * specular,) * 3
            combined_rgb = tuple(
                a + d + s for a, d, s in zip(ambient_rgb, diffuse_rgb, specular_rgb, strict=True)
            )
            samples.append((row, col, (ambient_rgb, diffuse_rgb, specular_rgb, combined_rgb)))
    return samples


def _srgb_byte(linear):
    linear = max(0, min(1, linear))
    encoded = 12.92 * linear if linear <= 0.0031308 else 1.055 * linear ** (1 / 2.4) - 0.055
    return round(encoded * 255)


def _color_runs(samples, index):
    """Group equal-color horizontal sample runs without changing their RGB values."""
    paths = defaultdict(list)
    run_row, run_start, run_end, run_rgb = -1, 0, 0, None
    for row, col, terms in samples:
        rgb = '#' + ''.join(f'{_srgb_byte(c):02x}' for c in terms[index])
        if row == run_row and col == run_end and rgb == run_rgb:
            run_end += 1
            continue
        if run_rgb is not None:
            length = run_end - run_start
            paths[run_rgb].append(f'M{run_start},{run_row}h{length}v1h-{length}z')
        run_row, run_start, run_end, run_rgb = row, col, col + 1, rgb
    if run_rgb is not None:
        length = run_end - run_start
        paths[run_rgb].append(f'M{run_start},{run_row}h{length}v1h-{length}z')
    return paths


def _lighting_terms():
    resolution = 384
    samples = _sphere_samples(resolution, cover_boundary=True)
    names = ('ambient', 'diffuse', 'specular', 'combined')
    titles = ('Ambient only', 'Diffuse only', 'Specular only', 'All three terms')
    labels = (
        'Constant surface contribution',
        'Depends on the light angle',
        'Highlight toward the eye',
        'Sum in linear light',
    )
    descriptions = (
        'the constant ambient contribution',
        'the diffuse contribution',
        'the white specular highlight',
        'the sum of the three contributions in linear light',
    )
    output = {}
    for index, (name, title, label, description) in enumerate(
        zip(names, titles, labels, descriptions, strict=True)
    ):
        svg = SVG(
            'lighting-' + name,
            title + ' on an illustrative sphere',
            'A mathematical illustration, not a capture of the course cube. '
            'The sphere has radius 0.9, center (0,0,-3), constant linear albedo '
            '(0.55,0.28,0.12), and white tint. Eye (0,0,0), light (2,2,0), '
            'and the chapter’s coefficients 0.16, 0.84, 0.35 and exponent 32 '
            'are shared by all four panels. This panel shows ' + description + '. '
            'All panels use the same exposure, clamping and linear-to-sRGB conversion.',
            height=350,
        )
        svg.text(16, 26, title, size=17, bold=True)
        svg.rect(32, 50, 256, 256, '#111820', 'none')
        # The projected silhouette of this centered perspective sphere is a circle.
        # Clip boundary cells to that circle so its outline stays smooth when scaled.
        radius = 0.9 / sqrt(9 - 0.9 * 0.9) * resolution / 0.68
        clip_id = 'lighting-' + name + '-silhouette'
        svg.raw(
            f'<defs><clipPath id="{clip_id}"><circle cx="{resolution / 2:g}" '
            f'cy="{resolution / 2:g}" r="{radius:.12g}"/></clipPath></defs>'
        )
        svg.raw(
            f'<g transform="translate(32 50) scale({256 / resolution:.16g})" '
            f'clip-path="url(#{clip_id})">'
        )
        # Paths use integer sample coordinates, with adjacent equal-color samples
        # merged into runs. No raster data, interpolation or per-panel rescaling.
        for rgb, cells in sorted(_color_runs(samples, index).items()):
            svg.raw(f'<path fill="{rgb}" shape-rendering="crispEdges" d="{"".join(cells)}"/>')
        svg.raw('</g>')
        svg.text(160, 333, label, anchor='middle')
        output['lighting-' + name] = svg.finish()
    return output


def figures():
    """Return deterministic SVG panels; callers write or compare their contents."""
    return {
        'face-uv-map': _uv_face(),
        'nearest-texel': _nearest(),
        'repeat-addressing': _repeat(),
        'light-positions': _light_positions(),
        'light-directions': _light_directions(),
        **_lighting_terms(),
    }
