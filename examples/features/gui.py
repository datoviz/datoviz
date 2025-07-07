"""
# GUI widgets

Show how to create a GUI dialog.

---
tags:
  - gui
  - table
  - tree
  - widgets
in_gallery: true
make_screenshot: false
---

"""

import numpy as np

import datoviz as dvz
from datoviz import Out, vec2, vec3

# Dialog width.
w = 300

labels = ['col0', 'col1', 'col2', '0', '1', '2', '3', '4', '5']
rows = 2
cols = 3
selected = np.array([False, True], dtype=np.bool)

# IMPORTANT: these values need to be defined outside of the GUI callback.
checked = Out(True)
color = vec3(0.7, 0.5, 0.3)

slider = Out(25.0)  # Warning: needs to be a float as it is passed to a function expecting a float
dropdown_selected = Out(1)

# GUI callback function, called at every frame. This is using Dear ImGui, an immediate-mode
# GUI system. This means the GUI is recreated from scratch at every frame.


app = dvz.App()
# NOTE: at the moment, you must indicate gui=True if you intend to use a GUI in a figure
figure = app.figure(gui=True)


@app.connect(figure)
def on_gui(ev):
    # Set the size of the next GUI dialog.
    dvz.gui_pos(vec2(25, 25), vec2(0, 0))
    dvz.gui_size(vec2(w + 20, 550))

    # Start a GUI dialog, specifying a dialog title.
    dvz.gui_begin('My GUI', 0)

    # Add a button. The function returns whether the button was pressed during this frame.
    if dvz.gui_button('Button', w, 30):
        print('button clicked')

    # Create a tree, this call returns True if this node is unfolded.
    if dvz.gui_node('Item 1'):
        # Display an item in the tree.
        dvz.gui_selectable('Hello inside item 1')
        # Return True if this item was clicked.
        if dvz.gui_clicked():
            print('clicked sub item 1')
        # Go up one level.
        dvz.gui_pop()

    if dvz.gui_node('Item 2'):
        if dvz.gui_node('Item 2.1'):
            dvz.gui_selectable('Hello inside item 2')
            if dvz.gui_clicked():
                print('clicked sub item 2')
            dvz.gui_pop()
        dvz.gui_pop()

    if dvz.gui_table('table', rows, cols, labels, selected, 0):
        print('Selected rows:', np.nonzero(selected)[0])

    if dvz.gui_checkbox('Checkbox', checked):
        print('Checked status:', checked.value)

    if dvz.gui_colorpicker('Color picker', color, 0):
        print('Color:', color)

    if dvz.gui_slider('Slider', 0.0, 100.0, slider):
        print('Slider value:', slider.value)

    if dvz.gui_dropdown('Dropdown', 3, ['item 1', 'item 2', 'item 3'], dropdown_selected, 0):
        print('Dropdown index:', dropdown_selected.value)

    # End the GUI dialog.
    dvz.gui_end()


app.run()
app.destroy()
