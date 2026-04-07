#!/usr/bin/env python3
# Copyright 2025, UNSW
# SPDX-License-Identifier: BSD-2-Clause

from __future__ import annotations
import argparse
from datetime import datetime
from dataclasses import dataclass
from pathlib import Path
import itertools
import sys
import os
import subprocess
import tempfile
from typing import Callable, Awaitable
import types

sys.path.insert(1, Path(__file__).parents[1].as_posix())

from ts_ci import (
    add_runner_arguments,
    apply_runner_arguments,
    execute_tests,
    ArgparseActionList,
    HardwareBackend,
    QemuBackend,
    MachineQueueBackend,
    MACHINE_QUEUE_BOARDS,
    MACHINE_QUEUE_BOARD_OPTIONS,
    TestCase,
)

CI_BUILD_DIR = Path(__file__).parents[1] / "ci_build"
LIBVMM = Path(__file__).parents[1]
mkvirtdisk = (LIBVMM / "dep" / "sddf" / "tools" / "mkvirtdisk").resolve()


def example_build_path(test_config: TestConfig):
    return (
        CI_BUILD_DIR
        / "examples"
        / test_config.example
        / test_config.build_system
        / test_config.board
        / test_config.config
    )


def backend_fn(
    test_config: TestCase,
    loader_img: Path,
) -> HardwareBackend:

    if test_config.is_qemu():
        QEMU_COMMON_FLAGS = (
            # fmt: off
            "-m", "size=2G",
            "-serial", "mon:stdio",
            "-nographic",
            "-d", "guest_errors",
            # fmt: on
        )

        if test_config.board == "qemu_virt_aarch64":
            return QemuBackend(
                "qemu-system-aarch64",
                # fmt: off
                "-machine", "virt,virtualization=on,highmem=off,secure=off",
                "-cpu", "cortex-a53",
                "-device", f"loader,file={loader_img.resolve()},addr=0x70000000,cpu-num=0",
                # fmt: on
                *QEMU_COMMON_FLAGS,
            )
        else:
            raise NotImplementedError(f"unknown qemu board {test_config.board}")

    else:
        mq_boards: list[str] = MACHINE_QUEUE_BOARDS[test_config.board]
        options = MACHINE_QUEUE_BOARD_OPTIONS.get(test_config.board, {})
        return MachineQueueBackend(loader_img.resolve(), mq_boards, **options)


def virtio_backend_fn(test_config: TestConfig, loader_img: Path) -> HardwareBackend:
    backend = backend_fn(test_config, loader_img)

    if isinstance(backend, QemuBackend):
        tmpdir = tempfile.TemporaryDirectory(suffix="libvmm_blk_disks")

        fd, disk_path = tempfile.mkstemp(dir=tmpdir.name)
        os.close(fd)

        subprocess.run(
            [mkvirtdisk, disk_path, "1", "512", "16777216", "GPT"],
            check=True,
            capture_output=True,
        )

        if test_config.board == "x86_64_generic":
            virtio_blk_device = "virtio-blk-pci,drive=hd,addr=0x3.0"
            virtio_net_device = "virtio-net-pci,netdev=netdev0,addr=0x2.0"
        else:
            virtio_blk_device = "virtio-blk-device,drive=hd,bus=virtio-mmio-bus.1"
            virtio_net_device = "virtio-net-device,netdev=netdev0,bus=virtio-mmio-bus.0"

        # fmt: off
        backend.invocation_args.extend([
            "-global", "virtio-mmio.force-legacy=false",
            "-drive", "file={},if=none,format=raw,id=hd".format(disk_path),
            "-device", virtio_blk_device,
            "-device", virtio_net_device,
            "-netdev", "user,id=netdev0,"
        ])
        # fmt: on

        orig_stop = backend.stop

        async def stop_with_cleanup(self):
            try:
                await orig_stop()
            finally:
                tmpdir.cleanup()

        backend.stop = types.MethodType(stop_with_cleanup, backend)

    return backend


