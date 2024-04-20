#!/usr/bin/env python3

import argparse
import multiprocessing
import os
import shutil
import subprocess
import sys

from pathlib import Path
from typing import List, Optional, Union

from utils import mk_initramfs

# path constants
PATH_REPO = Path(__file__).parent.resolve()
PATH_BUILD = os.path.join(PATH_REPO, "build")
PATH_WKS = os.path.join(PATH_REPO, "workspace")

PATH_WKS_ARTIFACT = os.path.join(PATH_WKS, "artifact")
PATH_WKS_ARTIFACT_BUILD = os.path.join(PATH_WKS_ARTIFACT, "build")
PATH_WKS_ARTIFACT_INSTALL = os.path.join(PATH_WKS_ARTIFACT, "install")
PATH_WKS_ARTIFACT_INSTALL_LIB = os.path.join(
    PATH_WKS_ARTIFACT_INSTALL, "lib", "x86_64-linux-gnu"
)
PATH_WKS_ARTIFACT_INSTALL_QEMU_AMD64 = os.path.join(
    PATH_WKS_ARTIFACT_INSTALL, "bin", "qemu-system-x86_64"
)

PATH_WKS_BUSYBOX = os.path.join(PATH_WKS, "busybox")

PATH_WKS_LINUX = os.path.join(PATH_WKS, "linux")
PATH_WKS_LINUX_BOOT = os.path.join(PATH_WKS_LINUX, "boot")
PATH_WKS_LINUX_BOOT_KERNEL = os.path.join(PATH_WKS_LINUX_BOOT, "kernel.img")
PATH_WKS_LINUX_BOOT_INITRD = os.path.join(PATH_WKS_LINUX_BOOT, "initrd.img")

# docker constants
DOCKER_TAG = "qemu"
DOCKER_SRC_DIR = "/src"
DOCKER_WORKDIR_PREFIX = "/work"

# system constants
NUM_CPUS = multiprocessing.cpu_count()


def _docker_exec(
    tag: str, volumes: List[str], ephemeral: bool, interactive: bool, cmdline: List[str]
) -> None:
    command = ["docker", "run", "--device=/dev/kvm"]

    if ephemeral:
        command.append("--rm")
    if interactive:
        command.extend(["--interactive", "--tty"])

    command.extend(
        [
            "--mount",
            "type=bind,source={},target={}".format(PATH_REPO, DOCKER_SRC_DIR),
        ]
    )
    for i, v in enumerate(volumes):
        command.extend(
            [
                "--mount",
                "type=bind,source={},target={}{},readonly".format(
                    os.path.abspath(v), DOCKER_WORKDIR_PREFIX, i
                ),
            ]
        )

    command.append(tag)
    command.extend(cmdline)

    subprocess.check_call(command)


def cmd_docker_build(force: bool) -> None:
    # TODO: check tag existence
    if force:
        subprocess.check_call(["docker", "image", "rm", DOCKER_TAG])

    subprocess.check_call(["docker", "build", ".", "-t", DOCKER_TAG], cwd=PATH_REPO)


def cmd_docker_shell(volumes: List[str]) -> None:
    _docker_exec(DOCKER_TAG, volumes, True, True, ["bash"])


def cmd_docker_run(volumes: List[str], args: List[str]) -> None:
    _docker_exec(DOCKER_TAG, volumes, True, False, args)


def _qemu_config(
    path_build: Union[str, Path], path_install: Union[str, Path], release: bool
) -> None:
    command = [
        os.path.join(PATH_REPO, "configure"),
        # basics
        "--prefix={}".format(path_install),
        # scopes
        "--target-list=x86_64-softmmu",
        # TODO: fine-tune a list of features
        # "--without-default-features",
        # "--without-default-devices",
        "--disable-docs",
        "--disable-user",
        # features
        "--enable-tcg",
        "--enable-slirp",
    ]

    if release:
        command.extend(
            [
                "--enable-lto",
            ]
        )
    else:
        command.extend(
            [
                "--enable-debug",
            ]
        )

    subprocess.run(command, cwd=path_build, check=True)


def cmd_init(force: bool) -> None:
    # prepare directory
    if os.path.exists(PATH_BUILD):
        if not force:
            sys.exit("Build directory not empty: {}".format(PATH_BUILD))
        shutil.rmtree(PATH_BUILD)

    # build
    _qemu_config(PATH_REPO, os.devnull, False)
    subprocess.run(["make", "-j{}".format(NUM_CPUS)], cwd=PATH_BUILD, check=True)

    # connect the compilation database
    shutil.copy2(
        os.path.join(PATH_BUILD, "compile_commands.json"),
        os.path.join(PATH_REPO, "compile_commands.json"),
    )


