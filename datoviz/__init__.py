"""Datoviz Python package entry point."""

from .run import run

__all__ = ['Host', 'run', 'capture']


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


def capture(scene, figure, path="output.png", width=800, height=600):
    """Render one offscreen frame, save it to a PNG, then destroy the app and scene."""
    from . import raw as _raw
    path_bytes = path.encode() if isinstance(path, str) else path
    app = _raw.dvz_app(scene)
    view = _raw.dvz_view_offscreen(app, figure, width, height)
    _raw.dvz_app_run(app, 1)
    _raw.dvz_view_capture_png(view, path_bytes)
    _raw.dvz_app_destroy(app)
    _raw.dvz_scene_destroy(scene)


def __getattr__(name):
    if name == 'Host':
        from .host import Host

        return Host
    if name.startswith(('dvz_', 'Dvz', 'DVZ_')):
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
