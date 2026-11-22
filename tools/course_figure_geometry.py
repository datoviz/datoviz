"""Byte layout, indexed geometry and coordinate-space course illustrations."""

from math import cos, sin

from course_figure_math import equation_fragment
from course_figure_svg import SVG, color


def vertex_bytes():
    """Show square byte cells with space between the records and their labels."""
    s = SVG(
        'vertex-bytes',
        'Vertex records as bytes',
        'Three consecutive 20-byte records, v0 to v2. Each square is one byte. '
        'Position uses eight bytes and color twelve; each group of four bytes is one float.',
        height=340,
    )
    s.text(20, 28, 'Each square is one byte', bold=True)
    x0, cell = 40, 12
    s.text(88, 66, 'position: 8 bytes', 14, 'blue', anchor='middle')
    s.text(208, 66, 'color: 12 bytes', 14, 'orange', anchor='middle')
    for j, label in enumerate(['x', 'y', 'r', 'g', 'b']):
        s.text(x0 + j * 48 + 24, 91, label, anchor='middle')
    for row in range(3):
        y = 105 + row * 48
        s.text(12, y + 11, f'v{row}', 14)
        for j in range(20):
            s.rect(x0 + j * cell, y, cell, cell, 'blue' if j < 8 else 'orange', 'paper')
        for j in range(5):
            s.rect(x0 + j * 48, y, 48, cell, 'none', 'ink')
        s.text(x0, y + 34, f'buffer bytes {row * 20}–{row * 20 + 19}', 14, 'muted')
    s.text(20, 266, 'Position offset: 0; color offset: 8.', 14)
    s.text(20, 291, 'Stride: 20 bytes to the next record.', 14)
    s.text(20, 316, 'v1 color starts at buffer byte 28.', 14, bold=True)
    return s.finish()


def vertex_fetch():
    """Map the two vertex attributes to their GLSL input locations."""
    s = SVG(
        'vertex-fetch',
        'Decode one vertex record',
        'Binding zero supplies the byte stream. At each record start, offset '
        'zero and format R32G32_SFLOAT provide two floats at shader location '
        'zero. Offset eight and R32G32B32_SFLOAT provide three floats at '
        'location one.',
        height=340,
    )
    s.text(20, 28, 'Binding 0: one byte stream', bold=True)
    for y, title, sub, shader, c in [
        (
            49,
            'Offset 0 · bytes 0–7',
            'Read 2 × 32-bit floats',
            'location 0 → vec2 in_position',
            'blue',
        ),
        (
            173,
            'Offset 8 · bytes 8–19',
            'Read 3 × 32-bit floats',
            'location 1 → vec3 in_color',
            'orange',
        ),
    ]:
        s.rect(12, y, 296, 51, 'paper', c, 4)
        s.text(23, y + 21, title, 14, c, bold=True)
        s.text(23, y + 41, sub, 14)
        s.arrow(160, y + 55, 160, y + 72, c)
        s.text(160, y + 96, shader, 14, anchor='middle')
    return s.finish()


def indexed_square():
    """Show the two triangles that share four vertex records."""
    s = SVG(
        'indexed-square',
        'Two triangles share four vertex records',
        'The square has corner zero top left, one top right, two bottom '
        'right and three bottom left. Its diagonal joins zero to two. '
        'Triangles use indices zero one two and zero two three.',
        height=300,
    )
    s.text(12, 24, 'Four complete vertex records', bold=True)
    p = [(68, 54), (248, 54), (248, 234), (68, 234)]
    s.polygon([p[0], p[1], p[2]], 'blue', 'blue', 0.22)
    s.polygon([p[0], p[2], p[3]], 'orange', 'orange', 0.22)
    for i, (x, y) in enumerate(p):
        s.circle(x, y, 5, 'blue' if i in [0, 2] else 'ink')
        s.text(x + (-17 if i in [0, 3] else 13), y + (-10 if i < 2 else 20), str(i), 16, bold=True)
    s.text(190, 116, '0, 1, 2', 15, 'blue', anchor='middle')
    s.text(123, 191, '0, 2, 3', 15, 'orange', anchor='middle')
    for cx, cy, c in [(190, 111, 'blue'), (123, 186, 'orange')]:
        r = 29
        s.raw(
            f'<path d="M{cx},{cy - r} A{r},{r} 0 1 1 {cx - r},{cy}" '
            f'fill="none" stroke="{color(c)}" stroke-width="1.8"/>'
        )
        s.arrow(cx - r, cy + 8, cx - r, cy, c, 1.8)
    s.text(160, 279, 'Both triangles wind clockwise here.', 14, anchor='middle')
    return s.finish()


