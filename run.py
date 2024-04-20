#!/usr/bin/env python3

import argparse
import multiprocessing
import os
import shutil
import subprocess
import sys
from enum import Enum
from pathlib import Path
from tempfile import TemporaryDirectory
from typing import List, Optional, Union

import utils

# path constants
PATH_REPO = Path(__file__).parent.resolve()
PATH_AGENT = os.path.join(PATH_REPO, "agent")
PATH_BUILD = os.path.join(PATH_REPO, "build")
PATH_WKS = os.path.join(PATH_REPO, "workspace")

PATH_Z3_SRC = os.path.join(PATH_REPO, "contrib", "smt", "z3")

PATH_AGENT_HOST_SRC = os.path.join(PATH_REPO, "agent", "host")
PATH_AGENT_GUEST_SRC = os.path.join(PATH_REPO, "agent", "guest", "guest.c")

PATH_WKS_DEPS = os.path.join(PATH_WKS, "deps")
PATH_WKS_DEPS_Z3 = os.path.join(PATH_WKS_DEPS, "z3")
PATH_WKS_DEPS_Z3_LIB = os.path.join(PATH_WKS_DEPS_Z3, "lib")

PATH_WKS_HOST = os.path.join(PATH_WKS, "host")
PATH_WKS_HOST_Z3 = os.path.join(PATH_WKS_HOST, "z3")

PATH_WKS_ARTIFACT = os.path.join(PATH_WKS, "artifact")
PATH_WKS_ARTIFACT_CONFIG = os.path.join(PATH_WKS_ARTIFACT, "config")
PATH_WKS_ARTIFACT_BUILD = os.path.join(PATH_WKS_ARTIFACT, "build")
PATH_WKS_ARTIFACT_INSTALL = os.path.join(PATH_WKS_ARTIFACT, "install")
PATH_WKS_ARTIFACT_INSTALL_LIB = os.path.join(
    PATH_WKS_ARTIFACT_INSTALL, "lib", "x86_64-linux-gnu"
)
PATH_WKS_ARTIFACT_INSTALL_QEMU_AMD64 = os.path.join(
    PATH_WKS_ARTIFACT_INSTALL, "bin", "qemu-system-x86_64"
)
PATH_WKS_ARTIFACT_INSTALL_QEMU_IMG = os.path.join(
    PATH_WKS_ARTIFACT_INSTALL, "bin", "qemu-img"
)

PATH_WKS_LINUX = os.path.join(PATH_WKS, "linux")
PATH_WKS_LINUX_KERNEL = os.path.join(PATH_WKS_LINUX, "kernel.img")
PATH_WKS_LINUX_INITRD = os.path.join(PATH_WKS_LINUX, "initrd.img")
PATH_WKS_LINUX_DISK = os.path.join(PATH_WKS_LINUX, "disk.qcow2")
PATH_WKS_LINUX_HARNESS_SRC = os.path.join(PATH_WKS_LINUX, "harness.c")
PATH_WKS_LINUX_HARNESS_BIN = os.path.join(PATH_WKS_LINUX, "harness")
PATH_WKS_LINUX_AGENT_HOST = os.path.join(PATH_WKS_LINUX, "agent-host")
PATH_WKS_LINUX_AGENT_GUEST = os.path.join(PATH_WKS_LINUX, "agent-guest")
PATH_WKS_LINUX_ROOTFS_EXT4 = os.path.join(PATH_WKS_LINUX, "rootfs.ext4")
PATH_WKS_LINUX_TRACE = os.path.join(PATH_WKS_LINUX, "trace")

# system constants
NUM_CPUS = multiprocessing.cpu_count()
KB_IN_BYTES = 1024
MB_IN_BYTES = KB_IN_BYTES * 1024
GB_IN_BYTES = MB_IN_BYTES * 1024

# qemu constants
VM_MEM_SIZE = 2 * GB_IN_BYTES
VM_DISK_NAME = "disk0"
VM_DISK_SIZE = 2 * GB_IN_BYTES
VM_IVSHMEM_FILE = "ivshmem"
VM_IVSHMEM_SIZE = 16 * MB_IN_BYTES
VM_MONITOR_SOCKET = "monitor"

