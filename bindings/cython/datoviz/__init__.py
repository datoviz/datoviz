import atexit
import logging
import os.path as op
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

logger = logging.getLogger(__name__)

# Set a null handler on the root logger
logger = logging.getLogger('datoviz')
logger.setLevel(logging.DEBUG)
logger.addHandler(logging.NullHandler())

_logger_fmt = '%(asctime)s.%(msecs)03d [%(levelname)s] %(caller)s %(message)s'
_logger_date_fmt = '%H:%M:%S'


class _Formatter(logging.Formatter):
    def format(self, record):
        # Only keep the first character in the level name.
        record.levelname = record.levelname[0]
        filename = op.splitext(op.basename(record.pathname))[0]
        record.caller = '{:s}:{:d}'.format(filename, record.lineno).ljust(20)
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


add_default_handler('DEBUG')


# Globals
# -------------------------------------------------------------------------------------------------

_APP = None
_EXITING = False
_EVENT_LOOP_INTEGRATION = False
# _IN_IPYTHON = False


# Globals
# -------------------------------------------------------------------------------------------------

def app(*args, **kwargs):
    global _APP
    if _APP is None:
        _APP = App(*args, **kwargs)
    assert _APP
    return _APP


def canvas(*args, **kwargs):
    c = app().canvas(*args, **kwargs)
    # enable_ipython()
    return c


def custom_colormap(*args, **kwargs):
    return app().context().colormap(*args, **kwargs)


def context():
    return app().context()


def run(**kwargs):
    global _EVENT_LOOP_INTEGRATION
    if _EVENT_LOOP_INTEGRATION:
        return
    # enable_ipython()
    # if not _IN_IPYTHON:
    app().run(**kwargs)


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

def in_ipython():
    try:
        return __IPYTHON__
    except NameError:
        return False


def is_interactive():
    return not hasattr(main, '__file__')


def inputhook(context):
    global _APP, _EXITING, _EVENT_LOOP_INTEGRATION
    if _EXITING:
        return
    if _APP is None:
        logger.debug("automatically creating a Datoviz app")
        _APP = app()
    assert _APP is not None
    _EVENT_LOOP_INTEGRATION = True
    while not context.input_is_ready():
        _APP.run_one_frame()


def enable_ipython():
    ipython = get_ipython()
    if ipython is not None:
        ipython.magic('%gui datoviz')


# _IN_IPYTHON = in_ipython()
register('datoviz', inputhook)

# print(is_interactive())

# def enable_ipython():
#     ipython = get_ipython()
#     if ipython is not None:
#         ipython.magic('%gui datoviz')
#     global _IN_IPYTHON
#     if _IN_IPYTHON:
#         return
#     try:
#         # Try to activate the GUI integration only if we are in IPython, and if
#         # IPython event loop has not yet been activated.
#         from IPython import get_ipython
#         ipython = get_ipython()
#         if ipython is not None:
#             ipython.magic('%gui datoviz')
#             print("IPython")
#             _IN_IPYTHON = True
#     except Exception as e:
#         logger.debug("Couldn't enable IPython integration: %s" % str(e))


# enable_ipython()
