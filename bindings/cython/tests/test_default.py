from pytest import fixture

from datoviz import App, app, canvas



def clear_loggers():
    """Remove handlers from all loggers"""
    import logging
    loggers = [logging.getLogger()] + list(logging.Logger.manager.loggerDict.values())
    for logger in loggers:
        handlers = getattr(logger, 'handlers', [])
        for handler in handlers:
            logger.removeHandler(handler)


def test_app():
    a = App()
    a.destroy()



def test_canvas():
    c = canvas()


def teardown():
    # HACK: fixes pytest bug
    # see https://github.com/pytest-dev/pytest/issues/5502#issuecomment-647157873
    clear_loggers()
