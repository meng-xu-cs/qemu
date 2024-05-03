#!/usr/bin/env python3

import argparse
import multiprocessing
import os
import shutil
import subprocess
import sys
from pathlib import Path
from tempfile import TemporaryDirectory, NamedTemporaryFile
from typing import List, Optional, Union

import utils

# path constants
PATH_REPO = Path(__file__).parent.resolve()
PATH_AGENT = os.path.join(PATH_REPO, "agent")
PATH_BUILD = os.path.join(PATH_REPO, "build")
PATH_WKS = os.path.join(PATH_REPO, "workspace")

PATH_AGENT_HOST_SRC = os.path.join(PATH_REPO, "agent", "host")
PATH_AGENT_GUEST_SRC = os.path.join(PATH_REPO, "agent", "guest", "guest.c")

PATH_WKS_ARTIFACT = os.path.join(PATH_WKS, "artifact")
PATH_WKS_ARTIFACT_BUILD = os.path.join(PATH_WKS_ARTIFACT, "build")
PATH_WKS_ARTIFACT_INSTALL = os.path.join(PATH_WKS_ARTIFACT, "install")
PATH_WKS_ARTIFACT_INSTALL_LIB = os.path.join(
    PATH_WKS_ARTIFACT_INSTALL, "lib", "x86_64-linux-gnu"
)
PATH_WKS_ARTIFACT_INSTALL_QEMU_AMD64 = os.path.join(
    PATH_WKS_ARTIFACT_INSTALL, "bin", "qemu-system-x86_64"
)

PATH_WKS_LINUX = os.path.join(PATH_WKS, "linux")
PATH_WKS_LINUX_KERNEL = os.path.join(PATH_WKS_LINUX, "kernel.img")
PATH_WKS_LINUX_INITRD = os.path.join(PATH_WKS_LINUX, "initrd.img")
PATH_WKS_LINUX_HARNESS = os.path.join(PATH_WKS_LINUX, "harness")
PATH_WKS_LINUX_BLOB = os.path.join(PATH_WKS_LINUX, "blob.data")
PATH_WKS_LINUX_SCRIPT = os.path.join(PATH_WKS_LINUX, "script.sh")
PATH_WKS_LINUX_AGENT_HOST = os.path.join(PATH_WKS_LINUX, "agent-host")
PATH_WKS_LINUX_AGENT_GUEST = os.path.join(PATH_WKS_LINUX, "agent-guest")

# docker constants
DOCKER_TAG = "qemu"
DOCKER_SRC_DIR = "/src"
DOCKER_WORKDIR_PREFIX = "/work"

# system constants
NUM_CPUS = multiprocessing.cpu_count()


def _docker_exec(
    tag: str, volumes: List[str], ephemeral: bool, interactive: bool, cmdline: List[str]
) -> None:
    command = ["docker", "run", "--device=/dev/kvm", "--tmpfs", "/dev/shm:exec"]

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
        subprocess.check_call(["docker", "image", "prune", "--force"])

    subprocess.check_call(["docker", "build", ".", "-t", DOCKER_TAG], cwd=PATH_REPO)


def cmd_docker_shell(volumes: List[str]) -> None:
    _docker_exec(DOCKER_TAG, volumes, True, True, ["bash"])


def _docker_exec_self(volumes: List[str], args: List[str]) -> None:
    _docker_exec(
        DOCKER_TAG,
        volumes,
        True,
        False,
        ["python3", "{}/run.py".format(DOCKER_SRC_DIR), *args],
    )


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
        "--enable-kvm",
        "--enable-slirp",
        "--disable-multiprocess",
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


