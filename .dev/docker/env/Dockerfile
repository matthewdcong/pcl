ARG UBUNTU_DISTRO=16.04
FROM ubuntu:${UBUNTU_DISTRO}

ARG VTK_VERSION=6
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
 && apt-get install -y \
      ccache \
      cmake \
      g++ \
      wget \
      libboost-date-time-dev \
      libboost-filesystem-dev \
      libboost-iostreams-dev \
      libeigen3-dev \
      libflann-dev \
      libglew-dev \
      libgtest-dev \
      libopenni-dev \
      libopenni2-dev \
      libproj-dev \
      libqhull-dev \
      libqt5opengl5-dev \
      libusb-1.0-0-dev \
      libvtk${VTK_VERSION}-dev \
      libvtk${VTK_VERSION}-qt-dev \
      qtbase5-dev \
 && rm -rf /var/lib/apt/lists/*

RUN wget -qO- https://github.com/IntelRealSense/librealsense/archive/v2.23.0.tar.gz | tar xz \
 && cd librealsense-2.23.0 \
 && mkdir build \
 && cd build \
 && cmake .. -DBUILD_EXAMPLES=OFF -DBUILD_GRAPHICAL_EXAMPLES=OFF \
 && make -j2 \
 && make install \
 && cd ../.. \
 && rm -rf librealsense-2.23.0

RUN wget -qO ensenso.deb https://download.ensenso.com/s/ensensosdk/download?files=ensenso-sdk-2.2.160-x64.deb \
 && dpkg -i ensenso.deb \
 && rm ensenso.deb
