"""
# Timer

Show how to use timers.

---
tags:
  - glyph
  - timer
in_gallery: true
make_screenshot: true
---

"""

import datoviz as dvz

color = [255, 255, 255, 255]

app = dvz.App()
figure = app.figure()
panel = figure.panel(background=True)
visual = app.glyph(font_size=84)
visual.set_strings(['Starting timer...'], color=color)
panel.add(visual)


@app.timer(period=0.5, delay=0.5, max_count=6)
def on_timer(e):
    t = e.time()
    i = e.tick()
    visual.set_strings([f'tick #{i}/5    t={t:.1f}'], color=color)
    if i > 5:
        visual.set_strings(['Timer stopped.'], color=color)


app.run()
app.destroy()
