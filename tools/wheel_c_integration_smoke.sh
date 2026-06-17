#!/usr/bin/env sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
WORKDIR=${TMPDIR:-/tmp}/datoviz-wheel-c-smoke
STAGE="$WORKDIR/stage"
DIST="$WORKDIR/dist"
TARGET="$WORKDIR/target"
CONSUMER="$WORKDIR/consumer"

rm -rf "$WORKDIR"
mkdir -p "$STAGE/datoviz" "$DIST" "$TARGET" "$CONSUMER"

cd "$ROOT"

cp datoviz/*.py "$STAGE/datoviz/"
cp -a datoviz/experimental "$STAGE/datoviz/"
cp pyproject.toml "$STAGE/"

DVZ_LIB=$(find build -maxdepth 3 \( -name 'libdatoviz.so' -o -name 'libdatoviz.dylib' -o -name 'datoviz.dll' -o -name 'libdatoviz.dll' \) | head -n 1)
if [ -z "$DVZ_LIB" ]; then
    echo "libdatoviz not found under build/; run just build first" >&2
    exit 2
fi
cp "$DVZ_LIB" "$STAGE/datoviz/"

tools/copy_wheel_c_integration.sh "$STAGE/datoviz"

python -m pip wheel "$STAGE" -w "$DIST" --no-deps >/dev/null
python -m pip install "$DIST"/datoviz-*.whl --target "$TARGET" --no-deps >/dev/null

DATOVIZ_CONFIG="$TARGET/bin/datoviz-config"
PYTHONPATH="$TARGET" "$DATOVIZ_CONFIG" --prefix --cflags --libs --cmake-dir

cat > "$CONSUMER/main.c" <<'EOF'
#include <datoviz.h>
int main(void) { return 0; }
EOF

CFLAGS=$(PYTHONPATH="$TARGET" "$DATOVIZ_CONFIG" --cflags)
LIBS=$(PYTHONPATH="$TARGET" "$DATOVIZ_CONFIG" --libs)
cc "$CONSUMER/main.c" $CFLAGS $LIBS -o "$CONSUMER/datoviz_config_consumer"
"$CONSUMER/datoviz_config_consumer"

cat > "$CONSUMER/CMakeLists.txt" <<'EOF'
cmake_minimum_required(VERSION 3.21)
project(datoviz_wheel_consumer C)
find_package(datoviz REQUIRED)
add_executable(datoviz_cmake_consumer main.c)
target_link_libraries(datoviz_cmake_consumer PRIVATE datoviz::datoviz)
EOF

cmake -S "$CONSUMER" -B "$CONSUMER/build" \
    -Ddatoviz_DIR="$(PYTHONPATH="$TARGET" "$DATOVIZ_CONFIG" --cmake-dir)" \
    -GNinja >/dev/null
cmake --build "$CONSUMER/build" >/dev/null
"$CONSUMER/build/datoviz_cmake_consumer"

echo "Wheel C integration smoke passed: $WORKDIR"
