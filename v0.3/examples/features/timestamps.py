"""
# Canvas presentation timestamps

Show how to retrieve the exact timestamps of the presentation of the last frames to the screen.
This may be useful in specific use-cases (e.g. hardware synchronization in scientific experimental
setups).

---
tags:
  - timer
  - timestamps
in_gallery: true
make_screenshot: false
---

"""

import numpy as np

import datoviz as dvz

app = dvz.App()
figure = app.figure()
panel = figure.panel()
panel.demo_2D()


@app.timer(delay=0.0, period=1.0, max_count=0)
def on_timer(ev):
    # Every second, we show the timestamps of the last `count` frames.
    # NOTE: it is currently impossible to call dvz.app_timestamps() after the window was closed.
    # The timestamps are automatically recorded at every frame, this call fetches the last 5.
    seconds, nanoseconds = app.timestamps(figure, 5)

    # We display the values.
    print('Last 5 frames:')
    print(np.c_[seconds, nanoseconds])


app.run()
app.destroy()
