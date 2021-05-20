import logging
import os
from pathlib import Path
import shutil

import numpy as np
import imageio

from datoviz import app, canvas


ROOT_PATH = Path(__file__).resolve().parent.parent.parent.parent
CYTHON_PATH = Path(__file__).resolve().parent.parent
IMAGES_PATH = CYTHON_PATH / 'images'


logger = logging.getLogger('datoviz')


def check_screenshot(filename):
    """Compare a new screenshot with the reference image."""

    assert filename.exists
    filename_ref = filename.with_suffix('').with_suffix('').with_suffix('.png')
    if not filename_ref.exists():
        logger.debug(f"Reference image {filename_ref} didn't exist, skipping image check.")
        shutil.copy(filename, filename_ref)
        return True
    img_new = imageio.imread(filename)
    img_ref = imageio.imread(filename_ref)
    if img_new.shape != img_ref.shape:
        logger.debug(f"Image size is different: {img_new.shape} != {img_ref.shape}")
        return False
    return np.all(img_new == img_ref)


def check_canvas(ca, test_name):
    """Run a canvas, make a screenshot, and check it with respect to the reference image."""

    if not IMAGES_PATH.exists():
        IMAGES_PATH.mkdir(exist_ok=True, parents=True)
    screenshot = IMAGES_PATH / f'{test_name}.new.png'

    # Interactive mode if debug.
    debug = os.environ.get('DVZ_DEBUG', None)
    if debug:
        app().run()
        ca.close()
        return

    # Run and save the screenshot.
    app().run(10, screenshot=str(screenshot))

    # Close the canvas.
    ca.close()

    # Check the screenshot.
    res = check_screenshot(screenshot)
    assert res, f"Screenshot check failed for {test_name}"

    # Delete the new screenshot if it matched the reference image.
    if res:
        logger.debug(f"Screenshot check succeedeed for {test_name}")
        os.remove(screenshot)
