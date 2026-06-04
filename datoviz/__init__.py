"""Datoviz Python package entry point."""


__all__ = ['Host']


def _facade():
    try:
        from . import _array_facade
    except ModuleNotFoundError as exc:
        package = __package__ or __name__
        if exc.name != f'{package}._array_facade':
            raise
        raise RuntimeError(
            'Datoviz array-aware facade has not been generated. Run `just ctypes` from the '
            'repository root, or install a package that includes datoviz/_array_facade.py.'
        ) from exc

    return _array_facade


def __getattr__(name):
    if name == 'Host':
        from .host import Host

        return Host
    if name.startswith(('dvz_', 'Dvz')):
        facade = _facade()
        if hasattr(facade, name):
            return getattr(facade, name)
    raise AttributeError(f'module {__name__!r} has no attribute {name!r}')


def __dir__():
    names = set(globals()) | {'Host'}
    try:
        names.update(getattr(_facade(), '__all__', []))
    except Exception:
        pass
    return sorted(names)
