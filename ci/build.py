#!/usr/bin/env python3
# Copyright 2025, UNSW
# SPDX-License-Identifier: BSD-2-Clause

import argparse
import os
from pathlib import Path
import shutil
import subprocess
import sys
import contextlib

sys.path.insert(1, Path(__file__).parents[1].as_posix())

from ts_ci import log, ArgparseActionList, matrix_product
from ci import common, matrix


def get_example_dir(example_name: str):
    LIBVMM = Path(__file__).parents[1]
    return LIBVMM / "examples" / example_name


def build_make(args: argparse.Namespace, test_config: common.TestConfig):
    build_dir = common.example_build_path(test_config)
    example_dir = get_example_dir(test_config.example)

    subprocess.run(
        [
            "make",
            f"--jobs={args.num_jobs}",
            f"--directory={example_dir}",
            f"BUILD_DIR={build_dir}",
            f"MICROKIT_SDK={args.microkit_sdk}",
            f"MICROKIT_BOARD={test_config.board}",
            f"MICROKIT_CONFIG={test_config.config}",
        ],
        check=True,
    )


def build_zig(args: argparse.Namespace, test_config: common.TestConfig):
    build_dir = common.example_build_path(test_config)
    example_dir = get_example_dir(test_config.example)

    zig_env = os.environ.copy()
    zig_env["ZIG_GLOBAL_CACHE_DIR"] = str(common.CI_BUILD_DIR / "zig-cache")
    zig_env["ZIG_LOCAL_CACHE_DIR"] = str(common.CI_BUILD_DIR / "zig-cache")

    # Explicitly handle each config in case of unexpected future Microkit
    # configurations
    zig_optimize_table = {
        "debug": "Debug",
        "debug-smp": "Debug",
        "release": "ReleaseSafe",
        "release-smp": "ReleaseSafe",
        "benchmark": "ReleaseSafe",
        "benchmark-smp": "ReleaseSafe",
    }
    zig_optimize = zig_optimize_table[test_config.config]

    with contextlib.chdir(example_dir):
        subprocess.run(
            [
                "zig",
                "build",
                f"-Dsdk={args.microkit_sdk}",
                f"-Dboard={test_config.board}",
                f"-Dconfig={test_config.config}",
                f"-Doptimize={zig_optimize}",
                "-p",
                build_dir,
                f"-j{args.num_jobs}",
            ],
            check=True,
            env=zig_env,
        )


def build(args: argparse.Namespace, test_config: common.TestConfig):
    log.group_start(
        "building example '%s' for '%s' with microkit config '%s' and '%s'"
        % (
            test_config.example,
            test_config.board,
            test_config.config,
            test_config.build_system,
        )
    )

    build_dir = common.example_build_path(test_config)

    if not args.no_clean:
        try:
            shutil.rmtree(build_dir)
        except FileNotFoundError:
            pass

    if test_config.build_system == "make":
        build_make(args, test_config)
    elif test_config.build_system == "zig":
        build_zig(args, test_config)
    else:
        raise NotImplementedError(f"unknown build system '{test_config.build_system}'")

    log.group_end("build finished")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)

    parser.add_argument("microkit_sdk")
    parser.add_argument("num_jobs", nargs="?", type=int, default=os.cpu_count())
    parser.add_argument(
        "--examples",
        default=set(matrix.EXAMPLES.keys()),
        action=ArgparseActionList,
    )
    parser.add_argument(
        "--no-clean",
        action="store_true",
        help="Do not remove any pre-existing CI build directory before building",
    )

    args = parser.parse_args()

    for example_name, options in matrix.EXAMPLES.items():
        if example_name not in args.examples:
            continue

        example_matrix = matrix_product(
            common.TestConfig,
            test=[None],
            example=[example_name],
            board=options["boards"],
            config=options["configs"],
            build_system=options["build_systems"],
            test_fn=[None],
            backend_fn=[None],
            no_output_timeout_s=[None],
        )
        for test_config in example_matrix:
            build(args, test_config)
