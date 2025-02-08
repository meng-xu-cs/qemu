#!/usr/bin/env python3

import argparse
import multiprocessing
import logging
import os
import platform
import shutil
import struct
import subprocess
import sys
import tempfile
from enum import Enum
from pathlib import Path
from tempfile import TemporaryDirectory
from typing import List, Optional, Set, Union

import utils


# platform configs
class SupportedPlatform(Enum):
    LinuxAmd64 = 0
    MacOSArm64 = 1
    LinuxArm64 = 2


class SupportedApproach(Enum):
    Through = 0
    Emulate = 1
    XNative = 2


class PlatformConfigs(object):
    def __init__(self):
        # probe for the platform configurations
        os_type = platform.system()
        cpu_arch = platform.machine()

        match os_type:
            case "Linux":
                match cpu_arch:
                    case "x86_64":
                        self.platform = SupportedPlatform.LinuxAmd64
                    case "aarch64":
                        self.platform = SupportedPlatform.LinuxArm64
                    case _:
                        sys.exit("expect either x86_64 or aarch64 ISA on Linux")
            case "Darwin":
                match cpu_arch:
                    case "arm64":
                        self.platform = SupportedPlatform.MacOSArm64
                    case _:
                        sys.exit("expect only arm64 ISA on MacOS")
            case _:
                sys.exit(f"Unsupported platform: {os_type} on {cpu_arch}")

        # whether emulation is requested
        match self.platform:
            case SupportedPlatform.LinuxAmd64:
                self.approach = SupportedApproach.Through
            case SupportedPlatform.LinuxArm64 | SupportedPlatform.MacOSArm64:
                if os.environ.get("EMULATE_IN_DOCKER", None) == "1":
                    self.approach = SupportedApproach.Emulate
                else:
                    self.approach = SupportedApproach.XNative

    def docker_tag(self) -> str:
        match self.approach:
            case SupportedApproach.Through:
                tag = "qemu"
            case SupportedApproach.Emulate:
                tag = "qemu-emulate"
            case SupportedApproach.XNative:
                tag = "qemu-xnative"
        return tag

    def arch_name(self) -> str:
        match self.platform:
            case SupportedPlatform.LinuxAmd64:
                name = "x86_64"
            case SupportedPlatform.LinuxArm64:
                name = "aarch64"
            case SupportedPlatform.MacOSArm64:
                name = "arm64"
        return name


PLATFORM = PlatformConfigs()

# path constants
PATH_REPO = Path(__file__).parent.resolve()
PATH_AGENT = os.path.join(PATH_REPO, "agent")
PATH_BUILD = os.path.join(PATH_REPO, "build")
PATH_WKS = os.path.join(PATH_REPO, "workspace")

PATH_Z3_SRC = os.path.join(PATH_REPO, "contrib", "smt", "z3")

PATH_AGENT_HOST_SRC = os.path.join(PATH_REPO, "agent", "host")
PATH_AGENT_GUEST_SRC = os.path.join(PATH_REPO, "agent", "guest", "guest.c")

PATH_TESTS = os.path.join(PATH_REPO, "tests", "qce")
PATH_TESTS_SAMPLE = os.path.join(PATH_TESTS, "sample")
PATH_TESTS_KERNEL = os.path.join(PATH_TESTS, "kernel")
PATH_TESTS_E2E = os.path.join(PATH_TESTS, "e2e")

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
    PATH_WKS_ARTIFACT_INSTALL, "lib", "{}-linux-gnu".format(PLATFORM.arch_name())
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
PATH_WKS_LINUX_DEFAULT_CORPUS = os.path.join(PATH_WKS_LINUX, "corpus")
PATH_WKS_LINUX_DEFAULT_OUTPUT = os.path.join(PATH_WKS_LINUX, "output")

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

# testing constants
TESTING_DEFAULT_KERNEL = "v6.11.7-miniconfig"

# docker constants
DOCKER_SRC_DIR = "/src"
DOCKER_WORKDIR_PREFIX = "/work"


def _docker_exec(
    volumes: List[str],
    ephemeral: bool,
    interactive: bool,
    cmdline: List[str],
) -> None:
    command = ["docker", "run"]
    if PLATFORM.approach == SupportedApproach.Emulate:
        command.extend(["--platform", "linux/amd64"])

    command.append("--privileged")
    if ephemeral:
        command.append("--rm")
    if interactive:
        command.extend(["--interactive", "--tty"])

    command.extend(["--tmpfs", tempfile.gettempdir()])
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

    command.append(PLATFORM.docker_tag())
    command.extend(cmdline)
    subprocess.check_call(command)


