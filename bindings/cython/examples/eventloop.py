from pathlib import Path
import asyncio
import aiohttp

import numpy as np
import numpy.random as nr
from datoviz import app, next_frame, canvas, run, colormap


async def fetch(session, url):
    async with session.get(url) as response:
        return await response.text()


async def make_request():
    url = 'http://httpbin.org/get'
    async with aiohttp.ClientSession() as session:
        print(await fetch(session, url))


c = canvas(show_fps=True)
panel = c.scene().panel(controller='axes')
visual = panel.visual('marker')

N = 10_000
pos = nr.randn(N, 3)
ms = nr.uniform(low=2, high=35, size=N)
color_values = nr.rand(N)
color = colormap(color_values, vmin=0, vmax=1, alpha=.75 * np.ones(N), cmap='viridis')

visual.data('pos', pos)
visual.data('color', color)
visual.data('ms', ms)


gui = c.gui("Test GUI")
@gui.control("button", "click here")
def on_change(value):
    print("make HTTP request")
    loop.create_task(make_request())


async def periodic():
    print("start datoviz event loop")
    while next_frame():
        await asyncio.sleep(0.001)


loop = asyncio.get_event_loop()
task = loop.create_task(periodic())


# async def main():
#     async with aiohttp.ClientSession() as session:
#         await asyncio.gather(
#             periodic(),
#             make_request(session),
#         )
# def stop():
#     task.cancel()
# loop.call_later(3, stop)


try:
    loop.run_until_complete(task)
except asyncio.CancelledError:
    pass
