import os.path
import shutil
import subprocess
from pathlib import Path
from tempfile import TemporaryDirectory
from typing import Optional, List


class CpioWriter(object):
    def __init__(self, tmp: Path):
        self._tmp = tmp
        self._entries: List[str] = []

    def mkdir(self, name: str, mode: int = 0o755) -> None:
        self._tmp.joinpath(name).mkdir(mode)
        self._entries.append(name)

    def symlink(self, name: str, target: str) -> None:
        self._tmp.joinpath(name).symlink_to(target)
        self._entries.append(name)

    def copy_file(self, name: str, original: Path) -> None:
        shutil.copyfile(original, self._tmp.joinpath(name))
        shutil.copymode(original, self._tmp.joinpath(name))
        self._entries.append(name)

    def output(self, output: str) -> None:
        command = ["cpio", "-o", "-O", output, "-H", "newc"]
        name_list = "\n".join(self._entries) + "\n"
        subprocess.run(
            command, input=name_list.encode("utf-8"), cwd=self._tmp, check=True
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


def __assert_handled(dir_path: Path, link: str):
    if not link.startswith("/"):
        link = os.path.normpath(dir_path.joinpath(link))
    assert link.startswith("/")

    l1 = link.split("/")[1]
    if l1 in INCLUDED_ROOT_DIRS or l1 in CREATED_ROOT_DIRS or l1 in MOUNTED_ROOT_DIRS:
        return

    print("link {} is not handled".format(link))
    assert False


def cpio_recursive(cw: CpioWriter, dir_path: Path, item: str) -> None:
    path = Path("/").joinpath(item)
    if path.is_symlink():
        dst = os.readlink(path)
        cw.symlink(item, dst)
        # debugging purpose only
        __assert_handled(dir_path, dst)

    elif path.is_file():
        cw.copy_file(item, path)

    else:
        assert path.is_dir()
        cw.mkdir(item, mode=path.stat().st_mode)
        for entry in os.listdir(path):
            child = format("{}/{}".format(item, entry))
            cpio_recursive(cw, path, child)


def mk_initramfs_from_host_rootfs(cw: CpioWriter) -> None:
    # included directories
    for item in INCLUDED_ROOT_DIRS:
        cpio_recursive(cw, Path("/"), item)

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

    bin_busybox = subprocess.check_output(["which", "busybox"], text=True).strip()
    cw.copy_file("bin/busybox", Path(bin_busybox))

    for tool in [
        "cat",
        "cp",
        "ip",
        "ln",
        "mdev",
        "mkdir",
        "mknod",
        "mount",
        "mv",
        "rm",
        "sh",
        "sleep",
        "umount",
    ]:
        cw.symlink("bin/{}".format(tool), "busybox")


def mk_initramfs(
    out: str,
    agent: str,
    harness: Optional[str],
    blob: Optional[str],
    use_host_rootfs: bool,
) -> None:
    # constants
    block_size = 512
    image_size = 2 * 1024 * 1024 * 1024 if use_host_rootfs else 256 * 1024 * 1024

    with TemporaryDirectory() as tmp:
        # create an empty image
        fs_img = os.path.join(tmp, "ext4.img")
        subprocess.check_call(
            [
                "dd",
                "if=/dev/zero",
                "of={}".format(fs_img),
                "bs={}".format(block_size),
                "count={}".format(image_size // block_size),
            ]
        )
        subprocess.check_call(["mkfs.ext4", fs_img])

        # mount the
        fs_mnt = os.path.join(tmp, "mnt")
        os.mkdir(fs_mnt)
        subprocess.check_call(["mount", "-o", "loop", fs_img, fs_mnt])

        # fill content in the image
        cw = CpioWriter(Path(fs_mnt))

        # basic layout
        if use_host_rootfs:
            mk_initramfs_from_host_rootfs(cw)
        else:
            mk_initramfs_from_bare_rootfs(cw)

        # specialized
        cw.mkdir("home")
        if harness is not None:
            cw.copy_file("home/harness", Path(harness))
        if blob is not None:
            cw.copy_file("home/blob", Path(blob))

        # mark the agent as init
        cw.copy_file("init", Path(agent))

        # output as qcow2
        cw.output(out)

        # clean-up
        subprocess.check_call(["umount", fs_mnt])
