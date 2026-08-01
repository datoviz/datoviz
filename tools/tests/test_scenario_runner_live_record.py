"""Regression contracts for interactive scenario recording."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / 'examples/c/runner/scenario_runner.c'


def _function_body(source: str, signature: str) -> str:
    """Return one C function body using balanced braces."""
    start = source.index(signature)
    opening = source.index('{', start)
    depth = 0
    for index in range(opening, len(source)):
        if source[index] == '{':
            depth += 1
        elif source[index] == '}':
            depth -= 1
            if depth == 0:
                return source[opening + 1 : index]
    raise AssertionError(f'unterminated C function: {signature}')


def test_paced_live_record_stops_after_window_close() -> None:
    """The paced loop must observe a close immediately after polling and rendering."""
    source = SOURCE.read_text(encoding='utf-8')
    body = _function_body(source, 'static void _run_paced(')

    render = body.index('dvz_app_run(app, 1);')
    close = body.index('if (dvz_app_should_exit(app))', render)
    schedule = body.index('const uint64_t after', render)

    assert render < close < schedule


def test_live_record_restores_glfw_controller_input() -> None:
    """Creating the offscreen capture view must not retain the panel input subscription."""
    source = SOURCE.read_text(encoding='utf-8')
    body = _function_body(source, 'int dvz_scenario_run_native(')

    capture_view = body.index('capture_view = dvz_view(app, ctx.figure, &desc);')
    reconnect = body.index('_connect_controller_bindings(&ctx, view)', capture_view)
    capture_start = body.index('dvz_view_capture_start(capture_view, &resolved.capture)', reconnect)

    assert capture_view < reconnect < capture_start
