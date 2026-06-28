pydev: # install the Python binding on a development machine
    @pip install -r requirements-dev.txt
    @pip install -e .
#

python-dev-install: pydev
#

bindings-python-clang: # install Python libclang when clang.cindex is missing
    @python -c "import clang.cindex" 2>/dev/null || python -m pip install libclang
#

api-json: bindings-python-clang
    @python tools/bindings/extract_api.py
#

ctypes: api-json
    @python tools/bindings/generate_ctypes.py
    @python tools/bindings/generate_array_facade.py
#

ctypes-abi: api-json
    @python tools/bindings/generate_ctypes_abi.py
#

ctypes-check: api-json ctypes-abi
    @python tools/bindings/generate_ctypes.py --check
    @python tools/bindings/generate_array_facade.py --check
    @python tools/bindings/validate_ctypes_policy.py
    @python tools/bindings/validate_array_facade.py
    @PYTHONPATH=. python tools/bindings/validate_ctypes_abi.py
#

ctypes-smoke:
    @PYTHONPATH=. python tools/bindings/ctypes_smoke.py
#

ctypes-facade-smoke:
    @PYTHONPATH=. python tools/bindings/array_facade_smoke.py
#

ctypes-python-smoke:
    @PYTHONPATH=. pytest -q testing/test_python_async_helpers.py testing/test_ctypes_raw_smoke.py testing/test_array_facade.py
#

ctypes-render-smoke:
    @PYTHONPATH=. python tools/bindings/ctypes_render_smoke.py
#

ctypes-editable-smoke: build ctypes
    @python tools/bindings/ctypes_package_smoke.py editable
#

ctypes-wheel-smoke: build ctypes
    @python tools/bindings/ctypes_package_smoke.py wheel
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