def index_stream():
    """Show the six indices and the size of the uploaded payload."""
    s = SVG(
        'index-stream',
        'Read six indices, reuse four records',
        'The six 16-bit indices are zero one two, then zero two three. The '
        'two index triples select complete vertices, including positions and '
        'colors. Four 20-byte records plus six 2-byte indices occupy 92 '
        'bytes, compared with 120 non-indexed bytes.',
        height=300,
    )
    s.text(12, 24, 'Read indices in groups of three', bold=True)
    for i, n in enumerate([0, 1, 2, 0, 2, 3]):
        x = 16 + i * 48
        s.rect(x, 57, 43, 43, 'paper', 'blue' if i < 3 else 'orange', 3)
        s.text(x + 21, 85, n, 20, anchor='middle', bold=True)
    s.line(16, 112, 155, 112, 'blue', 3)
    s.line(160, 112, 299, 112, 'orange', 3)
    s.text(85, 136, 'triangle 1', 14, 'blue', anchor='middle')
    s.text(230, 136, 'triangle 2', 14, 'orange', anchor='middle')
    s.text(12, 178, 'An index selects the whole record:', 14, bold=True)
    s.text(12, 202, 'position AND color travel together.', 14)
    s.text(12, 244, 'Vertex bytes: 4 × 20 = 80', 14)
    s.text(12, 269, 'Index bytes: 6 × 2 = 12; total = 92', 14)
    return s.finish()


def project_iso(p):
    """Project a three-dimensional point into the schematic drawing plane."""
    x, y, z = p
    return (160 + 70 * (0.84 * x + 0.54 * z), 142 - 70 * (y - 0.22 * x + 0.34 * z))


def transformed_cube(which):
    """Draw the stored cube or its world-space pose at time zero."""
    tilt = -0.35
    points = [(x, y, z) for z in [-0.65, 0.65] for y in [-0.65, 0.65] for x in [-0.65, 0.65]]

    def model(p):
        x, y, z = p
        return (x, cos(tilt) * y - sin(tilt) * z, sin(tilt) * y + cos(tilt) * z)

    if which == 'object':
        title = '1. Object space'
        vertices = points
        desc = (
            'The original cube is centered at its local origin. This is a '
            'schematic view of the stored coordinates.'
        )
    else:
        title = '2. World space'
        vertices = [model(p) for p in points]
        desc = (
            'At time zero the model tilts the cube by minus 0.35 radians about '
            'x; the spin is zero and scale is one. The cube stays centered at '
            'the world origin.'
        )
    s = SVG('cube-' + which, title, desc, height=300)
    s.text(12, 24, title, bold=True)
    origin = project_iso((0, 0, 0))
    for p, label in [((1.1, 0, 0), 'x'), ((0, 1.2, 0), 'y'), ((0, 0, 1.1), 'z')]:
        q = project_iso(p)
        s.arrow(*origin, *q, 'line', 1)
        s.text(q[0] + 5, q[1] + 3, label, 14, 'muted')
    for i, p in enumerate(vertices):
        for j in range(i + 1, 8):
            if sum(a != b for a, b in zip(points[i], points[j], strict=True)) == 1:
                s.line(*project_iso(p), *project_iso(vertices[j]), 'blue', 1.8)
    corner = vertices[7]
    q = project_iso(corner)
    s.circle(*q, 4, 'orange')
    s.text(20, 263, 'Same corner', 14, 'orange')
    s.text(
        20, 287, 'Stored positions' if which == 'object' else 'Tilt about the center', 14, 'muted'
    )
    return s.finish()


