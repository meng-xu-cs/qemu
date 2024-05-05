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

        namebytes = name.encode(TEXT_ENCODING)
        if b"\0" in namebytes:
            raise ValueError("Filename cannot contain a NUL")

        namesize = len(namebytes) + 1
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
        self.__write(namebytes)
        self.__write(b"\0")
        self.__write(((2 - namesize) % 4) * b"\0")
        self.__write(body)
        self.__write(((-filesize) % 4) * b"\0")

    def write_trailer(self) -> None:
        self.write_object(name="TRAILER!!!", body=b"", mode=0, ino=0, nlink=1)
        self.__write(((-self.__totalsize) % 512) * b"\0")

    def mkdir(self, name: str, mode: int = 0o755) -> None:
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


def cpio_copy_with_mode(cw: CpioWriter, src: str, dst: str) -> None:
    stat = os.stat(src)
    with open(src, "rb") as f:
        cw.write_file(dst, body=f.read(), mode=stat.st_mode)


def mk_initramfs_from_host_rootfs(cw: CpioWriter) -> None:
    # included directories
    for item in INCLUDED_ROOT_DIRS:
        cpio_recursive(cw, "/", item)

    # created directories
    for item in CREATED_ROOT_DIRS:
        cw.mkdir(item)
    for item in MOUNTED_ROOT_DIRS:
        cw.mkdir(item)


def mk_initramfs_from_bare_rootfs(cw: CpioWriter) -> None:
    # base layout
    for name in MOUNTED_ROOT_DIRS:
        cw.mkdir(name)
    for name in CREATED_ROOT_DIRS:
        cw.mkdir(name)

    # prepare for busybox
    cw.mkdir("bin")

    path_busybox = subprocess.check_output(["which", "busybox"], text=True).strip()
    with open(path_busybox, "rb") as f_busybox:
        cw.write_file("bin/busybox", body=f_busybox.read(), mode=0o755)

    for tool in [
        "cat",
        "cp",
        "ip",
        "ln",
        "mkdir",
        "mknod",
        "mount",
        "mv",
        "rm",
        "sh",
        "sleep",
        "umount",
    ]:
        cw.symlink("busybox", "bin/{}".format(tool))


def mk_initramfs(
    out: str,
    agent: str,
    harness: Optional[str],
    blob: Optional[str],
    use_host_rootfs: bool,
) -> None:
    with open(out, "w+b") as f:
        cw = CpioWriter(f)

        # rootfs
        if use_host_rootfs:
            mk_initramfs_from_host_rootfs(cw)
        else:
            mk_initramfs_from_bare_rootfs(cw)

        # specialized
        cw.mkdir("home")
        if harness is not None:
            cpio_copy_with_mode(cw, harness, "home/harness")
        if blob is not None:
            cpio_copy_with_mode(cw, blob, "home/blob")

        # mark the agent as init
        cpio_copy_with_mode(cw, agent, "init")

        # done
        cw.write_trailer()