# docker constants
DOCKER_TAG = "qemu"
DOCKER_SRC_DIR = "/src"
DOCKER_WORKDIR_PREFIX = "/work"


def _docker_exec(
    tag: str, volumes: List[str], ephemeral: bool, interactive: bool, cmdline: List[str]
) -> None:
    command = [
        "docker",
        "run",
        "--device=/dev/kvm",
        "--tmpfs",
        "/dev/shm:exec",
        "--tmpfs",
        "/tmp",
        "--privileged=true",
    ]

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
    command = ["docker", "build", ".", "-t", DOCKER_TAG]
    if force:
        subprocess.check_call(["docker", "pull", "ubuntu:24.04"])
        command.append("--no-cache")
    subprocess.check_call(command, cwd=PATH_REPO)


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


def __build_deps_z3(path_src: Union[str, Path], path_install: Union[str, Path]):
    # clear up (in case of unsuccessful build)
    deps_z3_build = os.path.join(path_src, "build")
    if os.path.exists(deps_z3_build):
        shutil.rmtree(deps_z3_build)

    # config
    subprocess.check_call(
        [
            "python3",
            "scripts/mk_make.py",
            "--prefix={}".format(path_install),
        ],
        cwd=path_src,
    )

    # build and install
    subprocess.check_call(
        ["make", "-j{}".format(NUM_CPUS)],
        cwd=deps_z3_build,
    )
    subprocess.check_call(
        ["make", "install"],
        cwd=deps_z3_build,
    )

    # clean up
    shutil.rmtree(deps_z3_build)


def _qemu_config(
    path_build: Union[str, Path],
    path_install: Union[str, Path],
    path_deps_z3: str,
    release: bool,
) -> None:
    # tweak the qce config
    config_header = os.path.join(PATH_REPO, "include", "qemu", "qce-config.h")
    if not os.path.exists(config_header):
        sys.exit("Cannot locate the QCE config header")

    config_content = [
        # header
        "/* automatically generated, do not modify */",
        "#ifndef QEMU_QCE_CONFIG_H",
        # release configs
        "#define QCE_RELEASE" if release else "/* QCE_RELEASE not set */",
        "/* QCE_DEBUG_IR not set */" if release else "#define QCE_DEBUG_IR",
        # other configs
        "#define QCE_SMT_Z3_EAGER_SIMPLIFY",
        "/* QCE_SUPPORTS_VEC not set */",
        # tailer
        "#endif",
    ]
    with open(config_header, "w") as f:
        for line in config_content:
            f.write(line + "\n")

    # run autoconfig
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
        # deps
        "--z3={}".format(path_deps_z3),
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

    subprocess.check_call(command, cwd=path_build)


def cmd_init(force: bool) -> None:
    # prepare directory
    if os.path.exists(PATH_BUILD):
        if not force:
            sys.exit("Build directory exists: {}".format(PATH_BUILD))
        shutil.rmtree(PATH_BUILD)

    if os.path.exists(PATH_WKS_HOST):
        if not force:
            sys.exit("WKS-host directory exists: {}".format(PATH_WKS_HOST))
        shutil.rmtree(PATH_WKS_HOST)

    os.makedirs(PATH_WKS, exist_ok=True)
    os.mkdir(PATH_WKS_HOST)

    # deps
    __build_deps_z3(PATH_Z3_SRC, PATH_WKS_HOST_Z3)

    # build
    _qemu_config(
        PATH_REPO, os.devnull, os.path.relpath(PATH_WKS_HOST_Z3, PATH_REPO), False
    )
    subprocess.check_call(["make", "-j{}".format(NUM_CPUS)], cwd=PATH_BUILD)

    # connect the compilation database
    shutil.copy2(
        os.path.join(PATH_BUILD, "compile_commands.json"),
        os.path.join(PATH_REPO, "compile_commands.json"),
    )


