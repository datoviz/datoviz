#!/usr/bin/env bash
set -euxo pipefail

PIP_USER=false "$PYTHON" -m pip install . --no-deps --no-build-isolation -vv
