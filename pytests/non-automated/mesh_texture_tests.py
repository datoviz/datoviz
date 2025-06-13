"""

    Mesh visual texture tests.

"""
import numpy as np
import imageio.v3 as iio
import datoviz as dvz


def load_image(path):
    try:
        arr = iio.imread(path)
    except FileNotFoundError:
        full_path = dvz.download_data(path)
        arr = iio.imread(full_path)
    if arr.shape[2] == 3:
        image = np.ones((arr.shape[0], arr.shape[1], 4), dtype='uint8') * 255
        image[:, :, :3] = arr
        arr = image
    return arr.astype('uint8', 'C')


def rcl(n):
    row, col = divmod(n, 3)
    return col - 1, 1 - row, 0


sc = dvz.ShapeCollection()
scale = 0.75
sc.add_square(offset=rcl(0), scale=scale)
sc.add_disc(offset=rcl(1), count=36, scale=scale)
sc.add_sector(offset=rcl(2), angle_start=0, angle_stop=4.5, scale=scale)
sc.add_cube(offset=rcl(3), scale=scale)
sc.add_sphere(offset=rcl(4), scale=scale)
sc.add_cylinder(offset=rcl(5), scale=scale)
sc.add_cone(offset=rcl(6), scale=scale)
sc.add_torus(offset=rcl(7), tube_radius=.2, scale=scale)

# Surface
x = np.linspace(0, 2 * 2.0 * np.pi, 64, dtype='f')
y = np.linspace(0, 2 * 2.0 * np.pi, 64, dtype='f')
heights = (np.sin(x) * np.cos(y)[..., None])/10.0
colors = dvz.cmap('plasma', heights, -1.0, +1.0)
ox, oy, oz = rcl(8)  # offsets
sc.add_surface(offset=(ox + 0.5, oy - 1.5, oz - 1.0), heights=heights, colors=colors, scale=scale/2.0)


def show_images(texture_file_path):
    app = dvz.App()
    figure = app.figure()
    panel = figure.panel(background=True)

    # Add a 3D gizmo.
    panel.gizmo()

    arcball = panel.arcball()

    image = load_image(texture_file_path)
    texture = app.texture(image, interpolation='linear', address_mode="repeat")

    visual = app.mesh(sc, lighting=True, texture=texture)
    panel.add(visual)

    app.run()
    app.destroy()


if __name__ == '__main__':

    texture_file_path = 'textures/crate_this_side_up.jpg'
    show_images(texture_file_path)

    texture_file_path = 'textures/world.200412.3x5400x2700.jpg'
    show_images(texture_file_path)

    sc.destroy()
