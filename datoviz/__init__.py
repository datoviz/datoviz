"""Datoviz Python package entry point."""


__all__ = ['Host']


def __getattr__(name):
    if name == 'Host':
        from .host import Host

        return Host
    raise AttributeError(f'module {__name__!r} has no attribute {name!r}')
