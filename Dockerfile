# syntax=docker/dockerfile:1
FROM ubuntu:22.04

# setup
RUN apt-get update
RUN DEBIAN_FRONTEND=noninteractive apt-get install -y \
    # python base \
    python3 python3-pip python3-venv \
    # python deps \
    python3-argcomplete \
    # utilities \
    git wget curl binutils file \
    # build core \
    build-essential ninja-build cargo rustc \
    # qemu required \
    libglib2.0-dev \
    # virtme required \
    iproute2 systemd udev busybox-static

RUN git clone --recurse-submodules \
    https://github.com/arighi/virtme-ng.git
RUN cd virtme-ng && git checkout v1.23 && make
