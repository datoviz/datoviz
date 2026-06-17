#!/usr/bin/env bash
set -euxo pipefail

"$PYTHON" -m pip install . --no-deps --no-build-isolation --ignore-installed -vv
