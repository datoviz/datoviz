
# -------------------------------------------------------------------------------------------------
# Imports
# -------------------------------------------------------------------------------------------------

import logging
import os
from pathlib import Path
import shutil

import numpy as np
import imageio

from datoviz import Requester


ROOT_PATH = Path(__file__).resolve().parent.parent
IMAGES_PATH = ROOT_PATH / 'images'
SCREENSHOTS_PATH = ROOT_PATH / 'data/screenshots'


logger = logging.getLogger('datoviz')


# -------------------------------------------------------------------------------------------------
# Util functions
# -------------------------------------------------------------------------------------------------

def check_screenshot(filename):
    """Compare a new screenshot with the reference image."""

    assert filename.exists
    filename_ref = filename.with_suffix('').with_suffix('').with_suffix('.png')
    if not filename_ref.exists():
        logger.debug(
            f"Reference image {filename_ref} didn't exist, skipping image check.")
        shutil.copy(filename, filename_ref)
        return True
    img_new = imageio.imread(filename)
    if img_new.sum() == 0:
        logger.warning("Screenshot is empty")
        return False
    img_ref = imageio.imread(filename_ref)
    if img_new.shape != img_ref.shape:
        logger.debug(
            f"Image size is different: {img_new.shape} != {img_ref.shape}")
        return False
    return np.all(img_new == img_ref)


# def check_canvas(ca, test_name, output_dir=None):
#     """Run a canvas, make a screenshot, and check it with respect to the reference image."""

#     output_dir = output_dir or IMAGES_PATH
#     if not output_dir.exists():
#         output_dir.mkdir(exist_ok=True, parents=True)
#     screenshot = output_dir / f'{test_name}.new.png'

#     # Interactive mode if debug.
#     debug = os.environ.get('DVZ_DEBUG', None)
#     if debug:
#         app().run()
#         ca.close()
#         return

#     # Run and save the screenshot.
#     # app().run(10, screenshot=str(screenshot))
#     app().run(n_frames=5)
#     ca.screenshot(str(screenshot))
#     ca.close()

#     # Check the screenshot.
#     res = check_screenshot(screenshot)
#     assert res, f"Screenshot check failed for {test_name}"

#     # Delete the new screenshot if it matched the reference image.
#     if res:
#         logger.debug(f"Screenshot check succeedeed for {test_name}")
#         os.remove(screenshot)
