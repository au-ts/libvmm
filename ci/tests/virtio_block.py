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
from ci.common import virtio_backend_fn, run_tests


async def test_virtio_block(backend: HardwareBackend, test_config: common.TestConfig):
    async with asyncio.timeout(180):
        await wait_for_output(backend, b"buildroot login: ")
        # Sleep should not be necessary, see https://github.com/au-ts/libvmm/issues/195
        await asyncio.sleep(1)
        await send_input(backend, b"root\n")
        await wait_for_output(backend, b"# ")
        # Unconditionally unmount and make a fresh file system before running the tests
        await send_input(backend, b"umount /mnt\n")
        await wait_for_output(backend, b"# ")
        await send_input(backend, b"mkdosfs -F 32 -n virtio-blk /dev/vda\n")
        await wait_for_output(backend, b"# ")
        await send_input(backend, b"mount /dev/vda /mnt\n")
        await wait_for_output(backend, b"# ")
        # Now run the tests
        await send_input(backend, b"./blk_integration_tests.sh\n")
        await wait_for_output(backend, b"All is well in the universe")
        # Run twice to ensure that our virtio block device left the filesystem in a sane state
        # after all clean up actions. If the FS becomes corrupted, Linux will set the FS to
        # read only and the script will fail.
        await send_input(backend, b"./blk_integration_tests.sh\n")
        await wait_for_output(backend, b"All is well in the universe")


# export
TEST_CASES = matrix.generate_example_test_cases(
    "virtio_block",
    ["virtio", "virtio_pci"],
    test_fn=test_virtio_block,
    backend_fn=common.virtio_backend_fn,
    no_output_timeout_s=matrix.NO_OUTPUT_DEFAULT_TIMEOUT_S,
)


if __name__ == "__main__":
    run_tests(TEST_CASES)
