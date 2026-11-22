"""Byte layout, indexed geometry and coordinate-space course illustrations."""

from math import cos, sin

from course_figure_svg import SVG


def vertex_bytes():
    """Show three consecutive records and the offsets within each record."""
    s = SVG(
        'vertex-bytes',
        'Vertex records as bytes',
        'Each row shows one 20-byte vertex: two four-byte position floats '
        'followed by three four-byte color floats. Rows are consecutive '
        'memory, not separate buffers.',
        height=315,
    )
    s.text(12, 24, 'One square = one byte', bold=True)
    x0 = 72
    cell = 11
    s.text(x0 + 44, 51, 'position: 8 bytes', 14, 'blue', anchor='middle')
    s.text(x0 + 154, 51, 'color: 12 bytes', 14, 'orange', anchor='middle')
    for j, label in enumerate(['x', 'y', 'r', 'g', 'b']):
        s.text(x0 + j * 44 + 22, 73, label, 14, anchor='middle')
    for row in range(3):
        y = 88 + row * 38
        s.text(8, y + 14, f'Vertex {row}', 14)
        for j in range(20):
            s.rect(x0 + j * cell, y, cell, 20, 'blue' if j < 8 else 'orange', 'paper')
        for j in range(6):
            s.line(x0 + j * 44, y, x0 + j * 44, y + 20, 'ink', 1)
        s.text(x0, y + 32, f'buffer bytes {row * 20}–{row * 20 + 19}', 14, 'muted')
    s.text(72, 217, '… three more records …', 14, 'muted')
    s.text(12, 246, 'Position offset = 0; color offset = 8.', 14)
    s.text(12, 268, 'Stride = 20 bytes between record starts.', 14)
    s.text(12, 294, 'Vertex 1 color starts at buffer byte 28.', 14, bold=True)
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
        height=315,
    )
    s.text(12, 24, 'Binding 0: one byte stream', bold=True)
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
    s.text(160, 282, 'Corners 0 and 2 belong to both triangles.', 14, anchor='middle')
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
    s.text(
        12,
        253,
        'Corner: ' + ('(0.650, 0.650, 0.650)' if which == 'object' else '(0.650, 0.833, 0.388)'),
        14,
    )
    s.text(12, 273, 'Spatial schematic, not the rendered image.', 14, 'muted')
    s.text(
        12,
        294,
        'Stored coordinates' if which == 'object' else 'M at time = 0: fixed tilt, no spin',
        14,
        'muted',
    )
    return s.finish()


def camera_view():
    """Compare world and view coordinates for the same camera and cube."""
    s = SVG(
        'camera-view',
        'World coordinates become view coordinates',
        'Side view at time zero. The world camera is at z equals three and '
        'the cube center at z zero. Subtracting three from every world z '
        'puts the camera at the view origin and the cube center at z minus '
        'three. The scene is re-expressed, not physically moved.',
        height=300,
    )
    s.text(12, 24, '3. View space', bold=True)
    for y, label, cube, eye, ticks in [
        (88, 'World', 75, 255, ['z = 0', 'z = +3']),
        (199, 'View', 75, 255, ['z = −3', 'z = 0']),
    ]:
        s.text(12, y - 26, label, 14, bold=True)
        s.line(45, y, 280, y, 'line')
        s.rect(cube - 13, y - 14, 26, 28, 'blue', 'blue')
        s.circle(eye, y, 7, 'orange')
        s.arrow(eye - 12, y - 24, cube + 12, y - 24, 'orange', 1.5)
        if label == 'World':
            s.text(cube, y + 22, 'cube center', 14, anchor='middle')
            s.text(eye, y + 22, 'camera', 14, anchor='middle')
        for x, t in [(cube, ticks[0]), (eye, ticks[1])]:
            s.text(x, y + 42, t, 14, anchor='middle')
    s.text(160, 147, 'view z = world z − 3', 15, anchor='middle', bold=True)
    s.text(12, 267, 'Same scene, camera-centered coordinates.', 14, 'muted')
    s.text(12, 292, 'Corner z: 0.388 → −2.612', 14, 'orange')
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
        height=320,
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
    s.text(near, 254, 'z = −2', 14, anchor='middle')
    s.text(far, 254, 'z = −4', 14, anchor='middle')
    s.text(12, 290, 'Half the apparent size at twice the distance.', 14)
    s.text(12, 311, 'Illustrative distances; not near/far clip planes.', 14, 'muted')
    return s.finish()


def perspective_numbers():
    """Evaluate the perspective divide for two illustrative view-space points."""
    s = SVG(
        'perspective-divide',
        'Projection and the divide by w',
        'For the course projection at aspect one, x clip is about 1.732 '
        'times view x and w is minus view z. For view x one at z minus two, '
        'NDC x is about 0.866. At z minus four, it is about 0.433. The '
        'second point is halfway from the center on screen.',
        height=320,
    )
    s.text(12, 24, 'The divide makes distance matter', bold=True)
    s.text(12, 58, 'Course projection · aspect = 1', 14, 'muted')
    s.text(12, 88, 'xclip = 1.732 × xview', 18)
    s.text(12, 115, 'wclip = −zview', 18)
    s.text(12, 147, 'For xview = 1:', 14, bold=True)
    for y, z, w, ndc, c in [
        (183, '−2', '2', '0.866', 'blue'),
        (218, '−4', '4', '0.433', 'orange'),
    ]:
        s.text(12, y, f'z = {z}   →   w = {w}', 15, c)
        s.text(187, y, f'xNDC ≈ {ndc}', 15, c)
    s.text(12, 258, 'xNDC = xclip / wclip', 18, bold=True)
    s.text(12, 291, 'The GPU performs this division.', 14)
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
        'camera-view': camera_view(),
        'perspective-rays': perspective_rays(),
        'perspective-divide': perspective_numbers(),
    }
