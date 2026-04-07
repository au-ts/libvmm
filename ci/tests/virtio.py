#!/usr/bin/env python3
# Copyright 2025, UNSW
# SPDX-License-Identifier: BSD-2-Clause

import asyncio
from pathlib import Path
import sys

from ts_ci import (
    HardwareBackend,
    QemuBackend,
    send_input,
    wait_for_output,
)

sys.path.insert(1, Path(__file__).parents[2].as_posix())
from ci import common, matrix
from ci.common import virtio_backend_fn, run_tests


async def test(backend: HardwareBackend, test_config: common.TestConfig):
    async with asyncio.timeout(30):
        await wait_for_output(backend, b"buildroot login: ")
        # Sleep should not be necessary, see https://github.com/au-ts/libvmm/issues/195
        await asyncio.sleep(1)
        await send_input(backend, b"root\n")
        await wait_for_output(backend, b"# ")


# export
TEST_CASES = matrix.generate_example_test_cases(
    "virtio",
    ["virtio", "virtio_pci"],
    test_fn=test,
    backend_fn=virtio_backend_fn,
    no_output_timeout_s=matrix.NO_OUTPUT_DEFAULT_TIMEOUT_S,
)


if __name__ == "__main__":
    run_tests(TEST_CASES)
