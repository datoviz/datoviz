import atexit

from IPython.terminal.pt_inputhooks import register

from visky.pyvisky import App


_APP = None
_EXITING = False


def app(*args, **kwargs):
    global _APP
    if _APP is None:
        _APP = App(*args, **kwargs)
    assert _APP
    return _APP


def canvas(*args, **kwargs):
    return app().canvas(*args, **kwargs)


def run():
    app().run()


@atexit.register
def destroy():
    global _APP, _EXITING
    _EXITING = True
    print("EXITING")
    if _APP:
        del _APP
    _APP = None


def inputhook(context):
    global _APP, _EXITING
    if _EXITING:
        print("SKIP EXIT")
        return
    if _APP is None:
        print("CREATE APP")
        _APP = app()
        _APP.run_begin()
    # print("HOOK")
    assert _APP is not None
    while not context.input_is_ready():
        _APP.run_process()


register('visky', inputhook)


def enable_ipython():
    try:
        from IPython import get_ipython
        ipython = get_ipython()
        ipython.magic('%gui visky')
    except Exception as e:
        print("Couldn't enable IPython integration: %s" % str(e))