def camera_view(world=False):
    """Compare two coordinate origins using identical camera and cube positions."""
    name = 'camera-world' if world else 'camera-view'
    title = '3a. Measure from the scene origin' if world else '3b. Measure from the camera'
    description = (
        'Side view of the same scene, with z increasing to the right. The camera looks left. '
        'In world coordinates the cube center is z zero and the camera is z three. '
        'In view coordinates the camera is z zero and the cube center is z minus three. '
        'Only the reference origin changes; the camera and cube keep their positions.'
    )
    s = SVG(name, title, description, height=300)
    s.text(16, 27, title, 14, bold=True)
    s.text(16, 52, 'World coordinates' if world else 'View coordinates', 14, 'muted')
    cube, camera, axis = 60, 240, 154
    # Both panels draw the same side view. A camera icon distinguishes the eye from a vertex.
    s.rect(cube - 12, 89, 24, 24, 'blue', 'blue')
    s.rect(camera - 9, 91, 20, 20, 'green', 'green', 2)
    s.polygon(
        [(camera - 9, 94), (camera - 19, 87), (camera - 19, 115), (camera - 9, 108)], 'green'
    )
    s.text(cube, 134, 'cube', 14, anchor='middle')
    s.text(camera, 134, 'camera', 14, anchor='middle')
    s.arrow(camera - 27, 78, cube + 38, 78, 'green', 1.5)
    s.text(162, 72, 'looks along −z', 14, 'green', anchor='middle')
    s.line(cube, 113, cube, axis, 'line', 1, True)
    s.line(camera, 113, camera, axis, 'line', 1, True)
    s.arrow(30, axis, 285, axis, 'line', 1.5)
    s.text(291, axis + 5, '+z', 14)
    for i in range(4):
        x = cube + i * 60
        value = i if world else i - 3
        is_origin = value == 0
        if is_origin:
            s.circle(x, axis, 9, 'paper', 'blue' if world else 'green')
        else:
            s.line(x, axis - 4, x, axis + 4, 'line')
        s.text(
            x,
            axis + 30,
            str(value).replace('-', '−'),
            16,
            'blue' if world and is_origin else 'green' if is_origin else 'ink',
            anchor='middle',
            bold=is_origin,
        )
    s.text(16, 219, 'Zero is at the cube’s center.' if world else 'Zero is now at the camera.', 14)
    s.text(
        16,
        246,
        'The camera is 3 units along +z.' if world else 'The cube is 3 units along −z.',
        14,
    )
    s.text(
        16,
        273,
        'The scene origin is our choice.' if world else 'Its view-space z is −3.',
        14,
        'muted',
    )
    return s.finish()


def perspective_rays():
    """Show how distance changes apparent size on a virtual image plane."""
    s = SVG(
        'perspective-rays',
        'Equal sizes at different distances',
        'Schematic x-z side view, with positive x upward. A camera at '
        'distance zero sees equal-length segments at view z minus two and '
        'minus four. Rays through a virtual image plane at distance one '
        'produce heights in the ratio two to one. Distances and image-plane '
        'position are illustrative.',
        height=340,
    )
    s.text(12, 24, 'Same size, twice the distance', bold=True)
    eye = (28, 172)
    plane = 88
    near = 148
    far = 268
    s.line(20, 172, 300, 172, 'line', 1, True)
    s.line(plane, 75, plane, 268, 'line', 1, True)
    for x, c in [(far, 'orange'), (near, 'blue')]:
        s.line(x, 112, x, 232, c, 3)
        # Rays intersect distance-one plane; collinear endpoint geometry is exact.
        for y in [112, 232]:
            s.line(*eye, x, y, c, 1.2)
        half = 60 * (plane - eye[0]) / (x - eye[0])
        s.line(plane, 172 - half, plane, 172 + half, c, 4)
    s.circle(*eye, 4, 'ink')
    s.text(17, 197, 'eye', 14)
    s.arrow(28, 151, 28, 102, 'line', 1)
    s.text(28, 92, '+x', 14, anchor='middle')
    s.text(plane, 61, 'virtual plane', 14, anchor='middle')
    s.text(near, 254, 'nearer', 14, anchor='middle')
    s.text(far, 254, 'farther', 14, anchor='middle')
    s.text(12, 290, 'Half the apparent size at twice the distance.', 14)

    return s.finish()


def perspective_numbers():
    """Explain the perspective divide symbolically, without a worked calculation."""
    s = SVG(
        'perspective-divide',
        'Why distance changes apparent size',
        'NDC x is clip x divided by clip w. In this projection w is positive distance '
        'in front of the camera. A greater distance makes the denominator larger '
        'and moves the point closer to the screen center.',
        height=340,
    )
    s.text(20, 28, 'Divide by distance', bold=True)
    s.raw(
        equation_fragment(
            'perspective-x',
            r'x_{\mathrm{NDC}}=\frac{x_{\mathrm{clip}}}{w_{\mathrm{clip}}}',
            20,
            62,
            280,
            66,
        )
    )
    s.raw(
        equation_fragment(
            'perspective-w', r'w_{\mathrm{clip}}=-z_{\mathrm{view}}', 20, 144, 280, 40
        )
    )
    s.text(20, 221, 'Here, w is the distance in front', 14)
    s.text(20, 244, 'of the camera.', 14)
    s.text(20, 282, 'Farther away → larger denominator', 14)
    s.text(20, 307, '                    → smaller image', 14)
    return s.finish()


def figures():
    """Return the geometry panels as deterministic SVG source strings."""
    return {
        'vertex-bytes': vertex_bytes(),
        'vertex-fetch': vertex_fetch(),
        'indexed-square': indexed_square(),
        'index-stream': index_stream(),
        'cube-object': transformed_cube('object'),
        'cube-world': transformed_cube('world'),
        'camera-world': camera_view(world=True),
        'camera-view': camera_view(),
        'perspective-rays': perspective_rays(),
        'perspective-divide': perspective_numbers(),
    }