def cmd_build(incremental: bool, release: bool, deps_z3: bool) -> None:
    # config
    if incremental:
        if deps_z3:
            sys.exit("cannot run incremental build with dependencies building")

        # check consistency
        if not os.path.exists(PATH_WKS_ARTIFACT_CONFIG):
            sys.exit("cannot run incremental build without a config file")

        with open(PATH_WKS_ARTIFACT_CONFIG) as f:
            content = f.read()
        if content == "debug":
            if release:
                sys.exit("cannot run incremental release build on a debug base")
        elif content == "release":
            if not release:
                sys.exit("cannot run incremental debug build on a release base")
        else:
            sys.exit("unknown build config: {}".format(content))

    else:
        if os.path.exists(PATH_WKS_ARTIFACT):
            shutil.rmtree(PATH_WKS_ARTIFACT)
            os.makedirs(PATH_WKS_ARTIFACT, exist_ok=False)

        # deps
        if deps_z3:
            __build_deps_z3(PATH_Z3_SRC, PATH_WKS_DEPS_Z3)

        # mark
        with open(PATH_WKS_ARTIFACT_CONFIG, "w") as f:
            f.write("release" if release else "debug")

        # qemu
        os.makedirs(PATH_WKS_ARTIFACT_BUILD, exist_ok=True)
        _qemu_config(
            PATH_WKS_ARTIFACT_BUILD,
            PATH_WKS_ARTIFACT_INSTALL,
            os.path.relpath(PATH_WKS_DEPS_Z3, PATH_REPO),
            release,
        )

    # build
    subprocess.check_call(
        ["make", "-j{}".format(NUM_CPUS)], cwd=PATH_WKS_ARTIFACT_BUILD
    )
    subprocess.check_call(["make", "install"], cwd=PATH_WKS_ARTIFACT_BUILD)


class AgentMode(Enum):
    Shell = 0
    Test = 1
    Fuzz = 2
    Check = 3


def __compile_agent_guest(mode: AgentMode, simulate_virtme: bool):
    command = [
        "cc",
        "-static",
        "-std=c2x",
        "-DMODE_{}=1".format(mode.name),
        PATH_AGENT_GUEST_SRC,
        "-o",
        PATH_WKS_LINUX_AGENT_GUEST,
    ]
    if simulate_virtme:
        command.append("-DVIRTME")

    # run the compilation
    subprocess.check_call(command, cwd=PATH_AGENT)


def __compile_agent_host(verbose: bool):
    agent_host_name = "qce-agent-host"

    # re-use the debug build of the host agent
    if verbose:
        subprocess.check_call(["cargo", "build"], cwd=PATH_AGENT_HOST_SRC)
        shutil.copy2(
            os.path.join(PATH_AGENT_HOST_SRC, "target", "debug", agent_host_name),
            PATH_WKS_LINUX_AGENT_HOST,
        )
        return

    # build a fresh host agent (in non-verbose mode)
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


def _prepare_linux(
    kvm: bool,
    kernel: str,
    harness: Optional[str],
    blob: Optional[str],
    simulate_virtme: bool,
    verbose: bool,
) -> AgentMode:
    # infer mode based on parameters
    if harness is None:
        if blob is None:
            mode = AgentMode.Shell
        else:
            if not os.path.exists(blob):
                sys.exit("blob check file does not exist at {}".format(blob))
            if kvm:
                sys.exit("cannot run checks when kvm is enabled")
            harness = blob  # TODO: use a better way to represent --check than this hack
            mode = AgentMode.Check
    else:
        if not os.path.exists(harness):
            sys.exit("harness source code does not exist at {}".format(harness))

        if blob is None:
            if kvm:
                sys.exit("cannot run fuzzer when kvm is enabled")
            mode = AgentMode.Fuzz
        else:
            if not os.path.exists(blob):
                sys.exit("blob data file does not exist at {}".format(blob))
            mode = AgentMode.Test

    # clear previous states
    if os.path.exists(PATH_WKS_LINUX):
        shutil.rmtree(PATH_WKS_LINUX)
    os.mkdir(PATH_WKS_LINUX)

    # search for kernel image
    if not os.path.exists(kernel):
        sys.exit("kernel image does not exist: {}".format(kernel))
    shutil.copy2(kernel, PATH_WKS_LINUX_KERNEL)

    # conditionally compile the host agent
    if mode in [AgentMode.Fuzz, AgentMode.Check]:
        __compile_agent_host(verbose)

    # always compile the guest agent
    __compile_agent_guest(mode, simulate_virtme)

    # prepare harness
    if mode in [AgentMode.Test, AgentMode.Fuzz, AgentMode.Check]:
        assert harness is not None  # to keep mypy happy
        if mode == AgentMode.Test:
            shutil.copy2(harness, PATH_WKS_LINUX_HARNESS_SRC)
        else:
            utils.patch_harness(harness, PATH_WKS_LINUX_HARNESS_SRC)

        subprocess.check_call(
            [
                "cc",
                "-static",
                PATH_WKS_LINUX_HARNESS_SRC,
                "-o",
                PATH_WKS_LINUX_HARNESS_BIN,
            ]
        )

    # prepare the rootfs image
    utils.mk_rootfs(
        PATH_WKS_ARTIFACT_INSTALL_QEMU_IMG,
        PATH_WKS_LINUX_DISK,
        VM_DISK_SIZE,
        PATH_WKS_LINUX_AGENT_GUEST,
        None if harness is None else PATH_WKS_LINUX_HARNESS_BIN,
        None if mode == AgentMode.Check else blob,
        use_host_rootfs=simulate_virtme,
    )

    # prepare the init ramdisk image
    utils.mk_initramfs(PATH_WKS_LINUX_INITRD)

    # done with the preparation
    return mode