def _prepare_linux(
    kernel: str,
    harness: Optional[str],
    blob: Optional[str],
    verbose: bool,
) -> Optional[str]:
    # clear previous states
    if os.path.exists(PATH_WKS_LINUX):
        shutil.rmtree(PATH_WKS_LINUX)
    os.mkdir(PATH_WKS_LINUX)

    # search for kernel image
    if not os.path.exists(kernel):
        sys.exit("kernel image does not exist: {}".format(kernel))
    shutil.copy2(kernel, PATH_WKS_LINUX_KERNEL)

    # prepare an initramfs
    # TODO: place it in correct location
    utils.mk_initramfs(PATH_WKS_LINUX_INITRD, harness, None, blob)

    # prepare for execution script
    if harness is None:
        # shell mode
        if blob is not None:
            sys.exit("cannot specify blob without harness")
        return None

    if not os.path.exists(harness):
        sys.exit("harness source code does not exist at {}".format(harness))
    subprocess.check_call(["cc", "-static", harness, "-o", PATH_WKS_LINUX_HARNESS])

    if blob is None:
        # fuzzing mode
        agent_host_name = "qce-agent-host"
        if not verbose:
            # build a fresh host agent
            with TemporaryDirectory() as tmp:
                subprocess.check_call(
                    [
                        "cargo",
                        "build",
                        "--release",
                        "--target-dir",
                        tmp,
                    ],
                    cwd=PATH_AGENT_HOST_SRC,
                )
                shutil.copy2(
                    os.path.join(tmp, "release", agent_host_name),
                    PATH_WKS_LINUX_AGENT_HOST,
                )
        else:
            # re-use the debug build of the host agent
            shutil.copy2(
                os.path.join(PATH_AGENT_HOST_SRC, "target", "debug", agent_host_name),
                PATH_WKS_LINUX_AGENT_HOST,
            )

        # compile the guest agent
        subprocess.check_call(
            [
                "cc",
                "-static",
                "-std=c2x",
                '-DHARNESS="{}"'.format(PATH_WKS_LINUX_HARNESS),
                PATH_AGENT_GUEST_SRC,
                "-o",
                PATH_WKS_LINUX_AGENT_GUEST,
            ],
            cwd=PATH_AGENT,
        )
        guest_cmdline = "echo '[harness-fuzz] {}'\n{}{}".format(
            harness, PATH_WKS_LINUX_AGENT_GUEST, PATH_WKS_LINUX_BLOB
        )

    else:
        # testing mode
        if not os.path.exists(blob):
            sys.exit("blob data file does not exist at {}".format(blob))

        shutil.copy2(blob, PATH_WKS_LINUX_BLOB)
        guest_cmdline = "echo '[harness-test] {}'\n{} {}".format(
            harness, PATH_WKS_LINUX_HARNESS, PATH_WKS_LINUX_BLOB
        )

    with open(PATH_WKS_LINUX_SCRIPT, "w") as f:
        f.write("#!/bin/sh\n{}".format(guest_cmdline))
    os.chmod(PATH_WKS_LINUX_SCRIPT, 0o755)

    # mark that we have script to execute
    return PATH_WKS_LINUX_SCRIPT


def _execute_linux(
    virtme: str,
    script: Optional[str],
    workspace: str,
    monitor_socket: str,
    kvm: bool,
    verbose: bool,
) -> None:
    # basics
    command = [virtme]
    if verbose:
        command.extend(["--show-command", "--verbose", "--show-boot-console"])

    # machine
    command.extend(["--qemu-bin", PATH_WKS_ARTIFACT_INSTALL_QEMU_AMD64])
    command.extend(["--memory", "2G"])
    if not kvm:
        command.extend(["--disable-kvm"])

    # kernel
    command.extend(["--kimg", PATH_WKS_LINUX_KERNEL])
    command.extend(["--mods", "auto"])

    # script
    if script is not None:
        command.extend(["--script-sh", script])

    # workspace
    command.append("--rwdir=/tmp/wks={}".format(workspace))

    # monitor
    command.extend(
        [
            "--qemu-opts",
            "-chardev",
            "socket,id=mon1,path={},server=on,wait=off".format(monitor_socket),
            "-mon",
            "chardev=mon1,mode=control",
        ]
    )

    # execute
    subprocess.check_call(
        command, env={"LD_LIBRARY_PATH": PATH_WKS_ARTIFACT_INSTALL_LIB}
    )


