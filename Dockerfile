# NOTE: this dockerfile compiles but doesn't work yet
# TODO: start from an nvidia docker instead

FROM ubuntu:20.04

LABEL maintainer "Datoviz Development Team"

# Install the build and lib dependencies, install vulkan and a recent version of CMake
RUN \
    apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y tzdata && \
    apt-get install -y build-essential git cmake wget ninja-build xcb libx11-xcb-dev \
    libxcb-glx0 libglfw3-dev libpng-dev libavcodec-dev libavformat-dev libavfilter-dev \
    libavutil-dev libswresample-dev libvncserver-dev xtightvncviewer libqt5opengl5-dev \
    libfreetype6-dev libassimp-dev \
    python3-dev python3-pip python3-numpy cython3

# install vulkan sdk
ENV VULKAN_SDK_VERSION=1.2.148.1
RUN echo "downloading Vulkan SDK $VULKAN_SDK_VERSION" \
    && wget -q --show-progress --progress=bar:force:noscroll \
    "https://sdk.lunarg.com/sdk/download/$VULKAN_SDK_VERSION/linux/vulkansdk-linux-x86_64-$VULKAN_SDK_VERSION.tar.gz?Human=true" \
    -O /tmp/vulkansdk-linux-x86_64-$VULKAN_SDK_VERSION.tar.gz \
    && echo "installing Vulkan SDK $VULKAN_SDK_VERSION" \
    && mkdir -p /opt/vulkan \
    && tar -xf /tmp/vulkansdk-linux-x86_64-$VULKAN_SDK_VERSION.tar.gz -C /opt/vulkan \
    &&  rm /tmp/vulkansdk-linux-x86_64-$VULKAN_SDK_VERSION.tar.gz
ENV VULKAN_SDK=/opt/vulkan/${VULKAN_SDK_VERSION}/x86_64
ENV PATH="$VULKAN_SDK/bin:$PATH" \
    LD_LIBRARY_PATH="$VULKAN_SDK/lib:${LD_LIBRARY_PATH:-}" \
    VK_LAYER_PATH="$VULKAN_SDK/etc/vulkan/explicit_layer.d"

# Python dependencies
RUN pip3 install pyparsing scikit-build colorcet imageio

# User and group
RUN \
    groupadd -g 1000 datoviz && \
    useradd -d /home/datoviz -s /bin/bash -m datoviz -u 1000 -g 1000 && \
    chown -R datoviz:datoviz /datoviz
USER datoviz
ENV HOME /home/datoviz
