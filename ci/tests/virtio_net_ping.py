#!/usr/bin/env python3
# Copyright 2026, UNSW
# SPDX-License-Identifier: BSD-2-Clause

import asyncio
from pathlib import Path
import sys

from ts_ci import (
    HardwareBackend,
    send_input,
    wait_for_output,
)

sys.path.insert(1, Path(__file__).parents[2].as_posix())
from ci import common, matrix
from ci.common import run_tests


async def test_virtio_net_ping(
    backend: HardwareBackend, test_config: common.TestConfig
):
    async with asyncio.timeout(30):
        await wait_for_output(backend, b"buildroot login: ")
        # Sleep should not be necessary, see https://github.com/au-ts/libvmm/issues/195
        await asyncio.sleep(1)
        await send_input(backend, b"root\n")
        await wait_for_output(backend, b"# ")
        # Ping the default gateway, it would be great if we can ping the internet,
        # but the GitHub runners block ICMP so...
        await send_input(backend, b"ping -c 10 $(ip route | grep default | cut -d' ' -f3)\n")
        await wait_for_output(backend, b"64 bytes from")
        await wait_for_output(backend, b"10 packets transmitted, 10 packets received, 0% packet loss")


# export
TEST_CASES = matrix.generate_example_test_cases(
    "virtio_network_ping",
    ["virtio", "virtio_pci"],
    test_fn=test_virtio_net_ping,
    backend_fn=common.virtio_backend_fn,
    no_output_timeout_s=matrix.NO_OUTPUT_DEFAULT_TIMEOUT_S,
)


if __name__ == "__main__":
    run_tests(TEST_CASES)
