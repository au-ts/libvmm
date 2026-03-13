#!/usr/bin/env python3
# Copyright 2025, UNSW
# SPDX-License-Identifier: BSD-2-Clause

from __future__ import annotations
from itertools import chain
from typing import TYPE_CHECKING, Any, Literal, Optional, Sequence, TypedDict

from ts_ci import MACHINE_QUEUE_BOARDS, matrix_product
from . import common
from .common import TestConfig, TestFunction, BackendFunction

# longer because release mode image unpacking of vmms
NO_OUTPUT_DEFAULT_TIMEOUT_S: int = 300


def generate_example_test_cases(
    test: str,
    example_names: list[str],
    test_fn: TestFunction,
    backend_fn: BackendFunction,
    no_output_timeout_s: int,
) -> list[TestConfig]:
    def listify(s: str | Sequence[str]) -> Sequence[str]:
        if isinstance(s, str):
            return [s]
        else:
            return s

    examples_matrices = [EXAMPLES[ex] for ex in example_names]

    test_cases = set()

    for example in example_names:
        example_matrix = EXAMPLES[example]

        test_cases |= set(
            matrix_product(
                TestConfig,
                test=[test],
                example=[example],
                board=example_matrix["boards"],
                config=example_matrix["configs"],
                build_system=example_matrix["build_systems"],
                test_fn=[test_fn],
                backend_fn=[backend_fn],
                no_output_timeout_s=[no_output_timeout_s],
            )
        )

        for exclude in example_matrix["tests_exclude"]:
            test_cases -= set(
                matrix_product(
                    TestConfig,
                    test=[test],
                    example=[example],
                    board=listify(exclude.get("board", example_matrix["boards"])),
                    config=listify(exclude.get("config", example_matrix["configs"])),
                    build_system=listify(
                        exclude.get("build_system", example_matrix["build_systems"])
                    ),
                    test_fn=[test_fn],
                    backend_fn=[backend_fn],
                    no_output_timeout_s=[no_output_timeout_s],
                )
            )

    return list(test_cases)


EXAMPLES: dict[str, _ExampleMatrixType] = {
    "rust": {
        "configs": ["debug", "release"],
        "build_systems": ["make"],
        "boards": [
            "qemu_virt_aarch64",
        ],
        "tests_exclude": [],
    },
    "simple": {
        "configs": ["debug", "release"],
        "build_systems": ["make", "zig"],
        "boards": [
            "qemu_virt_aarch64",
            "odroidc4",
            "maaxboard",
        ],
        "tests_exclude": [],
    },
    "virtio": {
        "configs": ["debug", "release"],
        "build_systems": ["make", "zig"],
        "boards": [
            "qemu_virt_aarch64",
            "maaxboard",
        ],
        "tests_exclude": [],
    },
    "virtio_pci": {
        "configs": ["debug", "release"],
        "build_systems": ["make"],
        "boards": [
            "qemu_virt_aarch64",
            "maaxboard",
        ],
        "tests_exclude": [],
    },
    "zig": {
        "configs": ["debug", "release"],
        "build_systems": ["zig"],
        "boards": [
            "qemu_virt_aarch64",
        ],
        "tests_exclude": [],
    },
}

## Type Hinting + Sanity Checks ##
_BoardNames = Literal[
    "maaxboard",
    "odroidc4",
    "qemu_virt_aarch64",
]

known_board_names = set(MACHINE_QUEUE_BOARDS.keys()) | {
    # simulation boards
    "qemu_virt_aarch64",
    "qemu_virt_riscv64",
}
assert (
    set(_BoardNames.__args__) <= known_board_names  # type: ignore
), f"_BoardNames contains a board that is not valid {known_board_names ^ set(_BoardNames.__args__)}"  # type: ignore

for ex in EXAMPLES.values():
    for board in chain(
        ex["boards"], (excl["board"] for excl in ex["tests_exclude"] if "board" in excl)
    ):
        assert board in known_board_names, f"{board} not a valid board"


class _ExampleMatrixType(TypedDict):
    configs: list[Literal["debug", "release", "benchmark"]]
    build_systems: list[Literal["make", "zig"]]
    boards: list[_BoardNames]
    tests_exclude: list[dict[str, str]]
