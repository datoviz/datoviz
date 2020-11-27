import time
import numpy as np
from visky import App

app = App()
canvas = app.canvas()


def f(pos):
    print(pos)


canvas.connect('mouse', f)
app.run()
app.destroy()
