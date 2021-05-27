# Building a manylinux wheel for datoviz

# CentOS
# yum install libXrandr-devel libXinerama-devel libXcursor-devel libXi-devel libXcursor-devel vulkan-devel

# Debian

# This script is to run in a Debian-based manylinux docker image: manylinux_2_24_x86_64
# Run as follows:
#
# export DOCKER_IMAGE=quay.io/pypa/manylinux_2_24_x86_64
# sudo docker pull $DOCKER_IMAGE
# sudo docker run --rm -v `pwd`:/io $DOCKER_IMAGE /io/make-wheel.sh

cd /io/
apt update
apt install -y wget libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libxcursor-dev ninja-build


VULKAN_SDK_VERSION=1.2.170.0
PYTHON='/opt/python/cp38-cp38/bin/python3'
PIP='/opt/python/cp38-cp38/bin/pip'

$PYTHON -m pip install --upgrade pip
$PIP install -r requirements-build.txt
$PIP uninstall auditwheel -y
$PIP install git+https://github.com/rossant/auditwheel.git@include-exclude --user
AUDITWHEEL='/root/.local/bin/auditwheel'

wget -q --show-progress --progress=bar:force:noscroll \
    "https://sdk.lunarg.com/sdk/download/$VULKAN_SDK_VERSION/linux/vulkansdk-linux-x86_64-$VULKAN_SDK_VERSION.tar.gz?Human=true" \
    -O /tmp/vulkansdk-linux-x86_64-$VULKAN_SDK_VERSION.tar.gz
mkdir -p /opt/vulkan
tar -xf /tmp/vulkansdk-linux-x86_64-$VULKAN_SDK_VERSION.tar.gz -C /opt/vulkan
rm /tmp/vulkansdk-linux-x86_64-$VULKAN_SDK_VERSION.tar.gz

source /opt/vulkan/setup-env.sh
# export VULKAN_SDK=/opt/vulkan/${VULKAN_SDK_VERSION}/x86_64
# export PATH="$VULKAN_SDK/bin:$PATH"
# LD_LIBRARY_PATH="$VULKAN_SDK/lib:${LD_LIBRARY_PATH:-}"
# VK_LAYER_PATH="$VULKAN_SDK/etc/vulkan/explicit_layer.d"

# Build the library.
mkdir -p build_wheel
cd build_wheel
cmake .. -GNinja
ninja
cd ..

# Build the Cython extension module.
$PYTHON utils/generate_cython.py
cd bindings/cython
rm -rf build dist datoviz.egg-info
$PYTHON setup.py build_ext -i
# Build the wheel.
$PYTHON setup.py sdist bdist_wheel

# Repair the wheel.
mkdir -p dist/backup
cp dist/datoviz*.whl dist/backup
LD_LIBRARY_PATH=/io/build $AUDITWHEEL repair dist/datoviz*.whl --include libdatoviz -w dist/
