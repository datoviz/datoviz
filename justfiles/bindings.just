bindings_python := env_var_or_default("PYTHON", if path_exists(".venv/bin/python") == "true" { ".venv/bin/python" } else { "python" })

pydev: # install the Python binding on a development machine
    @{{bindings_python}} -m pip install -r requirements-dev.txt
    @{{bindings_python}} -m pip install -e .
#

python-dev-install: pydev
#

bindings-python-clang: # verify Python libclang is available
    @if ! {{bindings_python}} -c "import clang.cindex" 2>/dev/null; then \
        echo "Missing Python package 'libclang' for {{bindings_python}}." >&2; \
        echo "Run '{{bindings_python}} -m pip install -r requirements-dev.txt' or set PYTHON=/path/to/python." >&2; \
        exit 1; \
    fi
#

api-json: bindings-python-clang
    @{{bindings_python}} tools/bindings/extract_api.py
#

ctypes: api-json
    @{{bindings_python}} tools/bindings/generate_ctypes.py
    @{{bindings_python}} tools/bindings/generate_array_facade.py
#

ctypes-abi: api-json
    @{{bindings_python}} tools/bindings/generate_ctypes_abi.py
#

ctypes-check: api-json ctypes-abi
    @{{bindings_python}} tools/bindings/generate_ctypes.py --check
    @{{bindings_python}} tools/bindings/generate_array_facade.py --check
    @{{bindings_python}} tools/bindings/validate_ctypes_policy.py
    @{{bindings_python}} tools/bindings/validate_array_facade.py
    @PYTHONPATH=. {{bindings_python}} tools/bindings/validate_ctypes_abi.py
#

ctypes-smoke:
    @PYTHONPATH=. {{bindings_python}} tools/bindings/ctypes_smoke.py
#

ctypes-facade-smoke:
    @PYTHONPATH=. {{bindings_python}} tools/bindings/array_facade_smoke.py
#

ctypes-python-smoke:
    @PYTHONPATH=. {{bindings_python}} -m pytest -q testing/test_python_async_helpers.py testing/test_ctypes_raw_smoke.py testing/test_array_facade.py
#

ctypes-render-smoke:
    @PYTHONPATH=. {{bindings_python}} tools/bindings/ctypes_render_smoke.py
#

ctypes-editable-smoke: build ctypes
    @{{bindings_python}} tools/bindings/ctypes_package_smoke.py editable
#

ctypes-wheel-smoke: build ctypes
    @{{bindings_python}} tools/bindings/ctypes_package_smoke.py wheel
#

ctypes-package-smoke: ctypes-editable-smoke ctypes-wheel-smoke
#

c-integration-smoke:
    @tools/c_integration_smoke.sh
#

bindings: build ctypes ctypes-check ctypes-smoke ctypes-facade-smoke ctypes-python-smoke ctypes-render-smoke ctypes-package-smoke
#


# Consistent v0.4 task names.
bindings-api-json:
    @just api-json
#

bindings-struct-check:
    @just checkstructs
#

bindings-check:
    @just ctypes-check
#

bindings-preflight:
    @just bindings
#

pytest:
