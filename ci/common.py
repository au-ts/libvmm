#!/usr/bin/env python3
# Copyright 2025, UNSW
# SPDX-License-Identifier: BSD-2-Clause

from pathlib import Path
import sys
from dataclasses import dataclass

sys.path.insert(1, Path(__file__).parents[1].as_posix())

from ts_ci import (
    HardwareBackend,
    QemuBackend,
    MachineQueueBackend,
    MACHINE_QUEUE_BOARDS,
    MACHINE_QUEUE_BOARD_OPTIONS,
    TestConfig,
)

CI_BUILD_DIR = Path(__file__).parents[1] / "ci_build"


def example_build_path(test_config: TestConfig):
    return (
        CI_BUILD_DIR
        / "examples"
        / test_config.example
        / test_config.build_system
        / test_config.board
        / test_config.config
    )


def loader_img_path(
    test_config: TestConfig,
):
    return (
        example_build_path(test_config)
        / ("bin" if test_config.build_system == "zig" else "")
        / "loader.img"
    )


def backend_fn(
    test_config: TestConfig,
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
