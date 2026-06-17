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


# Virtualising the architectural timer(s) is tricky, especially on x86. So let
# check that Linux is happy with our virtualisation job.
async def test(backend: HardwareBackend, test_config: common.TestConfig):
    async with asyncio.timeout(30):
        if "x86" in test_config.board:
            # HPET detected?
            await wait_for_output(backend, b"clocksource: hpet: mask: 0xffffffff max_cycles: 0xffffffff")
            # TSC-early detected?
            await wait_for_output(backend, b"clocksource: tsc-early: mask: 0xffffffffffffffff")
            # ACPI PM timer detected?
            await wait_for_output(backend, b"clocksource: acpi_pm:")
            # Should switch from TSC-early to TSC now, if the switch doesn't happen then one of the clocksources
            # isn't agreeing with each others on what the time is, or the APIC code is dropping interrupts.
            await wait_for_output(backend, b"clocksource: tsc: mask: 0xffffffffffffffff")
            await wait_for_output(backend, b"clocksource: Switched to clocksource tsc")
            # Schedclock should be stable now
            await wait_for_output(backend, b"sched_clock: Marking stable")
        else:
            await wait_for_output(backend, b"clocksource: Switched to clocksource arch_sys_counter")

        # @billn extend in the future to use kselftest or something like that

# export
TEST_CASES = matrix.generate_example_test_cases(
    "arch_timers",
    ["simple"],
    test_fn=test,
    backend_fn=common.backend_fn,
    no_output_timeout_s=matrix.NO_OUTPUT_DEFAULT_TIMEOUT_S,
)

if __name__ == "__main__":
    common.run_tests(TEST_CASES)
