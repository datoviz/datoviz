"""
# Mouse events

Show how to react to mouse events.

---
tags:
  - mouse
in_gallery: true
make_screenshot: false
---

"""

import datoviz as dvz

app = dvz.App()
figure = app.figure()


@app.connect(figure)
def on_mouse(ev):
    action = ev.mouse_event()
    x, y = ev.pos()
    print(f'{action} ({x:.0f}, {y:.0f}) ', end='')

    if action in ('click', 'double_click'):
        button = ev.button_name()
        print(f'{button} button', end='')

    if action in ('drag_start', 'drag_stop', 'drag'):
        button = ev.button_name()
        xd, yd = ev.press_pos()
        print(f'{button} button pressed at ({xd:.0f}, {yd:.0f})', end='')

    if action == 'wheel':
        w = ev.wheel()
        print(f'wheel direction {w}', end='')

    print()


app.run()
app.destroy()
