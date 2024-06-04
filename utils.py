import guestfs  # type: ignore
import os.path
import shutil
import subprocess
from pathlib import Path
from tempfile import TemporaryDirectory
from typing import Optional, List


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


class RootfsWriter(object):
    def __init__(self, tmp: str):
        self.g = guestfs.GuestFS(python_return_dict=True)
        self.g.add_drive_opts(tmp, format="raw", readonly=False)
        self.g.launch()
        self.g.mount_options("", "/dev/sda", "/")

    def mkdir(self, name: str, mode: int = 0o755) -> None:
        # print("mkdir {}".format(name))
        self.g.mkdir_mode("/" + name, mode)

    def symlink(self, name: str, target: str) -> None:
        # print("symlink {} -> {}".format(target, name))
        self.g.ln("/" + target, "/" + name)

    def copy_file(self, name: str, original: Path) -> None:
        # read original file
        # print("copy_file {} -> {}".format(original, name))
        with open(original, "rb") as f:
            content = f.read()
        self.g.write("/" + name, content)
        # for convenience, 755 here
        # TODO: find and set the correct mode
        self.g.chmod(0o755, "/" + name)

    def write_file(self, name: str, body: bytes, mode: int) -> None:
        # print("write_file {} -> {}".format(body, name))
        self.g.write("/" + name, body)
        self.g.chmod(mode, "/" + name)

    def cleanup(self):
        self.g.umount_all()
        self.g.close()


def __assert_handled(dir_path: Path, link: str):
    if not link.startswith("/"):
        link = os.path.normpath(dir_path.joinpath(link))
    assert link.startswith("/")

    l1 = link.split("/")[1]
    if l1 in INCLUDED_ROOT_DIRS or l1 in CREATED_ROOT_DIRS or l1 in MOUNTED_ROOT_DIRS:
        return

    print("link {} is not handled".format(link))
    assert False


def cp_rootfs_recursive(cw: RootfsWriter, dir_path: Path, item: str) -> None:
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
            cp_rootfs_recursive(cw, path, child)


def mk_rootfs_from_host_rootfs(cw: RootfsWriter) -> None:
    # included directories
    for item in INCLUDED_ROOT_DIRS:
        cp_rootfs_recursive(cw, Path("/"), item)

    # created directories
    for item in CREATED_ROOT_DIRS:
        cw.mkdir(item)
    for item in MOUNTED_ROOT_DIRS:
        cw.mkdir(item)


def mk_rootfs_from_bare_rootfs(cw: RootfsWriter) -> None:
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
        cw.symlink("bin/{}".format(tool), "bin/busybox")


def mk_rootfs(
    qemu_img: str,
    qcow_disk: str,
    qcow_size: str,
    agent: str,
    harness: Optional[str],
    blob: Optional[str],
    use_host_rootfs: bool,
) -> None:
    # constants
    block_size = 512
    image_size = 1 * 1024 * 1024 * 1024 if use_host_rootfs else 256 * 1024 * 1024

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

        # mount the filesystem
        fs_mnt = os.path.join(tmp, "mnt")
        os.mkdir(fs_mnt)

        # fill content in the image
        cw = RootfsWriter(fs_img)

        # basic layout
        if use_host_rootfs:
            mk_rootfs_from_host_rootfs(cw)
        else:
            mk_rootfs_from_bare_rootfs(cw)

        # specialized
        cw.mkdir("root")
        cw.copy_file("root/agent", Path(agent))
        if harness is not None:
            cw.copy_file("root/harness", Path(harness))
        if blob is not None:
            cw.copy_file("root/blob", Path(blob))

        # umount the filesystem
        cw.cleanup()

        # output as qcow2
        subprocess.check_call(
            [
                qemu_img,
                "convert",
                "-O",
                "qcow2",
                fs_img,
                qcow_disk,
            ]
        )

        subprocess.check_call(
            [
                qemu_img,
                "resize",
                qcow_disk,
                qcow_size,
            ]
        )


class CpioWriter(object):
    def __init__(self, tmp: Path):
        self._tmp = tmp
        self._entries: List[str] = []

    def mkdir(self, name: str, mode: int = 0o755) -> None:
        self._tmp.joinpath(name).mkdir(mode)
        self._entries.append(name)

    def mknod(self, name: str, major: int, minor: int, mode: int) -> None:
        os.mknod(self._tmp.joinpath(name), mode, os.makedev(major, minor))
        self._entries.append(name)

    def symlink(self, name: str, target: str) -> None:
        self._tmp.joinpath(name).symlink_to(target)
        self._entries.append(name)

    def copy_file(self, name: str, original: Path) -> None:
        shutil.copyfile(original, self._tmp.joinpath(name))
        shutil.copymode(original, self._tmp.joinpath(name))
        self._entries.append(name)

    def write_file(self, name: str, body: bytes, mode: int) -> None:
        path = self._tmp.joinpath(name)
        path.write_bytes(body)
        path.chmod(mode)
        self._entries.append(name)

    def output(self, output: str) -> None:
        command = ["cpio", "-o", "-O", output, "-H", "newc"]
        name_list = "\n".join(self._entries) + "\n"
        subprocess.run(
            command, input=name_list.encode("utf-8"), cwd=self._tmp, check=True
        )


INIT_SCRIPT = r"""#!/bin/sh
if ! /bin/mount -n -t devtmpfs -o mode=0755,nosuid,noexec devtmpfs /dev/; then
    echo "failed to mount /dev"
    exit 1
fi

if ! /bin/mount -n -t ext4 -o rw,suid,dev,exec,sync /dev/vda /new_root/; then
    echo "failed to mount real root"
    exit 1
fi

if ! umount /dev; then
    echo "failed to umount /dev"
    exit 1
fi

exec /bin/switch_root /new_root /root/agent
"""


def mk_initramfs(out: str) -> None:
    with TemporaryDirectory() as tmp:
        cw = CpioWriter(Path(tmp))

        # base layout
        cw.mkdir("bin")
        cw.mkdir("dev")
        cw.mkdir("new_root")

        # prepare for busybox
        bin_busybox = subprocess.check_output(["which", "busybox"], text=True).strip()
        cw.copy_file("bin/busybox", Path(bin_busybox))

        for tool in ["sh", "mount", "umount", "switch_root"]:
            cw.symlink("bin/{}".format(tool), "/bin/busybox")

        # write the init script
        cw.write_file("init", INIT_SCRIPT.encode("utf-8"), 0o755)

        # done
        cw.output(out)


def patch_harness(src: str, dst: str) -> None:
    with open(src) as f:
        content = f.read()

    with open(dst, "w") as f:
        f.write('#include "init.c"\n')
        f.write(content)