TestFunction = Callable[[HardwareBackend, "TestConfig"], Awaitable[None]]
BackendFunction = Callable[["TestConfig", Path], HardwareBackend]


# Implements 'TestCase' protocol
@dataclass(order=True, frozen=True)
class TestConfig(TestCase):
    test: str
    example: str
    board: str
    config: str
    build_system: str

    test_fn: TestFunction
    backend_fn: BackendFunction
    no_output_timeout_s: int

    def is_qemu(self):
        # TODO: x86_64_generic assumes QEMU for the moment.
        return self.board.startswith("qemu") or self.board == "x86_64_generic"

    def pretty_name(self) -> str:
        return f"{self.test} for {self.example} on {self.board} ({self.config}, built with {self.build_system})"

    def backend(self, loader_img: Path) -> HardwareBackend:
        return self.backend_fn(self, loader_img)

    def loader_img(self) -> Path:
        return (
            example_build_path(self)
            / ("bin" if self.build_system == "zig" else "")
            / "loader.img"
        )

    async def run(self, backend: HardwareBackend) -> None:
        await self.test_fn(backend, self)

    def log_file_path(self, logs_dir: Path, now: datetime) -> Path:
        return (
            logs_dir
            / self.test
            / self.example
            / self.board
            / self.config
            / self.build_system
            / f"{now.strftime('%Y-%m-%d_%H.%M.%S')}.log"
        )


def test_case_summary(tests: list[TestConfig]):
    if len(tests) == 0:
        return "   (none)"

    lines = []
    for test, subtests in itertools.groupby(tests, key=lambda c: c.test):
        lines.append(f"--- Test: {test} ---")

        for ex, subsubtests in itertools.groupby(subtests, key=lambda c: c.example):
            lines.append(f"  ~~~ Example: {ex} ~~~")

            for board, group in itertools.groupby(subsubtests, key=lambda c: c.board):
                lines.append(
                    "    - {}: {}".format(
                        board, ", ".join(f"{c.config}/{c.build_system}" for c in group)
                    )
                )

    return "\n".join(lines)


def subset_test_cases(
    tests: list[TestConfig], filters: argparse.Namespace
) -> list[TestConfig]:
    # This works under the assumption that all elements of tests are the same
    # subset of TestConfig.

    def filter_check(test: TestConfig):
        implies = lambda a, b: not a or b
        return all(
            [
                (test.test in filters.tests),
                (test.example in filters.examples),
                (test.board in filters.boards),
                (test.config in filters.configs),
                (test.build_system in filters.build_systems),
                (implies(filters.only_qemu is True, test.is_qemu())),
                (implies(filters.only_qemu is False, not test.is_qemu())),
            ]
        )

    return list(sorted(set(filter(filter_check, tests))))


def run_tests(tests: list[TestConfig]) -> None:
    parser = argparse.ArgumentParser(description="Run tests")

    filters = parser.add_argument_group(title="filters")
    filters.add_argument(
        "--tests", default={test.test for test in tests}, action=ArgparseActionList
    )
    filters.add_argument(
        "--examples",
        default={test.example for test in tests},
        action=ArgparseActionList,
    )
    filters.add_argument(
        "--boards", default={test.board for test in tests}, action=ArgparseActionList
    )
    filters.add_argument(
        "--configs", default={test.config for test in tests}, action=ArgparseActionList
    )
    filters.add_argument(
        "--build-systems",
        default={test.build_system for test in tests},
        action=ArgparseActionList,
    )
    filters.add_argument(
        "--only-qemu",
        action=argparse.BooleanOptionalAction,
        help="select only QEMU tests",
    )

    add_runner_arguments(parser)

    args = parser.parse_args()

    filter_args = argparse.Namespace(
        **{a.dest: getattr(args, a.dest) for a in filters._group_actions}
    )

    tests = subset_test_cases(tests, filter_args)

    tests = apply_runner_arguments(parser, args, tests, test_case_summary)

    execute_tests(tests, args, test_case_summary)
