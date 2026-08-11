#!/usr/bin/env python3
# Copyright 2026, UNSW
# SPDX-License-Identifier: BSD-2-Clause

import re
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
from ci.common import virtio_backend_fn, run_tests


async def test_virtio_net_wget(
    backend: HardwareBackend, test_config: common.TestConfig
):
    # It is a large image so it takes a while to load
    # TODO @billn make the 4 VMMs share the images
    async with asyncio.timeout(240):
        await wait_for_output(backend, b"buildroot login: ")
        await wait_for_output(backend, b"buildroot login: ")
        await wait_for_output(backend, b"buildroot login: ")
        await wait_for_output(backend, b"buildroot login: ")

        # 4 VMs ready, now make sure that we are on VM 0
        await send_input(backend, b"\x1c0\r")
        await send_input(backend, b"root\n")
        await wait_for_output(backend, b"# ")

        # VM 0 should reach the internet
        await send_input(backend, b"wget https://trustworthy.systems/song\n")
        await wait_for_output(backend, b"'song' saved")
        await send_input(backend, b"cat song\n")
        await wait_for_output(backend, b"Implementation deep and fine.")
        await wait_for_output(backend, b"# ")

        # Get VM 0 IP addr
        await send_input(backend, b"ifconfig eth0 | grep 'inet addr' | xargs | cut -d' ' -f2 | cut -d':' -f2\n")
        await wait_for_output(backend, b"\r\n")
        vm0_ip = re.search(rb"\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}", await wait_for_output(backend, b"# ")).group(0).decode() # type: ignore

        # Switch to VM 1, make sure it can get to the internet, then get its IP addr
        await send_input(backend, b"\x1c1\r")
        await send_input(backend, b"root\n")
        await wait_for_output(backend, b"# ")

        await send_input(backend, b"wget https://trustworthy.systems/song\n")
        await wait_for_output(backend, b"'song' saved")
        await send_input(backend, b"cat song\n")
        await wait_for_output(backend, b"Implementation deep and fine.")
        await wait_for_output(backend, b"# ")

        await send_input(backend, b"ifconfig eth0 | grep 'inet addr' | xargs | cut -d' ' -f2 | cut -d':' -f2\n")
        await wait_for_output(backend, b"\r\n")
        vm1_ip = re.search(rb"\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}", await wait_for_output(backend, b"# ")).group(0).decode() # type: ignore

        # VM 1 should be able to ping VM 0
        await send_input(backend, f"ping -c 5 {vm0_ip}\n".encode('utf-8'))
        await wait_for_output(backend, b"5 packets transmitted, 5 packets received, 0% packet loss")
        await wait_for_output(backend, b"# ")

        # VM 0 should be able to ping VM 1
        await send_input(backend, b"\x1c0\r")
        await send_input(backend, f"ping -c 5 {vm1_ip}\n".encode('utf-8'))
        await wait_for_output(backend, b"5 packets transmitted, 5 packets received, 0% packet loss")
        await wait_for_output(backend, b"# ")

        # Switch to VM 2, make sure it can get to the internet, then get its IP addr
        await send_input(backend, b"\x1c2\r")
        await send_input(backend, b"root\n")
        await wait_for_output(backend, b"# ")

        await send_input(backend, b"wget https://trustworthy.systems/song\n")
        await wait_for_output(backend, b"'song' saved")
        await send_input(backend, b"cat song\n")
        await wait_for_output(backend, b"Implementation deep and fine.")
        await wait_for_output(backend, b"# ")

        await send_input(backend, b"ifconfig eth0 | grep 'inet addr' | xargs | cut -d' ' -f2 | cut -d':' -f2\n")
        await wait_for_output(backend, b"\r\n")
        vm2_ip = re.search(rb"\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}", await wait_for_output(backend, b"# ")).group(0).decode() # type: ignore

        # VM 2 shouldn't be able to ping VM 1
        await send_input(backend, f"ping -c 5 {vm1_ip}\n".encode('utf-8'))
        await wait_for_output(backend, b"5 packets transmitted, 0 packets received, 100% packet loss")
        await wait_for_output(backend, b"# ")

        # VM 1 shouldn't be able to ping VM 2
        await send_input(backend, b"\x1c1\r")
        await send_input(backend, f"ping -c 5 {vm2_ip}\n".encode('utf-8'))
        await wait_for_output(backend, b"5 packets transmitted, 0 packets received, 100% packet loss")
        await wait_for_output(backend, b"# ")

# export
TEST_CASES = matrix.generate_example_test_cases(
    "vswitch",
    ["vswitch"],
    test_fn=test_virtio_net_wget,
    backend_fn=common.virtio_backend_fn,
    no_output_timeout_s=matrix.NO_OUTPUT_DEFAULT_TIMEOUT_S,
)


if __name__ == "__main__":
    run_tests(TEST_CASES)
