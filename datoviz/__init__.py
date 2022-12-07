import asyncio
import atexit
import logging
import os
import os.path as op
import sys
import time
import __main__ as main

from IPython import get_ipython
from IPython.terminal.pt_inputhooks import register

try:
    from .requester import Requester
    from .renderer import Renderer
except ImportError:
    raise ImportError(
        "Unable to load the shared library, make sure to run in your terminal:\n"
        "`export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$(pwd)/build`")
    exit(1)


# Logging
# -------------------------------------------------------------------------------------------------

# Set a null handler on the root logger
logger = logging.getLogger('datoviz')
logger.setLevel('DEBUG')

_logger_fmt = '%(asctime)s.%(msecs)03d py %(levelname)s %(caller)s %(message)s'
_logger_date_fmt = '%H:%M:%S'


class _Formatter(logging.Formatter):
    def format(self, record):
        # Only keep the first character in the level name.
        record.levelname = record.levelname[0]
        filename = op.splitext(op.basename(record.pathname))[0]
        record.caller = '{:>18s}:{:04d}:'.format(
            filename, record.lineno).ljust(22)
        message = super(_Formatter, self).format(record)
        color_code = {'D': '90', 'I': '0', 'W': '33',
                      'E': '31'}.get(record.levelname, '7')
        message = '\33[%sm%s\33[0m' % (color_code, message)
        return message


def add_default_handler(level='INFO', logger=logger):
    handler = logging.StreamHandler()
    handler.setLevel(level)

    formatter = _Formatter(fmt=_logger_fmt, datefmt=_logger_date_fmt)
    handler.setFormatter(formatter)

    logger.addHandler(handler)


# DEBUG
add_default_handler('DEBUG')


# Globals
# -------------------------------------------------------------------------------------------------