def _execute_linux(
    tmp: str,
    kvm: bool,
    managed_by_host: bool,
    check: bool,
    trace: bool,
    verbose: bool,
) -> None:
    # prepare the pipe
    path_ivshmem = Path(tmp).joinpath(VM_IVSHMEM_FILE)

    # command holder
    command = [PATH_WKS_ARTIFACT_INSTALL_QEMU_AMD64]
    kernel_args = []

    # basics
    command.extend(["-m", "{}G".format(VM_MEM_SIZE // VM_MEM_SIZE)])
    if kvm:
        command.extend(["-machine", "accel=kvm"])
    else:
        command.extend(["-machine", "accel=tcg"])
        # we need to ask vm snapshot to store the clock as well
        command.extend(["-icount", "shift=auto", "-rtc", "clock=vm"])

    # no display
    command.extend(["-vga", "none"])
    command.extend(["-display", "none"])

    # IO
    command.extend(["-parallel", "none"])
    command.extend(["-serial", "stdio"])
    if verbose:
        kernel_args.append("console=ttyS0")

    # kernel
    command.extend(["-kernel", PATH_WKS_LINUX_KERNEL])
    command.extend(["-initrd", PATH_WKS_LINUX_INITRD])

    # disk
    command.extend(
        [
            "-drive",
            ",".join(
                [
                    "file={}".format(PATH_WKS_LINUX_DISK),
                    "node-name={}".format(VM_DISK_NAME),
                    "if=virtio",
                    "media=disk",
                    "index=0",
                ]
            ),
        ]
    )

    # networking
    command.extend(["-net", "none"])
    command.extend(["-device", "virtio-net-pci,netdev=n0"])
    command.extend(["-netdev", "user,id=n0"])
    kernel_args.extend(
        [
            # prevent annoying interface renaming
            "net.ifnames=0",
            "biosdevname=0",
        ]
    )

    # behaviors
    kernel_args.append("panic=-1")  # instructs the kernel to reboot immediately
    command.append("-no-reboot")  # and immediately intercept the reboot in QEMU
    if managed_by_host:
        command.append("-no-shutdown")

    # monitor
    command.extend(
        [
            "-chardev",
            "socket,id=qmp,path={},server=on,wait=off".format(
                os.path.join(tmp, VM_MONITOR_SOCKET)
            ),
            "-mon",
            "chardev=qmp,mode=control",
        ]
    )

    # interaction
    command.extend(
        [
            "-object",
            "memory-backend-file,size={}M,share=on,mem-path={},prealloc=on,id=vmio".format(
                VM_IVSHMEM_SIZE / MB_IN_BYTES, path_ivshmem
            ),
            "-device",
            "ivshmem-plain,memdev=vmio,master=on",
        ]
    )

    # append kernel arguments
    if len(kernel_args) != 0:
        command.extend(["-append", " ".join(kernel_args)])

    # execute
    envs = {
        "LD_LIBRARY_PATH": ":".join(
            [PATH_WKS_ARTIFACT_INSTALL_LIB, PATH_WKS_DEPS_Z3_LIB]
        )
    }
    if check:
        envs["QCE_CHECK"] = "1"
    if trace:
        envs["QCE_TRACE"] = PATH_WKS_LINUX_TRACE
    subprocess.check_call(command, env=envs)


def cmd_linux(
    kvm: bool,
    kernel: str,
    harness: Optional[str],
    blob: Optional[str],
    simulate_virtme: bool,
    trace: bool,
    verbose: bool,
) -> None:
    mode = _prepare_linux(kvm, kernel, harness, blob, simulate_virtme, verbose)
    with TemporaryDirectory() as tmp:
        # start the host only in fuzzing or checking mode
        if mode in [AgentMode.Fuzz, AgentMode.Check]:
            command = [PATH_WKS_LINUX_AGENT_HOST, tmp]
            if mode == AgentMode.Check:
                command.append("--check")
            if verbose:
                command.append("--verbose")
            host = subprocess.Popen(command)
        else:
            host = None

        # start the guest
        _execute_linux(
            tmp,
            kvm,
            host is not None,
            mode == AgentMode.Check,
            trace,
            verbose,
        )

        # wait for host termination (if we have one)
        if host is not None:
            host.wait()


def cmd_dev_check(volumes: List[str]) -> None:
    if len(volumes) != 1:
        sys.exit("Expect one and only one volume to attach")

    passthrough_args = [
        "linux",
        "--verbose",
        "--kernel",
        "/{}0/src/arch/x86_64/boot/bzImage".format(DOCKER_WORKDIR_PREFIX),
        "--blob",
        "/{}0/src/test_harnesses/linux_test_harness.c".format(DOCKER_WORKDIR_PREFIX),
    ]
    _docker_exec_self(volumes, passthrough_args)


def cmd_dev_sample(
    volumes: List[str],
    kvm: bool,
    virtme: bool,
    trace: bool,
    solution: bool,
) -> None:
    if len(volumes) != 1:
        sys.exit("Expect one and only one volume to attach")

    passthrough_args = ["linux", "--verbose"]
    if kvm:
        passthrough_args.append("--kvm")
    if virtme:
        passthrough_args.append("--virtme")
    if trace:
        passthrough_args.append("--trace")

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
    parser_dev.add_argument("--release", action="store_true")
    parser_dev.add_argument("-v", "--volume", action="append", default=[])
    sub_dev = parser_dev.add_subparsers(dest="cmd_dev")

    sub_dev.add_parser("check")

    parser_dev_sample = sub_dev.add_parser("sample")
    parser_dev_sample.add_argument("--kvm", action="store_true")
    parser_dev_sample.add_argument("--virtme", action="store_true")
    parser_dev_sample.add_argument("--trace", action="store_true")
    parser_dev_sample.add_argument("--solution", action="store_true")

    #
    # base commands
    #

    parser_init = subparsers.add_parser("init")
    parser_init.add_argument("--force", action="store_true")

    parser_build = subparsers.add_parser("build")
    parser_build.add_argument("--incremental", action="store_true")
    parser_build.add_argument("--release", action="store_true")
    parser_build.add_argument("--deps-z3", action="store_true")

    parser_linux = subparsers.add_parser("linux")
    parser_linux.add_argument("--kernel", required=True)
    parser_linux.add_argument("--kvm", action="store_true")
    parser_linux.add_argument("--harness")
    parser_linux.add_argument("--blob")
    parser_linux.add_argument("--virtme", action="store_true")
    parser_linux.add_argument("--trace", action="store_true")
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
        build_args = ["build"]
        if args.release:
            build_args.append("--release")
        if args.fresh:
            build_args.append("--deps-z3")
        else:
            build_args.append("--incremental")
        _docker_exec_self([], build_args)

        if args.cmd_dev == "check":
            cmd_dev_check(args.volume)
        elif args.cmd_dev == "sample":
            cmd_dev_sample(
                args.volume,
                args.kvm,
                args.virtme,
                args.trace,
                args.solution,
            )
        else:
            parser_dev.print_help()

    elif args.command == "init":
        cmd_init(args.force)
    elif args.command == "build":
        cmd_build(args.incremental, args.release, args.deps_z3)
    elif args.command == "linux":
        cmd_linux(
            args.kvm,
            args.kernel,
            args.harness,
            args.blob,
            args.virtme,
            args.trace,
            args.verbose,
        )
    else:
        parser.print_help()


if __name__ == "__main__":
    main()
