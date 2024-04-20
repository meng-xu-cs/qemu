# syntax=docker/dockerfile:1
FROM ubuntu:24.04

# setup: apt
RUN apt-get update
RUN apt-get upgrade -y

# setup: managed
RUN DEBIAN_FRONTEND=noninteractive apt-get install -y \
    # python base \
    python3 python3-pip python3-venv \
    # python deps \
    python3-argcomplete \
    # utilities \
    git wget curl binutils file \
    # build core \
    build-essential ninja-build \
    # qemu required \
    libglib2.0-dev \
    # virtme required \
    cpio busybox-static systemd udev iproute2

# setup: rust
RUN curl -sSf https://sh.rustup.rs | bash -s -- -y
ENV PATH="/root/.cargo/bin:${PATH}"

# tweak
RUN git config --global --add safe.directory '*'
