import atexit
import logging
import os.path as op

from IPython.terminal.pt_inputhooks import register

from .pyvisky import App


# Logging
# -------------------------------------------------------------------------------------------------

logger = logging.getLogger(__name__)

# Set a null handler on the root logger
logger = logging.getLogger('visky')
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
_IN_IPYTHON = False


# Globals
# -------------------------------------------------------------------------------------------------

def app(*args, **kwargs):
    global _APP
    if _APP is None:
        _APP = App(*args, **kwargs)
    assert _APP
    return _APP


def canvas(*args, **kwargs):
    return app().canvas(*args, **kwargs)


def run():
    enable_ipython()
    if not _IN_IPYTHON:
        app().run()


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
    global _APP, _EXITING
    if _EXITING:
        return
    if _APP is None:
        logger.debug("automatically creating a Visky app")
        _APP = app()
    assert _APP is not None
    while not context.input_is_ready():
        _APP.run_one_frame()


register('visky', inputhook)


def enable_ipython():
    global _IN_IPYTHON
    if _IN_IPYTHON:
        return
    try:
        # Try to activate the GUI integration only if we are in IPython, and if
        # IPython event loop has not yet been activated.
        from IPython import get_ipython
        ipython = get_ipython()
        if ipython is not None:
            ipython.magic('%gui visky')
            _IN_IPYTHON = True
    except Exception as e:
        logger.debug("Couldn't enable IPython integration: %s" % str(e))
