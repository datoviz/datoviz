
# -------------------------------------------------------------------------------------------------
# Imports
# -------------------------------------------------------------------------------------------------

import logging
import os
from pathlib import Path

import numpy as np
from numpy.testing import assert_array_equal as ae
import numpy.random as nr
import imageio

from datoviz import Requester
from .utils import ROOT_PATH

logger = logging.getLogger('datoviz')

CUR_DIR = Path(__file__).parent


# -------------------------------------------------------------------------------------------------
# Tests
# -------------------------------------------------------------------------------------------------

def test_request_1():
    r = Requester()
