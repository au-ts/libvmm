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
    # The timeout is double of other tests because we are running 4 VCPUs
    # on 1 physical CPU due to https://github.com/seL4/seL4/issues/1645
    async with asyncio.timeout(60):
        await wait_for_output(backend, b"smp: Brought up 1 node, 4 CPUs")
        await wait_for_output(backend, b"SMP: Total of 4 processors activated.")
        await wait_for_output(backend, b"buildroot login: ")
        await send_input(backend, b"root\n")
        await wait_for_output(backend, b"# ")
        await send_input(backend, b"./guest_smp_test_script.sh\n")
        await wait_for_output(backend, b"SMP TEST PASSED!")


# export
TEST_CASES = matrix.generate_example_test_cases(
    "smp_detect_multiple_cpus",
    ["smp"],
    test_fn=test,
    backend_fn=common.backend_fn,
    no_output_timeout_s=matrix.NO_OUTPUT_DEFAULT_TIMEOUT_S,
)

if __name__ == "__main__":
    common.run_tests(TEST_CASES)
