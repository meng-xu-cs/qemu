import os.path
import subprocess
from typing import BinaryIO, Optional, Tuple


TEXT_ENCODING = "utf-8"


class CpioWriter(object):
    TYPE_DIR = 0o0040000
    TYPE_REG = 0o0100000
    TYPE_SYMLINK = 0o0120000
    TYPE_CHARDEV = 0o0020000
    TYPE_MASK = 0o0170000

    def __init__(self, f: BinaryIO):
        self.__f = f
        self.__totalsize = 0
        self.__next_ino = 0

    def __write(self, data: bytes) -> None:
        self.__f.write(data)
        self.__totalsize += len(data)

    def write_object(
        self,
        name: str,
        body: bytes,
        mode: int,
        ino: Optional[int] = None,
        nlink: Optional[int] = None,
        uid: int = 0,
        gid: int = 0,
        mtime: int = 0,
        devmajor: int = 0,
        devminor: int = 0,
        rdevmajor: int = 0,
        rdevminor: int = 0,
    ) -> None:
        if nlink is None:
            nlink = 2 if (mode & CpioWriter.TYPE_MASK) == CpioWriter.TYPE_DIR else 1

        namesize = len(name) + 1
        filesize = len(body)

        if ino is None:
            ino = self.__next_ino
            self.__next_ino += 1

        fields = [
            ino,
            mode,
            uid,
            gid,
            nlink,
            mtime,
            filesize,
            devmajor,
            devminor,
            rdevmajor,
            rdevminor,
            namesize,
            0,
        ]
        hdr = ("070701" + "".join("%08X" % f for f in fields)).encode()

        self.__write(hdr)
        self.__write(name.encode(TEXT_ENCODING))
        self.__write(b"\0")
        self.__write(((2 - namesize) % 4) * b"\0")
        self.__write(body)
        self.__write(((-filesize) % 4) * b"\0")

    def write_trailer(self) -> None:
        self.write_object(name="TRAILER!!!", body=b"", mode=0, ino=0, nlink=1)
        self.__write(((-self.__totalsize) % 512) * b"\0")

    def mkdir(self, name: str, mode: int) -> None:
        self.write_object(name=name, mode=CpioWriter.TYPE_DIR | mode, body=b"")

    def symlink(self, src: str, dst: str) -> None:
        self.write_object(
            name=dst,
            mode=CpioWriter.TYPE_SYMLINK | 0o777,
            body=src.encode(TEXT_ENCODING),
        )

    def write_file(self, name: str, body: bytes, mode: int) -> None:
        self.write_object(name=name, body=body, mode=CpioWriter.TYPE_REG | mode)

    def mkchardev(self, name: str, dev: Tuple[int, int], mode: int) -> None:
        major, minor = dev
        self.write_object(
            name=name,
            mode=CpioWriter.TYPE_CHARDEV | mode,
            rdevmajor=major,
            rdevminor=minor,
            body=b"",
        )


INCLUDED_ROOT_DIRS = [
    "bin",
    "etc",
    "lib",
    "lib32",
    "lib64",
    "libx32",
    "sbin",
    "usr",
]

CREATED_ROOT_DIRS = [
    "mnt",
    "var",
]

MOUNTED_ROOT_DIRS = [
    "dev",
    "proc",
    "run",
    "sys",
    "tmp",
]


def __assert_handled(dir_path: str, link: str):
    if not link.startswith("/"):
        link = os.path.normpath(os.path.join(dir_path, link))
    assert link.startswith("/")

    l1 = link.split("/")[1]
    if l1 in INCLUDED_ROOT_DIRS or l1 in CREATED_ROOT_DIRS or l1 in MOUNTED_ROOT_DIRS:
        return

    print("link {} is not handled".format(link))
    assert False


def cpio_recursive(cw: CpioWriter, dir_path: str, item: str) -> None:
    path = os.path.join("/", item)
    if os.path.islink(path):
        dst = os.readlink(path)
        cw.symlink(item, dst)
        # debugging purpose only
        __assert_handled(dir_path, dst)

    else:
        stat = os.stat(path)
        if os.path.isfile(path):
            with open(path, "rb") as f:
                cw.write_file(item, f.read(), stat.st_mode)
        else:
            assert os.path.isdir(path)
            cw.mkdir(item, stat.st_mode)
            for entry in os.listdir(path):
                child = format("{}/{}".format(item, entry))
                cpio_recursive(cw, path, child)


def mk_initramfs(
    out: str, harness: Optional[str], agent: Optional[str], blob: Optional[str]
) -> None:
    with open(out, "w+b") as f:
        # included directories
        cw = CpioWriter(f)
        for item in INCLUDED_ROOT_DIRS:
            cpio_recursive(cw, "/", item)

        # created directories
        for item in CREATED_ROOT_DIRS:
            cw.mkdir(item, 0o755)
        for item in MOUNTED_ROOT_DIRS:
            cw.mkdir(item, 0o755)

        # done
        cw.write_trailer()


def mk_initramfs2(
    out: str, harness: Optional[str], agent: Optional[str], blob: Optional[str]
) -> None:
    with open(out, "w+b") as f:
        cw = CpioWriter(f)

        # base layout
        for name in ("lib", "bin", "var", "etc", "dev", "proc", "sys", "tmp"):
            cw.mkdir(name, 0o755)

        cw.symlink("bin", "sbin")
        cw.symlink("lib", "lib64")

        # dev nodes
        cw.mkchardev("dev/null", (1, 3), mode=0o666)
        cw.mkchardev("dev/kmsg", (1, 11), mode=0o666)
        cw.mkchardev("dev/console", (5, 1), mode=0o660)

        # prepare for busybox
        path_busybox = subprocess.check_output(["which", "busybox"], text=True).strip()
        with open(path_busybox, "rb") as f_busybox:
            cw.write_file("bin/busybox", body=f_busybox.read(), mode=0o755)

        for tool in ("sh", "mount", "umount", "sleep", "mkdir", "mknod", "cp", "cat"):
            cw.symlink("busybox", "bin/{}".format(tool))

        # generate init
        if harness is None:
            # shell mode
            assert blob is None
            assert agent is None
            init_cmdline = "sh"
        else:
            cw.mkdir("prog", 0o755)
            with open(harness, "rb") as f_harness:
                cw.write_file("prog/harness", body=f_harness.read(), mode=0o755)

            if blob is None:
                # fuzzing mode
                assert agent is not None
                with open(agent, "rb") as f_agent:
                    cw.write_file("prog/agent", body=f_agent.read(), mode=0o755)
                init_cmdline = "echo '[harness-fuzz] {}'\n/prog/agent /prog/harness /prog/blob".format(
                    harness
                )
            else:
                # testing mode
                assert agent is None
                with open(blob, "rb") as f_blob:
                    cw.write_file("prog/blob", body=f_blob.read(), mode=0o644)
                init_cmdline = (
                    "echo '[harness-test] {}'\n/prog/harness /prog/blob".format(harness)
                )

        cw.write_file(
            "init",
            body="#!/bin/sh\n{}".format(init_cmdline).encode(TEXT_ENCODING),
            mode=0o755,
        )

        # done
        cw.write_trailer()
