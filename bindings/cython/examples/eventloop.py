"""
# Asynchronous visual data update

This example shows how to make asynchronous updates to the data by using the asyncio event loop.

There is only one thread. With the asyncio event loop, the event loop is running in Python
instead of C. We can use a special HTTP request library in Python, aiohttp, that fully supports
asyncio so that we can make asynchronous HTTP requests and avoid blocking the UI while waiting for
the server's response. When the response is ready, we update the visual's data.

"""
import asyncio
import json
from pathlib import Path

import aiohttp
import numpy as np
import numpy.random as nr
from datoviz import app, canvas, run, colormap, do_async


async def make_request(url):
    """Make an asynchronous HTTP GET request and return the response as text."""
    async with aiohttp.ClientSession() as session:
        async with session.get(url) as response:
            return(await response.text())


def update_data(N=10_000):
    """Update the visual's data with a given number of points."""
    pos = nr.randn(N, 3)
    ms = nr.uniform(low=2, high=35, size=N)
    color_values = nr.rand(N)
    color = colormap(color_values, vmin=0, vmax=1, alpha=.75 * np.ones(N), cmap='viridis')

    visual.data('pos', pos)
    visual.data('color', color)
    visual.data('ms', ms)


# Make the canvas and visual.
c = canvas(show_fps=True)
panel = c.scene().panel(controller='axes')
visual = panel.visual('marker')
update_data()


async def update():
    """This callback function is called asynchronously when clicking on a button.

    It makes an asynchronous HTTP GET request to a website generating random numbers.
    When the reply is ready, we update the visual with the returned number of points.

    """
    print("Making HTTP request to get random number...")
    text = await make_request('https://random-data-api.com/api/number/random_number')
    n = int(json.loads(text)['positive'])
    print(f"Show {n} points")
    update_data(n)


# GUI
gui = c.gui("Test GUI")

button = gui.control("button", "update the visual")

@button.connect
def on_change(value):
    # The special do_async() function, provided by datoviz, takes an asynchronous task and runs it
    # in the asyncio event loop.
    do_async(update())

# Start the asyncio event loop.
run(event_loop='asyncio')
