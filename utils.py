import os
import subprocess
import sys
import tempfile

from typing import BinaryIO, Optional, Tuple


class CpioWriter(object):
    TYPE_DIR = 0o0040000
    TYPE_REG = 0o0100000
    TYPE_SYMLINK = 0o0120000
    TYPE_CHRDEV = 0o0020000
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

        if isinstance(body, bytes):
            filesize = len(body)
        else:
            filesize = body.seek(0, 2)
            body.seek(0)

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
        hdr = ("070701" + "".join("%08X" % f for f in fields)).encode("ascii")

        self.__write(hdr)
        self.__write(name.encode("ascii"))
        self.__write(b"\0")
        self.__write(((2 - namesize) % 4) * b"\0")

        if isinstance(body, bytes):
            self.__write(body)
        else:
            while True:
                buf = body.read(65536)
                if buf == b"":
                    break
                self.__write(buf)

        self.__write(((-filesize) % 4) * b"\0")

    def write_trailer(self) -> None:
        self.write_object(name="TRAILER!!!", body=b"", mode=0, ino=0, nlink=1)
        self.__write(((-self.__totalsize) % 512) * b"\0")

    def mkdir(self, name: str, mode: int) -> None:
        self.write_object(name=name, mode=CpioWriter.TYPE_DIR | mode, body=b"")

    def symlink(self, src: str, dst: str) -> None:
        self.write_object(
            name=dst, mode=CpioWriter.TYPE_SYMLINK | 0o777, body=src.encode("ascii")
        )

    def write_file(self, name: str, body: bytes, mode: int) -> None:
        self.write_object(name=name, body=body, mode=CpioWriter.TYPE_REG | mode)

    def mkchardev(self, name: str, dev: Tuple[int, int], mode: int) -> None:
        major, minor = dev
        self.write_object(
            name=name,
            mode=CpioWriter.TYPE_CHRDEV | mode,
            rdevmajor=major,
            rdevminor=minor,
            body=b"",
        )


def mk_initramfs(out: str, harness: Optional[str], blob: Optional[str]) -> None:
    with open(out, "w+b") as f:
        cw = CpioWriter(f)

        # base layout
        for name in ("bin", "var", "etc", "dev", "proc", "prog"):
            cw.mkdir(name, 0o755)

        # dev nodes
        cw.mkchardev("dev/null", (1, 3), mode=0o666)
        cw.mkchardev("dev/kmsg", (1, 11), mode=0o666)
        cw.mkchardev("dev/console", (5, 1), mode=0o660)

        # prepare for busybox
        path_busybox = subprocess.check_output(["which", "busybox"], text=True).strip()
        with open(path_busybox, "rb") as f_busybox:
            cw.write_file("bin/busybox", body=f_busybox.read(), mode=0o755)

        for tool in ("sh", "mount", "umount", "sleep", "mkdir", "cp", "cat"):
            cw.symlink("busybox", "bin/{}".format(tool))

        # generate init
        if harness is None:
            # shell mode
            assert blob is None
            init_cmdline = "sh"
        else:
            assert os.path.exists(harness)
            with tempfile.TemporaryDirectory() as tmpdir:
                harness_bin = os.path.join(tmpdir, "harness")
                subprocess.check_call(["cc", "-static", harness, "-o", harness_bin])
                with open(harness_bin, "rb") as f_harness:
                    cw.write_file("prog/harness", body=f_harness.read(), mode=0o755)

            if blob is None:
                # fuzzing mode
                sys.exit("not supported")
            else:
                # testing mode
                assert os.path.exists(blob)
                with open(blob, "rb") as f_blob:
                    cw.write_file("prog/blob", body=f_blob.read(), mode=0o644)

                init_cmdline = "echo '[harness] {}'\n/prog/harness /prog/blob".format(
                    harness
                )

        cw.write_file(
            "init",
            body="#!/bin/sh\n{}".format(init_cmdline).encode("ascii"),
            mode=0o755,
        )

        # done
        cw.write_trailer()
