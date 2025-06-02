"""# Sphere visual example

Show the sphere visual with textures.

"""

import numpy as np
import imageio.v3 as iio

import datoviz as dvz
from datoviz import vec3


def load_texture_rgba(path):
    full_path = dvz.download_data(path)
    arr = iio.imread(full_path)
    return arr


def checker_array(shape):
    """
        Return an array with alternating pixels.
    """
    shape = np.array(shape)
    grid = np.zeros(1)
    for axis in shape:
        grid = grid[..., None] + np.arange(axis)
    return grid[0] % 2

# Get app environment.
app = dvz.App()

# Default texture to enable later changes to work.
#default_texture = app.texture(np.ones((4, 4, 4), dtype='uint8'), interpolation='linear', address_mode="repeat")

# Default visual setting values to save work later.
visual_values_defaults = dict(
    position = np.array(((0.0, 0.0, 0.0),), dtype='f'),
    color = np.array(((20, 100, 150, 255),)),
    size = np.array((0.33,), dtype='f'),
    lighting = True,
    light_pos=(-30, +30, +100, 1),
)


def get_visual_data():

    # list of visual data to add to panel.
    data = []
    count = 0
    pos = np.linspace(1, -1, 4)

    # Random pattern.
    w = h = 250
    image = np.random.rand(w * h * 4).astype('f').reshape((w, h, 4))
    values = visual_values_defaults.copy()
    y,x = divmod(count, 4)
    values['position'] = np.array(((-pos[x], pos[y], 0),), dtype='f')
    values['texture'] = app.texture(image, interpolation='linear', address_mode="repeat")
    data.append(values)
    count += 1

    # Random pattern with equal_rectangular == True:
    values = values.copy()  # Use previous data.
    y,x = divmod(count, 4)
    values['position'] = np.array(((-pos[x], pos[y], 0),), dtype='f')
    values['equal_rectangular'] = True
    data.append(values)
    count += 1

    # Checkered pattern:
    n = 24
    grid = checker_array((n, n)).astype('f')
    for axis in range(grid.ndim):
        grid = np.repeat(grid, 16, axis=axis)   # Increase pixel size of board squares.

    # Convert to rgba image array.
    image = np.array((grid,) * 4, dtype='f')
    image = np.moveaxis(image, 0, -1).astype('f', 'C')
    image[:,:,0] = 1.0   # Red
    image[:,:,3] = 0.5   # Alpha

    values = visual_values_defaults.copy()
    y,x = divmod(count, 4)
    values['position'] = np.array(((-pos[x], pos[y], 0),), dtype='f')
    values['texture'] = app.texture(image, interpolation='linear', address_mode="repeat")
    data.append(values)
    count += 1

    # Checkered pattern with equal_rectangular == True:
    values = values.copy()  # Use previous data.
    y,x = divmod(count, 4)
    values['position'] = np.array(((-pos[x], pos[y], 0),), dtype='f')
    values['equal_rectangular'] = True
    data.append(values)
    count += 1

    # Field Pattern
    x = np.linspace(0, 1, 512, dtype='f', endpoint=False)
    y = x[..., None]
    image = (np.sin(x * 16 * np.pi)) + np.cos(y * 16 * np.pi)
    image = np.stack((image, image, image, image), axis=-1)
    image = abs(image)
    image /= image.max()
    image[:, :, 3] = .8

    values = visual_values_defaults.copy()
    y,x = divmod(count, 4)
    values['position'] = np.array(((-pos[x], pos[y], 0),), dtype='f')
    values['texture'] = app.texture(image, interpolation='linear', address_mode="repeat")
    data.append(values)
    count += 1

    # Field pattern with equal_rectangular == True:
    values = values.copy()  # Use previous data.
    y,x = divmod(count, 4)
    values['position'] = np.array(((-pos[x], pos[y], 0),), dtype='f')
    values['equal_rectangular'] = True
    data.append(values)
    count += 1

    # Symbol pattern:
    image = load_texture_rgba('textures/pushpin.png')
    # Need to pad edges to avoid over wrapping image.
    p = 50
    image = np.pad(image, ((p, p), (p, p), (0, 0)), 'constant', constant_values=0)

    values = visual_values_defaults.copy()
    y,x = divmod(count, 4)
    values['position'] = np.array(((-pos[x], pos[y], 0),), dtype='f')
    values['texture'] = app.texture(image, interpolation='linear', address_mode="repeat")
    data.append(values)
    count += 1

    # Symbol pattern with equal_rectangular == True:
    values = values.copy()  # Use previous data.
    y,x = divmod(count, 4)
    values['position'] = np.array(((-pos[x], pos[y], 0),), dtype='f')
    values['equal_rectangular'] = True
    data.append(values)
    count += 1


    # Image
    image = load_texture_rgba('textures/landscape.jpg') / 255
    alpha = np.ones((image.shape[0], image.shape[1])) * 0.8
    image = np.stack((image[:, :, 0], image[:, :, 1], image[:, :, 2], alpha), axis=-1).astype('f')

    values = visual_values_defaults.copy()
    y,x = divmod(count, 4)
    values['position'] = np.array(((-pos[x], pos[y], 0),), dtype='f')
    values['texture'] = app.texture(image, interpolation='linear', address_mode="repeat")
    data.append(values)
    count += 1


    # Image with equal_rectangular == True:
    values = values.copy()  # Start this one with previous data.
    y,x = divmod(count, 4)
    values['position'] = np.array(((-pos[x], pos[y], 0),), dtype='f')
    values['equal_rectangular'] = True
    data.append(values)
    count += 1

    # Equal Rectangular Image
    # TODO: Enable code below after world texture is on github.
    # image = load_texture_rgba('textures/world.200412.3x5400x2700.jpg')/255
    # a = np.ones_like(img[..., -1])
    # image = np.zeros((img.shape[0], img.shape[1], img.shape[2] + 1), dtype='f')
    # image[:, :, :-1] = img
    # image[:, :, -1] = a
    #
    # values = visual_values_defaults.copy()
    # y,x = divmod(count, 4)
    # values['position'] = np.array(((-pos[x], pos[y], 0),), dtype='f')
    # values['texture'] = app.texture(image, interpolation='linear', address_mode="repeat")
    # data.append(values)
    # count += 1
    #
    # # Equal rectangular Image with equal_rectangular == True:
    # values = values.copy()  # Start this one with previous data.
    # y,x = divmod(count, 4)
    # values['position'] = np.array(((-pos[x], pos[y], 0),), dtype='f')
    # values['equal_rectangular'] = True
    # data.append(values)
    # count += 1

    return data


figure = app.figure()
panel = figure.panel(background=True)
arcball = panel.arcball()

data = get_visual_data()
for visual_data in data:
    visual = app.sphere(**visual_data)
    visual.set_material_params(vec3(.2, .2, .2), 0)  # Ambient
    visual.set_material_params(vec3(.9, .9, .9), 1)  # Diffuse
    visual.set_material_params(vec3(.2, .2, .2), 2)  # Specular
    visual.set_shine(.25)  # Specular level
    panel.add(visual)

app.run()
app.destroy()
