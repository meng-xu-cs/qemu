#!/usr/bin/env python3

import argparse
import multiprocessing
import os
import shutil
import subprocess
import tempfile

from run import PATH_TESTS_KERNEL


def build_kernel(path_repo: str, path_output: str, menu: bool, force: bool):
    # clean up the repository if requested
    if force:
        # clean up the repository
        subprocess.check_call(["git", "clean", "-fdx"], cwd=path_repo)
        subprocess.check_call(["git", "checkout", "--", "."], cwd=path_repo)

        # read up the config files
        with open(os.path.join(PATH_TESTS_KERNEL, "config"), "r") as f:
            content = [line.strip() for line in f]

        # piggyback on the tiny.config file
        path_tiny_config = os.path.join(
            path_repo, "arch", "x86", "configs", "tiny.config"
        )
        assert os.path.exists(path_tiny_config)
        with open(path_tiny_config, "a") as f:
            f.write("\n# auto-appended by QCE config\n")
            for line in content:
                f.write(line + "\n")

    # all build occur in a temp directory
    with tempfile.TemporaryDirectory() as tmp:
        # generate the config
        subprocess.check_call(
            ["make", "ARCH=x86_64", f"O={tmp}", "tinyconfig"],
            cwd=path_repo,
        )

        # customize the config if needed
        if menu:
            subprocess.check_call(
                ["make", "ARCH=x86_64", f"O={tmp}", "menuconfig"],
                cwd=path_repo,
            )

        # build the kernel
        subprocess.check_call(
            [
                "make",
                "ARCH=x86_64",
                f"O={tmp}",
                "-j{}".format(multiprocessing.cpu_count()),
            ],
            cwd=path_repo,
        )

        # copy over the image
        shutil.copy2(
            os.path.join(
                tmp,
                "arch",
                "x86_64",
                "boot",
                "bzImage",
            ),
            path_output,
        )


def main() -> None:
    # args
    parser = argparse.ArgumentParser()
    parser.add_argument("--repo", required=True)
    parser.add_argument("--output", required=True)
    parser.add_argument("--menu", action="store_true")
    parser.add_argument("--force", action="store_true")
    args = parser.parse_args()

    # build the linux kernel
    build_kernel(args.repo, args.output, args.menu, args.force)


if __name__ == "__main__":
    main()