def cmd_linux(
    virtme: str,
    kernel: str,
    kvm: bool,
    harness: Optional[str],
    blob: Optional[str],
    verbose: bool,
) -> None:
    script = _prepare_linux(kernel, harness, blob, verbose)
    with TemporaryDirectory() as tmp:
        # place the mark and monitor files first
        Path(os.path.join(tmp, "MARK")).touch(exist_ok=False)
        with NamedTemporaryFile() as monitor_socket:
            # start the host
            host = subprocess.Popen(
                [
                    PATH_WKS_LINUX_AGENT_HOST,
                    tmp,
                    monitor_socket.name,
                ]
            )
            # start the guest
            _execute_linux(virtme, script, tmp, monitor_socket.name, kvm, verbose)
            # wait for host termination
            host.wait()


def _dev_fresh() -> None:
    _docker_exec_self([], ["build"])


def cmd_dev_sample(volumes: List[str], kvm: bool, solution: bool) -> None:
    if len(volumes) != 1:
        sys.exit("Expect one and only one volume to attach")

    passthrough_args = ["linux"]
    if kvm:
        passthrough_args.append("--kvm")

    passthrough_args.append("--verbose")
    passthrough_args.extend(["--virtme", "/virtme-ng/virtme-run"])
    passthrough_args.extend(
        [
            "--kernel",
            "/{}0/src/arch/x86_64/boot/bzImage".format(DOCKER_WORKDIR_PREFIX),
        ]
    )
    passthrough_args.extend(
        [
            "--harness",
            "/{}0/src/test_harnesses/linux_test_harness.c".format(
                DOCKER_WORKDIR_PREFIX
            ),
        ]
    )
    if solution:
        passthrough_args.extend(
            [
                "--blob",
                "/{}0/exemplar_only/blobs/sample_solve.bin".format(
                    DOCKER_WORKDIR_PREFIX
                ),
            ]
        )

    _docker_exec_self(volumes, passthrough_args)


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

    #
    # dev group
    #

    parser_dev = subparsers.add_parser("dev")
    parser_dev.add_argument("--fresh", action="store_true")
    parser_dev.add_argument("-v", "--volume", action="append", default=[])
    sub_dev = parser_dev.add_subparsers(dest="cmd_dev")

    parser_dev_sample = sub_dev.add_parser("sample")
    parser_dev_sample.add_argument("--kvm", action="store_true")
    parser_dev_sample.add_argument("--solution", action="store_true")

    #
    # other commands
    #

    parser_init = subparsers.add_parser("init")
    parser_init.add_argument("--force", action="store_true")

    parser_build = subparsers.add_parser("build")
    parser_build.add_argument("--release", action="store_true")

    parser_linux = subparsers.add_parser("linux")
    parser_linux.add_argument("--virtme", required=True)
    parser_linux.add_argument("--kernel", required=True)
    parser_linux.add_argument("--kvm", action="store_true")
    parser_linux.add_argument("--harness")
    parser_linux.add_argument("--blob")
    parser_linux.add_argument("--verbose", action="store_true")

    # actions
    args = parser.parse_args()
    if args.command == "docker":
        if args.cmd_docker == "build":
            cmd_docker_build(args.force)
        elif args.cmd_docker == "shell":
            cmd_docker_shell(args.volume)
        else:
            parser_docker.print_help()

    elif args.command == "dev":
        if args.fresh:
            _dev_fresh()
        if args.cmd_dev == "sample":
            cmd_dev_sample(args.volume, args.kvm, args.solution)
        else:
            parser_dev.print_help()

    elif args.command == "init":
        cmd_init(args.force)
    elif args.command == "build":
        cmd_build(args.release)
    elif args.command == "linux":
        cmd_linux(
            args.virtme, args.kernel, args.kvm, args.harness, args.blob, args.verbose
        )
    else:
        parser.print_help()


if __name__ == "__main__":
    main()
