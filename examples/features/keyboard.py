"""# Keyboard example

Show how to react to keyboard events.

"""

import datoviz as dvz


app = dvz.App()
figure = app.figure()


@app.on_keyboard(figure)
def on_keyboard(ev):
    print(f"{ev.key_event()} key {ev.key()} ({ev.key_name()})")


app.run()
app.destroy()
