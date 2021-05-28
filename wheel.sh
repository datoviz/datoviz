# Building a manylinux wheel for datoviz
#
# CentOS
# yum install libXrandr-devel libXinerama-devel libXcursor-devel libXi-devel libXcursor-devel vulkan-devel
#
# Debian
# This script is to run in a Debian-based manylinux docker image: manylinux_2_24_x86_64
#
# See manage.sh wheel to see how to build the wheel and how to use this script within a docker image.


# Variables.
PYTHON='/opt/python/cp38-cp38/bin/python3'
PIP='/opt/python/cp38-cp38/bin/pip'
AUDITWHEEL='/root/.local/bin/auditwheel'

# Host directory (mounted).
ROOT_DIR="/io"
BUILD_DIR="$ROOT_DIR/build_wheel"
CYTHON_DIR="$ROOT_DIR/bindings/cython"
OUT_DIR="$CYTHON_DIR/dist"

# Container directory.
TMP_DIR="/tmp/datoviz"
TMP_BUILD="$TMP_DIR/build"
TMP_CYTHON="$TMP_DIR/bindings/cython"

# Ensure the directories exist.
mkdir -p $TMP_DIR
mkdir -p $TMP_CYTHON
mkdir -p $OUT_DIR


# Build the library in build_wheel/
cd $ROOT_DIR
rm -rf $BUILD_DIR && mkdir -p $BUILD_DIR
cd $BUILD_DIR && pwd
cmake .. -GNinja -DDATOVIZ_DOWNLOAD_GLFW=1 && ninja && cd ..


# Copy the Cython bindings to the container.
# NOTE: we keep a hierarchy of folders similar to the root directory because setup.py
# assumes that include/ is two directories up in the file system.
# - /tmp
# |-- include/
# |-- bindings/
# |----- cython/
# Copy the various headers.
cp -r "$ROOT_DIR/include" "$ROOT_DIR/external" $TMP_DIR
# Copy the build_wheel dir into tmp/datoviz/build so that the Cython builder can pick it.
# NOTE: the glfw headers are in there (CMake dependency).
rm -rf $TMP_BUILD && cp -r $BUILD_DIR $TMP_BUILD
# Copy the Cython sources.
cd $CYTHON_DIR
cp -r *.* datoviz $TMP_CYTHON


# Build the Cython extension module.
cd "$TMP_CYTHON"
$PYTHON setup.py build_ext -i
# Build the Cython bindings and make the wheel.
$PYTHON setup.py sdist bdist_wheel


# Backup the wheel before repairing it.
FILENAME=`ls dist/datoviz*.whl`
cp $FILENAME "$FILENAME~"


# Repair the wheel.
# NOTE: we only include libdatoviz
LD_LIBRARY_PATH=$BUILD_DIR $AUDITWHEEL repair $FILENAME \
    --include libdatoviz,libcglm -w dist/
mkdir -p $OUT_DIR
cp `ls dist/datoviz*manylinux*.whl` $OUT_DIR


# Leftovers:

# export VULKAN_SDK=/opt/vulkan/${VULKAN_SDK_VERSION}/x86_64
# export PATH="$VULKAN_SDK/bin:$PATH"
# LD_LIBRARY_PATH="$VULKAN_SDK/lib:${LD_LIBRARY_PATH:-}"
# VK_LAYER_PATH="$VULKAN_SDK/etc/vulkan/explicit_layer.d"

# cd /io/
# apt update
# apt install -y wget libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libxcursor-dev ninja-build

# VULKAN_SDK_VERSION=1.2.170.0
# PYTHON='/opt/python/cp38-cp38/bin/python3'
# PIP='/opt/python/cp38-cp38/bin/pip'

# $PYTHON -m pip install --upgrade pip
# $PIP install -r requirements-build.txt
# $PIP uninstall auditwheel -y
# $PIP install git+https://github.com/rossant/auditwheel.git@include-exclude --user
# AUDITWHEEL='/root/.local/bin/auditwheel'

# wget -q --show-progress --progress=bar:force:noscroll \
#     "https://sdk.lunarg.com/sdk/download/$VULKAN_SDK_VERSION/linux/vulkansdk-linux-x86_64-$VULKAN_SDK_VERSION.tar.gz?Human=true" \
#     -O /tmp/vulkansdk-linux-x86_64-$VULKAN_SDK_VERSION.tar.gz
# mkdir -p /opt/vulkan
# tar -xf /tmp/vulkansdk-linux-x86_64-$VULKAN_SDK_VERSION.tar.gz -C /opt/vulkan
# rm /tmp/vulkansdk-linux-x86_64-$VULKAN_SDK_VERSION.tar.gz
