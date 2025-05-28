"""
# Keyboard events

Show how to react to keyboard events.

---
tags:
  - keyboard
in_gallery: true
make_screenshot: false
---

"""

import datoviz as dvz

app = dvz.App()
figure = app.figure()


@app.connect(figure)
def on_keyboard(ev):
    print(f'{ev.key_event()} key {ev.key()} ({ev.key_name()})')


app.run()
app.destroy()
