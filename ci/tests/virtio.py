#!/usr/bin/env python3
# Copyright 2025, UNSW
# SPDX-License-Identifier: BSD-2-Clause

import asyncio
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import types

from ts_ci import (
    HardwareBackend,
    QemuBackend,
    send_input,
    wait_for_output,
)

sys.path.insert(1, Path(__file__).parents[2].as_posix())
from ci import common, matrix
from ci.common import TestConfig, run_tests

LIBVMM = Path(__file__).parents[2]
mkvirtdisk = (LIBVMM / "dep" / "sddf" / "tools" / "mkvirtdisk").resolve()


def backend_fn(test_config: TestConfig, loader_img: Path) -> HardwareBackend:
    backend = common.backend_fn(test_config, loader_img)

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
    backend_fn=backend_fn,
    no_output_timeout_s=matrix.NO_OUTPUT_DEFAULT_TIMEOUT_S,
)


if __name__ == "__main__":
    run_tests(TEST_CASES)
