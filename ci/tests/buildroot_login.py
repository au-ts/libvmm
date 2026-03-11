#!/usr/bin/env python3
# Copyright 2025, UNSW
# SPDX-License-Identifier: BSD-2-Clause

import asyncio
from pathlib import Path
import sys

from ts_ci import (
    HardwareBackend,
    wait_for_output,
    send_input,
    expect_output,
)

sys.path.insert(1, Path(__file__).parents[2].as_posix())
from ci import common, matrix


async def test(backend: HardwareBackend, test_config: common.TestConfig):
    async with asyncio.timeout(30):
        if test_config.board == "odroidc4":
            # FIXME
            await wait_for_output(backend, b"odroidc2 login: ")
        else:
            await wait_for_output(backend, b"buildroot login: ")
        await send_input(backend, b"root\n")
        await wait_for_output(backend, b"# ")


# export
TEST_CASES = [
    *matrix.generate_example_test_cases(
        "buildroot_login",
        "rust",
        matrix.EXAMPLES["rust"],
        test_fn=test,
        backend_fn=common.backend_fn,
        no_output_timeout_s=matrix.NO_OUTPUT_DEFAULT_TIMEOUT_S,
    ),
    *matrix.generate_example_test_cases(
        "buildroot_login",
        "simple",
        matrix.EXAMPLES["simple"],
        test_fn=test,
        backend_fn=common.backend_fn,
        no_output_timeout_s=matrix.NO_OUTPUT_DEFAULT_TIMEOUT_S,
    ),
    # *matrix.generate_example_test_cases("zig", matrix.EXAMPLES["zig"]),
]

if __name__ == "__main__":
    common.run_tests(TEST_CASES)