def cmd_docker_build(force: bool) -> None:
    # update the base image if requested
    if force:
        command = ["docker", "pull"]
        if PLATFORM.approach == SupportedApproach.Emulate:
            command.extend(["--platform", "linux/amd64"])

        command.append("ubuntu:24.04")
        subprocess.check_call(command)

    # build the new image
    command = ["docker", "build"]
    if PLATFORM.approach == SupportedApproach.Emulate:
        command.extend(["--platform", "linux/amd64"])

    tag_final = PLATFORM.docker_tag()
    match PLATFORM.approach:
        case SupportedApproach.Through | SupportedApproach.Emulate:
            tag = tag_final
        case SupportedApproach.XNative:
            tag = f"{tag_final}-base"
    if force:
        command.append("--no-cache")

    command.extend(["-t", tag])
    command.append(".")
    subprocess.check_call(command, cwd=PATH_REPO)

    # update the new image (if needed)
    if PLATFORM.approach == SupportedApproach.XNative:
        name = f"{tag_final}-next"

        command = ["docker", "run"]
        command.extend(["--name", name])
        command.append(tag)
        command.extend(["apt-get", "install", "-y", "gcc-x86-64-linux-gnu"])
        subprocess.check_call(command)

        command = ["docker", "commit", name, tag_final]
        subprocess.check_call(command)


def cmd_docker_shell(volumes: List[str]) -> None:
    _docker_exec(
        volumes,
        True,
        True,
        ["bash"],
    )


def _docker_exec_self(volumes: List[str], args: List[str]) -> None:
    _docker_exec(
        volumes,
        True,
        False,
        ["python3", "{}/run.py".format(DOCKER_SRC_DIR), *args],
    )


def __build_deps_z3(path_src: Union[str, Path], path_install: Union[str, Path]):
    # clear up (in case of unsuccessful build)
    deps_z3_build = os.path.join(path_src, "build")
    if os.path.exists(deps_z3_build):
        subprocess.check_call(["git", "clean", "-fdx", "."], cwd=path_src)

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
        "--enable-slirp",
        "--disable-kvm",
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
        os.makedirs(PATH_WKS_ARTIFACT, exist_ok=False) # need confirmed

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


# execution modes
class ExecSetup(Enum):
    Bare = 1
    Simple = 2
    Virtme = 3


class AgentMode(Enum):
    Shell = 0
    Test = 1
    Fuzz = 2
    Check = 3


def __compile_agent_guest(mode: AgentMode, setup: ExecSetup):
    match PLATFORM.approach:
        case SupportedApproach.Through | SupportedApproach.Emulate:
            cc = "cc"
        case SupportedApproach.XNative:
            cc = "x86_64-linux-gnu-gcc"

    command = [
        cc,
        "-static",
        "-std=c2x",
        "-DMODE_{}=1".format(mode.name),
        "-DSETUP_{}=1".format(setup.name),
        PATH_AGENT_GUEST_SRC,
        "-o",
        PATH_WKS_LINUX_AGENT_GUEST,
    ]

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


def __compile_harness():
    match PLATFORM.approach:
        case SupportedApproach.Through | SupportedApproach.Emulate:
            cc = "cc"
        case SupportedApproach.XNative:
            cc = "x86_64-linux-gnu-gcc"

    # other flags
    command = [
        cc,
        "-static",
        PATH_WKS_LINUX_HARNESS_SRC,
        "-o",
        PATH_WKS_LINUX_HARNESS_BIN,
    ]

    # run the compilation
    subprocess.check_call(command)


