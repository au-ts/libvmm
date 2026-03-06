#!/usr/bin/env python3
# Copyright 2025, UNSW
# SPDX-License-Identifier: BSD-2-Clause

import asyncio
from pathlib import Path
import sys

from ts_ci import (
    matrix_product,
    HardwareBackend,
    TestConfig,
    wait_for_output,
    send_input,
    expect_output,
    TestMetadata,
    run_test,
)

# TODO TODO buildroot_login.py
# distingush between tests and examples

sys.path.insert(1, Path(__file__).parents[2].as_posix())
from ci import common, matrix

TEST_MATRIX = [
    # *matrix.generate_example_test_matrix("rust", matrix.EXAMPLES["rust"]),
    *matrix.generate_example_test_matrix("simple", matrix.EXAMPLES["simple"]),
    # *matrix.generate_example_test_matrix("zig", matrix.EXAMPLES["zig"]),
]


async def test(backend: HardwareBackend, test_config: TestConfig):
    async with asyncio.timeout(30):
        if test_config.board == "odroidc4":
            # FIXME
            await wait_for_output(backend, b"odroidc2 login: ")
        else:
            await wait_for_output(backend, b"buildroot login: ")
        await send_input(backend, b"root\n")
        await wait_for_output(backend, b"# ")

# export
TEST_METADATA = TestMetadata(
    test_fn=test,
    backend_fn=common.backend_fn,
    loader_img_fn=common.loader_img_path,
    no_output_timeout_s=matrix.NO_OUTPUT_DEFAULT_TIMEOUT_S,
)

if __name__ == "__main__":
    run_test(TEST_METADATA, TEST_MATRIX)
