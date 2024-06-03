FROM ubuntu:22.04

# setup
RUN apt-get update
RUN DEBIAN_FRONTEND=noninteractive apt-get upgrade -y
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
    cpio busybox-static systemd udev iproute2 \
    # guestfs \
    libguestfs-tools python3-guestfs linux-image-generic-hwe-22.04

# rust
RUN curl --proto '=https' --tlsv1.2 -sSf \
    https://sh.rustup.rs | bash -s -- -y
ENV PATH="/root/.cargo/bin:${PATH}"

# virtme
RUN git clone --recurse-submodules \
    https://github.com/arighi/virtme-ng.git
RUN cd virtme-ng && git checkout v1.24 && make

# fuzzer
RUN cargo install cargo-fuzz
RUN rustup default nightly
