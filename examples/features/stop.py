"""
# Stopping an application

Show how to stop the application while it is running.

---
tags:
  - stop
  - timer
in_gallery: true
make_screenshot: false
---

"""

import datoviz as dvz

app = dvz.App()
figure = app.figure()
panel = figure.panel(background=True)
panel.demo_2D()


# This call will stop the application after 500 ms.
@app.timer(delay=0.5)
def on_timer(ev):
    app.stop()


app.run()
app.destroy()
