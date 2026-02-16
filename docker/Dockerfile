FROM ubuntu:24.04

ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
  build-essential \
  ca-certificates \
  cmake \
  git \
  libcurl4-openssl-dev \
  libgl1-mesa-dev \
  libglu1-mesa-dev \
  libgtk-3-dev \
  libogg-dev \
  libopus-dev \
  libphysfs-dev \
  libpng-dev \
  libssl-dev \
  libsdl2-dev \
  libsdl2-image-dev \
  libsdl2-net-dev \
  libsdl2-ttf-dev \
  libtheora-dev \
  libvorbis-dev \
  ninja-build \
  pkg-config \
  rapidjson-dev \
  zlib1g-dev \
  && rm -rf /var/lib/apt/lists/*

# Build nativefiledialog-extended into /opt/nfd for FindNFD.cmake.
RUN git clone --depth 1 https://github.com/btzy/nativefiledialog-extended.git /tmp/nfd && \
  cmake -S /tmp/nfd -B /tmp/nfd/build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_SHARED_LIBS=ON \
    -DNFD_BUILD_TESTS=OFF \
    -DNFD_BUILD_SDL2_TESTS=OFF \
    -DNFD_INSTALL=ON \
    -DCMAKE_INSTALL_PREFIX=/opt/nfd && \
  cmake --build /tmp/nfd/build -j"$(nproc)" --target install && \
  rm -rf /tmp/nfd

# Build TheoraPlayer into /opt/theoraplayer for FindTheoraPlayer.cmake.
COPY scripts/theoraplayer/CMakeLists.txt /tmp/theoraplayer-cmake/CMakeLists.txt

RUN git clone --depth 1 https://github.com/AprilAndFriends/theoraplayer.git /tmp/theoraplayer-src

RUN cmake -S /tmp/theoraplayer-cmake -B /tmp/theoraplayer-build -G Ninja \
      -DCMAKE_BUILD_TYPE=Release \
      -DTHEORAPLAYER_ROOT=/tmp/theoraplayer-src \
      -DTHEORAPLAYER_DEP_BACKEND=pkgconfig && \
    cmake --build /tmp/theoraplayer-build -j"$(nproc)" --target theoraplayer && \
    mkdir -p /opt/theoraplayer/include/theoraplayer /opt/theoraplayer/lib && \
    cp -f /tmp/theoraplayer-src/theoraplayer/include/theoraplayer/*.h /opt/theoraplayer/include/theoraplayer/ && \
    cp -f /tmp/theoraplayer-build/libtheoraplayer.so /opt/theoraplayer/lib/ && \
    rm -rf /tmp/theoraplayer-src /tmp/theoraplayer-cmake /tmp/theoraplayer-build

ENV NFD_DIR=/opt/nfd
ENV THEORAPLAYER_DIR=/opt/theoraplayer

WORKDIR /work

CMD ["bash"]