def cmd_build(release: bool) -> None:
    # prepare directory
    if os.path.exists(PATH_WKS_ARTIFACT):
        shutil.rmtree(PATH_WKS_ARTIFACT)

    os.makedirs(PATH_WKS_ARTIFACT, exist_ok=False)
    os.mkdir(PATH_WKS_ARTIFACT_BUILD)

    # build
    _qemu_config(PATH_WKS_ARTIFACT_BUILD, PATH_WKS_ARTIFACT_INSTALL, release)
    subprocess.run(
        ["make", "-j{}".format(NUM_CPUS)], cwd=PATH_WKS_ARTIFACT_BUILD, check=True
    )
    subprocess.run(["make", "install"], cwd=PATH_WKS_ARTIFACT_BUILD, check=True)


def _prepare_linux(repo: str, harness: Optional[str], blob: Optional[str]) -> None:
    # clear previous states
    if os.path.exists(PATH_WKS_LINUX):
        shutil.rmtree(PATH_WKS_LINUX)

    os.mkdir(PATH_WKS_LINUX)
    os.mkdir(PATH_WKS_LINUX_BOOT)

    # search for kernel image
    img_kernel = os.path.join(repo, "src", "arch", "x86_64", "boot", "bzImage")
    if not os.path.exists(img_kernel):
        sys.exit("kernel image does not exist: {}".format(img_kernel))
    shutil.copy2(img_kernel, PATH_WKS_LINUX_BOOT_KERNEL)

    # pack initramfs
    mk_initramfs(PATH_WKS_LINUX_BOOT_INITRD, harness, blob)


def cmd_linux(
    repo: str, kvm: bool, harness: Optional[str], blob: Optional[str]
) -> None:
    _prepare_linux(repo, harness, blob)

    command = [PATH_WKS_ARTIFACT_INSTALL_QEMU_AMD64]

    # basics
    command.extend(["-m", "2G"])
    if kvm:
        command.extend(["-machine", "accel=kvm:tcg"])

    # no display
    command.extend(["-vga", "none"])
    command.extend(["-display", "none"])

    # IO
    command.extend(["-serial", "stdio"])
    command.extend(["-parallel", "none"])

    # networking
    command.extend(["-net", "none"])
    command.extend(["-device", "virtio-net-pci,netdev=n0"])
    command.extend(["-netdev", "user,id=n0"])

    # kernel
    command.extend(["-kernel", PATH_WKS_LINUX_BOOT_KERNEL])
    command.extend(["-initrd", PATH_WKS_LINUX_BOOT_INITRD])
    command.extend(["-append", "console=ttyS0"])

    # behaviors
    command.extend(["-no-reboot"])

    # execute
    subprocess.check_call(
        command, env={"LD_LIBRARY_PATH": PATH_WKS_ARTIFACT_INSTALL_LIB}
    )


def main() -> None:
    # args
    parser = argparse.ArgumentParser()
    subparsers = parser.add_subparsers(dest="command")

    #
    # docker group
    #

    parser_docker = subparsers.add_parser("docker")
    sub_docker = parser_docker.add_subparsers(dest="cmd_docker")

    parser_docker_build = sub_docker.add_parser("build")
    parser_docker_build.add_argument("--force", action="store_true")

    parser_docker_shell = sub_docker.add_parser("shell")
    parser_docker_shell.add_argument("-v", "--volume", action="append", default=[])

    parser_docker_run = sub_docker.add_parser("run")
    parser_docker_run.add_argument("args", nargs="+")
    parser_docker_run.add_argument("-v", "--volume", action="append", default=[])

    #
    # other commands
    #

    parser_init = subparsers.add_parser("init")
    parser_init.add_argument("--force", action="store_true")

    parser_build = subparsers.add_parser("build")
    parser_build.add_argument("--release", action="store_true")

    parser_linux = subparsers.add_parser("linux")
    parser_linux.add_argument("repo")
    parser_linux.add_argument("--kvm", action="store_true")
    parser_linux.add_argument("--harness")
    parser_linux.add_argument("--blob")

    # actions
    args = parser.parse_args()
    if args.command == "docker":
        if args.cmd_docker == "build":
            cmd_docker_build(args.force)
        elif args.cmd_docker == "shell":
            cmd_docker_shell(args.volume)
        elif args.cmd_docker == "run":
            cmd_docker_run(args.volume, args.args)
        else:
            parser_docker.print_help()
    elif args.command == "init":
        cmd_init(args.force)
    elif args.command == "build":
        cmd_build(args.release)
    elif args.command == "linux":
        cmd_linux(args.repo, args.kvm, args.harness, args.blob)
    else:
        parser.print_help()


if __name__ == "__main__":
    main()
