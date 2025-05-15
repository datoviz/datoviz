"""
# Mesh Light example

Show how to manipulate advanced mesh lights.

"""
from pathlib import Path

import datoviz as dvz
from datoviz import vec3

ROOT_DIR = Path(__file__).resolve().parent.parent.parent
file_path = ROOT_DIR / 'data/mesh/bunny.obj'

light_color = (vec3( 0,  0,  1),    # Blue
               vec3( 0,  1,  0),    # Green
               vec3(.7, .7,  0),    # Yellow
               vec3( 1,  0,  0))    # Red

light_pos = (vec3(-9, 1, 5),      # Pos 0  x,y,z
             vec3(-3, 1, 5),      # Pos 1  x,y,z
             vec3( 3, 1, 5),      # Pos 2  x,y,z
             vec3( 9, 1, 5))      # Pos 3  x,y,z

material = (vec3(.2, .2, .2),     # Ambient    R, G, B
            vec3(.8, .8, .8),     # Diffuse    R, G, B
            vec3(.8, .8, .8),     # Specular   R, G, B
            vec3(.9, .9, .9))     # Shininess  R, G, B

sc = dvz.ShapeCollection()
sc.add_obj(file_path, contour='full')

app = dvz.App()
# NOTE: at the moment, you must indicate gui=True if you intend to use a GUI in a figure
figure = app.figure(gui=True)
panel = figure.panel()
arcball = panel.arcball(initial=(0.35, 0, 0))

visual = app.mesh_shape(sc, lighting=True)

panel.add(visual)

# GUI callback
def update_params():
    for i in range(4):
        c = light_color[i]
        visual.set_light_color((int(c[0] * 255),
                                int(c[1] * 255),
                                int(c[2] * 255)) , i)
        visual.set_light_dir(light_pos[i], i)
        visual.set_light_params(material[i], i)
    visual.update()

update_params()

@app.connect(figure)
def on_gui(ev):
    dvz.gui_size(dvz.vec2(500, 350))
    dvz.gui_begin('Change the light', 0)
    has_changed = False

    has_changed |= dvz.gui_slider_vec3('Pos 0 XYZ', -10, +10, light_pos[0])
    has_changed |= dvz.gui_slider_vec3('Pos 1 XYZ', -10, +10, light_pos[1])
    has_changed |= dvz.gui_slider_vec3('Pos 2 XYZ', -10, +10, light_pos[2])
    has_changed |= dvz.gui_slider_vec3('Pos 3 XYZ', -10, +10, light_pos[3])

    has_changed |= dvz.gui_slider_vec3('Color 0 RGB', 0, 1, light_color[0])
    has_changed |= dvz.gui_slider_vec3('Color 1 RGB', 0, 1, light_color[1])
    has_changed |= dvz.gui_slider_vec3('Color 2 RGB', 0, 1, light_color[2])
    has_changed |= dvz.gui_slider_vec3('Color 3 RGB', 0, 1, light_color[3])

    has_changed |= dvz.gui_slider_vec3('Ambient RGB', 0, 1, material[0])
    has_changed |= dvz.gui_slider_vec3('Diffuse RGB', 0, 1, material[1])
    has_changed |= dvz.gui_slider_vec3('Specular RGB', 0, 1, material[2])
    has_changed |= dvz.gui_slider_vec3('Shininess RGB', 0, 1, material[3])

    dvz.gui_end()

    if has_changed:
        update_params()

app.run()
app.destroy()
