import os.path
import re
import shutil
import subprocess
import sys
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
    def __init__(self, tmp: Path):
        self._tmp = tmp

    def mkdir(self, name: str, mode: int = 0o755) -> None:
        self._tmp.joinpath(name).mkdir(mode)

    def symlink(self, name: str, target: str) -> None:
        self._tmp.joinpath(name).symlink_to(target)

    def copy_file(self, name: str, original: Path) -> None:
        shutil.copyfile(original, self._tmp.joinpath(name))
        shutil.copymode(original, self._tmp.joinpath(name))

    def write_file(self, name: str, body: bytes, mode: int) -> None:
        path = self._tmp.joinpath(name)
        path.write_bytes(body)
        path.chmod(mode)


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
        cw.symlink("bin/{}".format(tool), "busybox")


def mk_rootfs(
    qemu_img: str,
    qcow_disk: str,
    qcow_size: int,
    agent: str,
    harness: Optional[str],
    blob: Optional[str],
    use_host_rootfs: bool,
) -> None:
    # constants
    block_size = 512

    with TemporaryDirectory() as tmp:
        # create an empty image
        fs_img = os.path.join(tmp, "ext4.img")
        subprocess.check_call(
            [
                "dd",
                "if=/dev/zero",
                "of={}".format(fs_img),
                "bs={}".format(block_size),
                "count={}".format(qcow_size // block_size),
            ]
        )
        subprocess.check_call(["mkfs.ext4", fs_img])

        # mount the filesystem
        fs_mnt = os.path.join(tmp, "mnt")
        os.mkdir(fs_mnt)
        subprocess.check_call(["mount", "-o", "loop", fs_img, fs_mnt])

        # fill content in the image
        cw = RootfsWriter(Path(fs_mnt))

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
        subprocess.check_call(["umount", fs_mnt])

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
            cw.symlink("bin/{}".format(tool), "busybox")

        # write the init script
        cw.write_file("init", INIT_SCRIPT.encode("utf-8"), 0o755)

        # done
        cw.output(out)


def patch_harness(src: str, dst: str) -> None:
    pattern_decl = re.compile(
        r"(?P<ret_type>\w+)\s+"
        r"harness\s*\(\s*"
        r"(?P<blob_type>\w+)\s*\*\s*(?P<blob_name>\w+)"
        r"\s*,\s*"
        r"(?P<size_type>\w+)\s+(?P<size_name>\w+)"
        r"\s*\)\s*\{",
        re.MULTILINE,
    )
    pattern_call = re.compile(
        r"\s+harness\s*\(\s*"
        r"(?P<blob_arg>.+?)"
        r"\s*,\s*"
        r"(?P<size_arg>.+?)"
        r"\s*\)\s*;",
        re.MULTILINE,
    )

    with open(src) as f:
        content = f.read()

    # locate the harness function
    match_decl = pattern_decl.search(content)
    if match_decl is None:
        sys.exit("Unable to find the harness function decl")

    match_call = pattern_call.search(content)
    if match_call is None:
        sys.exit("Unable to find the harness function call")

    # check types
    blob_type = match_decl["blob_type"]
    if blob_type not in ["char", "uint8_t", "int8_t"]:
        sys.exit("Unrecognized blob type: {}*".format(blob_type))

    size_type = match_decl["size_type"]
    if size_type not in [
        "int",
        "long",
        "unsigned",
        "size_t",
        "ssize_t",
        "int32_t",
        "uint32_t",
        "int64_t",
        "uint64_t",
    ]:
        sys.exit("Unrecognized size type: {}*".format(size_type))

    # enrich with the marker
    repl = """
({{
long __r = 1;
{}* __blob = {};
{} __size = {};
asm volatile ("encls"
    : "=a"(__r)
    : "a"(0x5), "b"(__size), "c"(__blob)
    : "memory");
if (__r) {{ exit(1); }}
harness(__blob, __size);
}});
""".format(
        blob_type, match_call["blob_arg"], size_type, match_call["size_arg"]
    )
    replaced = pattern_call.sub(repl, content)

    # dump the updated content
    with open(dst, "w") as f:
        f.write(replaced)
