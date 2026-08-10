#!/usr/bin/env python3
# Copyright 2026, UNSW
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
    async with asyncio.timeout(60):
        # This is the first thing that OVMF prints on the debug console
        await wait_for_output(backend, b"SecCoreStartupWithStack")
        # Firmware should see our virtual fw cfg device
        await wait_for_output(backend, b"QemuFwCfgProbe: Supported 1")
        # Firmware should load all our ACPI table
        await wait_for_output(backend, b"InstallQemuFwCfgTables: installed 5 tables")
        # Linux should detect EFI
        await wait_for_output(backend, b"efi: EFI v2.7 by EDK II")
        # Linux should finish booting
        await wait_for_output(backend, b"buildroot login: ")
        await send_input(backend, b"root\n")
        await wait_for_output(backend, b"# ")


# export
TEST_CASES = matrix.generate_example_test_cases(
    "uefi_firmware_boot",
    ["uefi"],
    test_fn=test,
    backend_fn=common.backend_fn,
    no_output_timeout_s=matrix.NO_OUTPUT_DEFAULT_TIMEOUT_S,
)

if __name__ == "__main__":
    common.run_tests(TEST_CASES)
