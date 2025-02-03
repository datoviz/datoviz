"""# Timestamps example

Show how to retrieve the exact timestamps of the presentation of the last frames to the screen.
This may be useful in specific use-cases (e.g. hardware synchronization in scientific experimental
setups).

Illustrates:

- How to retrieve the precise presentation times of the last N frames.
- How to use a demo panel.

"""

import numpy as np
import datoviz as dvz
from datoviz import A_

app = dvz.app(0)
batch = dvz.app_batch(app)
scene = dvz.scene(batch)
figure = dvz.figure(scene, 800, 600, 0)

panel = dvz.panel_default(figure)
dvz.demo_panel(panel)

# Frame presentation timestamps.
# Every second, we show the timestamps of the last `count` frames.
count = 5

# We prepare the arrays holding the data.
seconds = np.zeros(count, dtype=np.uint64) # epoch, in seconds
nanoseconds = np.zeros(count, dtype=np.uint64) # number of ns within the second

@dvz.timer
def on_timer(app, window_id, ev):
    # The timestamps are automatically recorded at every frame, this call fetches the last
    # `count` ones.
    dvz.app_timestamps(app, dvz.figure_id(figure), count, A_(seconds), A_(nanoseconds))

    # We display the values.
    print(f"Last {count} frames:")
    print('    '.join(f"{s % 1000:03d} s {round(ns / 1000000.) :03d} ms {(round(ns / 1000) % 1000):03d} us" for s, ns in zip(seconds, nanoseconds)))

# Timer: retrieve and display the timestamps every second.
# NOTE: it is currently impossible to call dvz.app_timestamps() after the window has been closed.
dvz.app_ontimer(app, on_timer, None)
dvz.app_timer(app, 0, 1, 0)

# Run the application and cleanup.
dvz.scene_run(scene, app, 0)
dvz.scene_destroy(scene)
dvz.app_destroy(app)
