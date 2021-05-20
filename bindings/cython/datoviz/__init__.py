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
    from .pydatoviz import App, colormap
except ImportError:
    raise ImportError(
        "Unable to load the shared library, make sure to run in your terminal:\n"
        "`export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$(pwd)/build`")
    exit(1)


# Logging
# -------------------------------------------------------------------------------------------------

# Set a null handler on the root logger
logger = logging.getLogger('datoviz')
logger.setLevel(logging.DEBUG)
logger.addHandler(logging.NullHandler())

_logger_fmt = '%(asctime)s.%(msecs)03d %(levelname)s %(caller)s %(message)s'
_logger_date_fmt = '%H:%M:%S'


class _Formatter(logging.Formatter):
    def format(self, record):
        # Only keep the first character in the level name.
        record.levelname = record.levelname[0]
        filename = op.splitext(op.basename(record.pathname))[0]
        record.caller = '{:>18s}:{:04d}:'.format(filename, record.lineno).ljust(22)
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

_APP = None
_EXITING = False
_EVENT_LOOP_INTEGRATION = False
_ASYNCIO_LOOP = None


# Main functions
# -------------------------------------------------------------------------------------------------

def app(*args, **kwargs):
    global _APP
    if _APP is None:
        _APP = App(*args, **kwargs)
    assert _APP
    return _APP


def canvas(*args, **kwargs):
    return app().gpu().canvas(*args, **kwargs)


@atexit.register
def destroy():
    global _APP, _EXITING
    _EXITING = True
    if _APP:
        logger.debug("destroying the app now")
        _APP.destroy()
    _APP = None


# IPython event loop integration
# -------------------------------------------------------------------------------------------------

def inputhook(context):
    global _APP, _EXITING, _EVENT_LOOP_INTEGRATION
    if _EXITING:
        return
    if _APP is None:
        logger.debug("automatically creating a Datoviz app")
        _APP = app()
    assert _APP
    _EVENT_LOOP_INTEGRATION = True
    while not context.input_is_ready():
        _APP.next_frame()
        # HACK: prevent the app.is_running flag to be reset to False at the end of next_frame()
        _APP._set_running(True)
        time.sleep(0.005)


def enable_ipython():
    ipython = get_ipython()
    if ipython:
        logger.info("Enabling Datoviz IPython event loop integration")
        app()._set_running(True)
        ipython.magic('%gui datoviz')


def in_ipython():
    try:
        return __IPYTHON__
    except NameError:
        return False


def is_interactive():
    if not in_ipython():
        return hasattr(sys, 'ps1')
    else:
        if '-i' in sys.argv:
            return True
        # return sys.__stdin__.isatty()
        # return hasattr(sys, 'ps1')
        return not hasattr(main, '__file__')


# print(f"In IPython: {in_ipython()}, is interactive: {is_interactive()}")
register('datoviz', inputhook)


# Event loops
# -------------------------------------------------------------------------------------------------

def run_asyncio(n_frames=0, **kwargs):
    # TODO: support kwargs options (autorun)

    global _ASYNCIO_LOOP
    if _ASYNCIO_LOOP is None:
        _ASYNCIO_LOOP = asyncio.get_event_loop()

    async def _event_loop():
        logger.debug("start datoviz asyncio event loop")
        i = 0
        while app().next_frame() and (n_frames == 0 or i < n_frames):
            await asyncio.sleep(0.005)
            i += 1

    task = _ASYNCIO_LOOP.create_task(_event_loop())

    try:
        _ASYNCIO_LOOP.run_until_complete(task)
    except asyncio.CancelledError:
        pass


def do_async(task):
    global _ASYNCIO_LOOP
    if _ASYNCIO_LOOP is None:
        _ASYNCIO_LOOP = asyncio.get_event_loop()
    _ASYNCIO_LOOP.create_task(task)


def run_native(n_frames=0, **kwargs):
    logger.debug("start datoviz native event loop")
    app().run(n_frames, **kwargs)


def run(n_frames=0, event_loop=None, **kwargs):
    event_loop = event_loop or 'native'
    if event_loop == 'ipython' or is_interactive():
        enable_ipython()
    elif event_loop == 'native':
        run_native(n_frames, **kwargs)
    elif event_loop == 'asyncio':
        run_asyncio(n_frames)