def _prepare_linux(
    kernel: str,
    harness: Optional[str],
    blob: Optional[str],
    setup: ExecSetup,
    verbose: bool,
) -> AgentMode:
    # infer mode based on parameters
    if harness is None:
        if blob is None:
            mode = AgentMode.Shell
        else:
            if not os.path.exists(blob):
                sys.exit("blob check file does not exist at {}".format(blob))
            harness = blob  # TODO: use a better way to represent --check than this hack
            mode = AgentMode.Check
    else:
        if not os.path.exists(harness):
            sys.exit("harness source code does not exist at {}".format(harness))

        if blob is None:
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
    __compile_agent_guest(mode, setup)

    # prepare harness
    if mode in [AgentMode.Test, AgentMode.Fuzz, AgentMode.Check]:
        assert harness is not None  # to keep mypy happy
        if mode == AgentMode.Test:
            shutil.copy2(harness, PATH_WKS_LINUX_HARNESS_SRC)
        else:
            utils.patch_harness(harness, PATH_WKS_LINUX_HARNESS_SRC)
        __compile_harness()

    # prepare the disk images
    if setup == ExecSetup.Bare:
        # prepare an empty disk for snapshot purpose
        utils.mk_empty_disk_image(
            PATH_WKS_ARTIFACT_INSTALL_QEMU_IMG,
            PATH_WKS_LINUX_DISK,
            VM_DISK_SIZE,
        )

        # prepare the init ramdisk image
        utils.mk_initramfs_as_rootfs(
            PATH_WKS_LINUX_AGENT_GUEST,
            None if harness is None else PATH_WKS_LINUX_HARNESS_BIN,
            None if mode == AgentMode.Check else blob,
            PATH_WKS_LINUX_INITRD,
        )
    else:
        # prepare the rootfs image
        utils.mk_rootfs(
            PATH_WKS_ARTIFACT_INSTALL_QEMU_IMG,
            PATH_WKS_LINUX_DISK,
            VM_DISK_SIZE,
            PATH_WKS_LINUX_AGENT_GUEST,
            None if harness is None else PATH_WKS_LINUX_HARNESS_BIN,
            None if mode == AgentMode.Check else blob,
            use_host_rootfs=(setup == ExecSetup.Virtme),
        )

        # prepare the init ramdisk image
        utils.mk_initramfs_bootstrap(PATH_WKS_LINUX_INITRD)

    # done with the preparation
    return mode


