"""
# Advanced mesh lights

Show how to manipulate advanced mesh lights.

---
tags:
  - mesh
  - light
in_gallery: true
make_screenshot: true
---

"""

import datoviz as dvz
from datoviz import Out, vec3, vec4

file_path = dvz.download_data('mesh/bunny.obj')

light_pos = (
    vec4(0, 0, 5, 1),  # Pos 0  x,y,z
    vec4(-5, 0, 5, 1),  # Pos 1  x,y,z
    vec4(0, 5, 5, 1),  # Pos 2  x,y,z
    vec4(5, 0, 5, 1),  # Pos 3  x,y,z
)

light_color = (
    vec4(1, 1, 1, 1),  # White
    vec4(1, 0, 0, 1),  # Red
    vec4(0, 1, 0, 1),  # Blue
    vec4(0, 0, 1, 1),  # Green
)

material = dict(
    ambient_params=vec3(0.25, 0.25, 0.25),  # R, G, B levels
    diffuse_params=vec3(0.75, 0.75, 0.75),
    specular_params=vec3(0.75, 0.75, 0.75),
    emission_params=vec3(0.5, 0.5, 0.5),
    shine=Out(0.5),
    emit=Out(0.0),
)


sc = dvz.ShapeCollection()
sc.add_obj(file_path)

app = dvz.App()
# NOTE: at the moment, you must indicate gui=True if you intend to use a GUI in a figure
figure = app.figure(gui=True)

panel = figure.panel(background=True)

arcball = panel.arcball(initial=(0.35, 0, 0))
camera = panel.camera(initial=(-0.75, 0, 4), initial_lookat=(-0.75, 0, 0))
visual = app.mesh(sc, lighting=True)
visual.clip('outer')
panel.add(visual)


# GUI callback
def update_params():
    for i in range(4):
        c = light_color[i]
        visual.set_light_color(
            (int(c[0] * 255), int(c[1] * 255), int(c[2] * 255), int(c[3] * 255)), i
        )
        visual.set_light_pos(light_pos[i], i)
    visual.set_ambient_params(material['ambient_params'])
    visual.set_diffuse_params(material['diffuse_params'])
    visual.set_specular_params(material['specular_params'])
    visual.set_emission_params(material['emission_params'])
    visual.set_shine(material['shine'].value)
    visual.set_emit(material['emit'].value)
    visual.update()


update_params()


@app.connect(figure)
def on_gui(ev):
    dvz.gui_size(dvz.vec2(350, 500))
    dvz.gui_pos(dvz.vec2(20, 20), dvz.vec2(0, 0))

    dvz.gui_begin('Change the light', 0)
    has_changed = False

    dvz.gui_text('Light 1:')
    has_changed |= dvz.gui_slider_vec4('Pos 0 XYZ', -20, +20, light_pos[0])
    has_changed |= dvz.gui_slider_vec4('Color 0 RGBA', 0, 1, light_color[0])

    dvz.gui_text('Light 2:')
    has_changed |= dvz.gui_slider_vec4('Pos 1 XYZ', -20, +20, light_pos[1])
    has_changed |= dvz.gui_slider_vec4('Color 1 RGBA', 0, 1, light_color[1])

    dvz.gui_text('Light 3:')
    has_changed |= dvz.gui_slider_vec4('Pos 2 XYZ', -20, +20, light_pos[2])
    has_changed |= dvz.gui_slider_vec4('Color 2 RGBA', 0, 1, light_color[2])

    dvz.gui_text('Light 4:')
    has_changed |= dvz.gui_slider_vec4('Pos 3 XYZ', -20, +20, light_pos[3])
    has_changed |= dvz.gui_slider_vec4('Color 3 RGBA', 0, 1, light_color[3])

    dvz.gui_text('Material properties:')
    has_changed |= dvz.gui_slider_vec3('Ambient RGB', 0, 1, material['ambient_params'])
    has_changed |= dvz.gui_slider_vec3('Diffuse RGB', 0, 1, material['diffuse_params'])
    has_changed |= dvz.gui_slider_vec3('Specular RGB', 0, 1, material['specular_params'])
    has_changed |= dvz.gui_slider_vec3('Emission RGB', 0, 1, material['emission_params'])
    has_changed |= dvz.gui_slider('Shininess level', 0, 1, material['shine'])
    has_changed |= dvz.gui_slider('Emission level', 0, 1, material['emit'])

    dvz.gui_end()

    if has_changed:
        update_params()


app.run()
app.destroy()
