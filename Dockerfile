# syntax=docker/dockerfile:1
FROM --platform=linux/amd64 ubuntu:22.04

# setup
RUN apt-get update
RUN DEBIAN_FRONTEND=noninteractive apt-get install -y \
    python3 python3-venv \
    python3-sphinx python3-sphinx-rtd-theme \
    git wget curl binutils \
    build-essential ninja-build \
    libglib2.0-dev \
    busybox-static