def _execute_linux(
    tmp: str,
    mode: AgentMode,
    setup: ExecSetup,
    path_corpus: str,
    path_output: str,
    trace: bool,
    verbose: bool,
) -> subprocess.Popen:
    # prepare the pipe
    path_ivshmem = Path(tmp).joinpath(VM_IVSHMEM_FILE)

    # command holder
    command = [PATH_WKS_ARTIFACT_INSTALL_QEMU_AMD64]
    kernel_args = []

    # basics
    command.extend(["-m", "{}G".format(VM_MEM_SIZE // VM_MEM_SIZE)])
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

    # disk (always attached even in bare mode in order to host snapshot)
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
    if setup != ExecSetup.Bare:
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
    if mode == AgentMode.Fuzz:
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
    if mode in [AgentMode.Fuzz, AgentMode.Test, AgentMode.Check]:
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
        ),
        "QCE_CORPUS": path_corpus,
        "QCE_OUTPUT": path_output,
    }
    if mode == AgentMode.Check:
        envs["QCE_CHECK"] = "1"
    if trace:
        envs["QCE_TRACE"] = "1"

    return subprocess.Popen(command, env=envs)


def cmd_linux(
    kernel: str,
    harness: Optional[str],
    blob: Optional[str],
    setup: ExecSetup,
    path_corpus: Optional[str],
    path_output: Optional[str],
    trace: bool,
    verbose: bool,
) -> None:
    # preparation
    mode = _prepare_linux(kernel, harness, blob, setup, verbose)

    # init directories
    if path_corpus is None:
        path_corpus = PATH_WKS_LINUX_DEFAULT_CORPUS
    os.makedirs(path_corpus, exist_ok=True)

    if path_output is None:
        path_output = PATH_WKS_LINUX_DEFAULT_OUTPUT
    os.makedirs(path_output, exist_ok=True)

    # execution
    with TemporaryDirectory() as tmp:
        # start the host only in fuzzing or checking mode
        if mode in [AgentMode.Fuzz, AgentMode.Check]:
            command = [
                PATH_WKS_LINUX_AGENT_HOST,
                tmp,
                "--corpus",
                path_corpus,
                "--output",
                path_output,
            ]
            if mode == AgentMode.Check:
                command.append("--check")
            if verbose:
                command.append("--verbose")
            host = subprocess.Popen(command)
        else:
            host = None

        # start the guest
        guest = _execute_linux(
            tmp,
            mode,
            setup,
            path_corpus,
            path_output,
            trace,
            verbose,
        )

        # wait for host termination (if we have one)
        if host is not None:
            host.wait()

        # decide on what to do with guest
        if mode == AgentMode.Fuzz:
            guest.kill()
        elif guest.wait() != 0:
            sys.exit("guest terminated with error")

        if mode == AgentMode.Test:
            # save the ivshmem file to output directory
            path_ivshmem = os.path.join(path_output, VM_IVSHMEM_FILE)
            if os.path.exists(path_ivshmem):
                sys.exit("ivshmem file already exists (unexpectedly) in testing mode")
            shutil.copy2(os.path.join(tmp, VM_IVSHMEM_FILE), path_ivshmem)


def cmd_dev_sample(
    volume: str,
    bare: bool,
    virtme: bool,
    trace: bool,
    solution: bool,
) -> None:
    # formulate the arguments
    passthrough_args = [
        "linux",
        "--verbose",
        "--kernel",
        "/{}0/bzImage".format(DOCKER_WORKDIR_PREFIX),
        "--harness",
        "/{}0/harness.c".format(DOCKER_WORKDIR_PREFIX),
    ]
    if bare:
        passthrough_args.append("--bare")
    if virtme:
        passthrough_args.append("--virtme")
    if trace:
        passthrough_args.append("--trace")
    if solution:
        passthrough_args.extend(
            [
                "--blob",
                "/{}0/solution.bin".format(DOCKER_WORKDIR_PREFIX),
            ]
        )

    _docker_exec_self([volume], passthrough_args)


def __dev_run_e2e_test(name: str, solution: Optional[str], trace: bool) -> None:
    path_test = os.path.join(PATH_TESTS_E2E, name)

    # compose the new harness
    with open(os.path.join(path_test, "harness.c")) as f:
        content = f.readlines()
    assert len(content) != 0
    assert content[0] == '#include "../common.h"\n'

    with open(os.path.join(PATH_TESTS_E2E, "common.h")) as f:
        merged = f.readlines()
    merged.extend(content[1:])

    # construct a temporary volume
    with TemporaryDirectory() as tmp:
        with open(os.path.join(tmp, "harness.c"), "w") as f_harness:
            f_harness.writelines(merged)

        # copy over the kernel
        path_test_kernel = os.path.join(path_test, "bzImage")
        if not os.path.exists(path_test_kernel):
            path_test_kernel = os.path.join(PATH_TESTS_KERNEL, TESTING_DEFAULT_KERNEL)
        shutil.copy2(path_test_kernel, os.path.join(tmp, "bzImage"))

        # copy over the solution if requested
        if solution is not None:
            shutil.copy2(solution, os.path.join(tmp, "solution.bin"))

        # run the sample over ths temporary volume
        cmd_dev_sample(tmp, True, False, trace, solution is not None)


def cmd_dev_e2e(name: str, solution: Optional[str], trace: bool) -> None:
    if solution is not None:
        __dev_run_e2e_test(name, solution, trace)

    #
    # full cycle of e2e test execution below
    #

    # do the fuzzing first
    __dev_run_e2e_test(name, None, trace)

    # do the seeds check in a temporary directory
    with TemporaryDirectory() as tmp:
        # copy over the seeds
        path_corpus = os.path.join(tmp, "corpus")
        shutil.copytree(PATH_WKS_LINUX_DEFAULT_CORPUS, path_corpus)

        # sanity checks
        if len(os.listdir(os.path.join(path_corpus, "queue"))) != 0:
            sys.exit("pending seeds in queue")

        # check the quality of generated test cases
        ret_seen: Set[int] = set()
        path_corpus_tried = os.path.join(path_corpus, "tried")
        for seed in os.listdir(path_corpus_tried):
            # seed replay
            path_seed = os.path.join(path_corpus_tried, seed)
            __dev_run_e2e_test(name, path_seed, trace)

            # examine the status of the harness return value
            path_ivshmem = os.path.join(PATH_WKS_LINUX_DEFAULT_OUTPUT, VM_IVSHMEM_FILE)
            with open(path_ivshmem, "rb") as f:
                status = struct.unpack("<Q", f.read(8))[0]

            # derive the return code
            if status & 0x7F != 0:
                sys.exit("harness terminated by signal unexpectedly")
            rc = (status & 0xFF00) >> 8

            # ensure uniqueness
            if rc in ret_seen:
                sys.exit(f"return code {rc} marked multiple times")

            # ensure completeness
            for i in range(len(ret_seen)):
                if i not in ret_seen:
                    sys.exit(f"missing return code {i} in corpus")


def cmd_dev_check(unit_tests_only: bool) -> None:
    # unit tests
    logging.debug("running unit tests")
    with TemporaryDirectory() as tmp:
        path_test_kernel = os.path.join(PATH_TESTS_KERNEL, TESTING_DEFAULT_KERNEL)
        shutil.copy2(path_test_kernel, os.path.join(tmp, "bzImage"))
        path_test_harness = os.path.join(PATH_TESTS_SAMPLE, "harness.c")
        shutil.copy2(path_test_harness, os.path.join(tmp, "harness.c"))

        passthrough_args = [
            "linux",
            "--verbose",
            "--bare",
            "--kernel",
            "/{}0/bzImage".format(DOCKER_WORKDIR_PREFIX),
            # NOTE: passing `--blob` without `--harness` triggers unit tests
            # TODO: it is counter-intuitive, so maybe find a better way to do this
            "--blob",
            "/{}0/harness.c".format(DOCKER_WORKDIR_PREFIX),
        ]
        _docker_exec_self([tmp], passthrough_args)

    logging.info("all unit tests passed")
    if unit_tests_only:
        return

    # e2e tests
    for item in os.listdir(os.path.join(PATH_TESTS, "e2e")):
        # ignore the common file
        if item == "common.h":
            continue

        # full cycle of e2e test
        logging.debug(f"running e2e test {item}")
        cmd_dev_e2e(item, None, False)
        logging.info(f"e2e test {item} passed")

    logging.info("all e2e tests passed")


def main() -> None:
    # args
    parser = argparse.ArgumentParser()
    parser.add_argument("--verbose", action="store_true")
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
    sub_dev = parser_dev.add_subparsers(dest="cmd_dev")

    parser_dev_sample = sub_dev.add_parser("sample")
    parser_dev_sample.add_argument("--volume", default=PATH_TESTS_SAMPLE)
    parser_dev_sample_exec_setup = parser_dev_sample.add_mutually_exclusive_group()
    parser_dev_sample_exec_setup.add_argument("--bare", action="store_true")
    parser_dev_sample_exec_setup.add_argument("--virtme", action="store_true")
    parser_dev_sample.add_argument("--trace", action="store_true")
    parser_dev_sample.add_argument("--solution", action="store_true")

    parser_dev_e2e = sub_dev.add_parser("e2e")
    parser_dev_e2e.add_argument("name")
    parser_dev_e2e.add_argument("--solution")
    parser_dev_e2e.add_argument("--trace", action="store_true")

    parser_dev_check = sub_dev.add_parser("check")
    parser_dev_check.add_argument("--unit", action="store_true") # need confirmed

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
    parser_linux.add_argument("--harness")
    parser_linux.add_argument("--blob")
    parser_linux_exec_setup = parser_linux.add_mutually_exclusive_group()
    parser_linux_exec_setup.add_argument("--bare", action="store_true")
    parser_linux_exec_setup.add_argument("--virtme", action="store_true")
    parser_linux.add_argument("--corpus")
    parser_linux.add_argument("--output")
    parser_linux.add_argument("--trace", action="store_true")
    parser_linux.add_argument("--verbose", action="store_true")

    # actions
    args = parser.parse_args()

    # logging
    utils.enable_color_in_logging(args.verbose)

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

        if args.cmd_dev == "sample":
            cmd_dev_sample(
                args.volume,
                args.bare,
                args.virtme,
                args.trace,
                args.solution,
            )
        elif args.cmd_dev == "e2e":
            cmd_dev_e2e(args.name, args.solution, args.trace)
        elif args.cmd_dev == "check":
            cmd_dev_check(args.unit)
        else:
            parser_dev.print_help()

    elif args.command == "init":
        cmd_init(args.force)
    elif args.command == "build":
        cmd_build(args.incremental, args.release, args.deps_z3)
    elif args.command == "linux":
        if args.bare:
            setup = ExecSetup.Bare
        elif args.virtme:
            setup = ExecSetup.Virtme
        else:
            setup = ExecSetup.Simple

        cmd_linux(
            args.kernel,
            args.harness,
            args.blob,
            setup,
            args.corpus,
            args.output,
            args.trace,
            args.verbose,
        )
    else:
        parser.print_help()


if __name__ == "__main__":
    main()
