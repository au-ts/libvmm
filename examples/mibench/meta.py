# Copyright 2025, UNSW
# SPDX-License-Identifier: BSD-2-Clause
import argparse
import struct
import random
from dataclasses import dataclass
from typing import List, Tuple
from sdfgen import SystemDescription, Sddf, DeviceTree, Vmm
from importlib.metadata import version

import sys, os

import json
import subprocess
import shutil
from typing import Callable, Optional

ProtectionDomain = SystemDescription.ProtectionDomain
MemoryRegion = SystemDescription.MemoryRegion
Map = SystemDescription.Map
Channel = SystemDescription.Channel
VirtualMachine = SystemDescription.VirtualMachine
Irq = SystemDescription.Irq
IrqConventional = SystemDescription.IrqConventional


@dataclass
class Board:
    name: str
    arch: SystemDescription.Arch
    paddr_top: int
    #serial: str
    #guest_serial: str
    timer: str
    #ethernet: str
    #passthrough: list[str]


BOARDS: List[Board] = [
    Board(
        name="odroidc4",
        arch=SystemDescription.Arch.AARCH64,
        paddr_top=0x60000000,
        # serial="soc/bus@ff800000/serial@3000",
        # guest_serial="virtio-console@100000",
        timer="soc/bus@ffd00000/watchdog@f0d0",
        # ethernet="soc/ethernet@ff3f0000",
        # passthrough=[
            # "soc/bus@ff600000/bus@34400",   # pinmux
            # "soc/bus@ff600000/bus@3c000",   # power controller
            # "soc/bus@ff600000/mdio-multiplexer@4c000", # ethernet PHY
            # "soc/bus@ff800000/sys-ctrl@0",  # pinmux
        # ],
    ),
]

def generate(sdf_file: str, output_dir: str, dtb: DeviceTree, mibench_vm_dtb: DeviceTree):
    
    timer_node = dtb.node(board.timer)
    assert timer_node is not None

    timer_driver = ProtectionDomain("timer_driver", "timer_driver.elf", priority=110, cpu=0)
    timer_system = Sddf.Timer(sdf, timer_node, timer_driver)

    mibench_vmm_pd = ProtectionDomain("mibench_pd", "vmm.elf", priority=106,cpu=0)
    mibench_vm = VirtualMachine("linux", [VirtualMachine.Vcpu(id=0)], priority=105)
    # one_to_one_ram=True is very important as the network drivers in Linux uses DMA!
    mibench_vmm = Vmm(sdf, mibench_vmm_pd, mibench_vm, mibench_vm_dtb, one_to_one_ram=False)

    timer_system.add_client(mibench_vmm_pd)

    # From the original system file
    bus1 = MemoryRegion(sdf, "bus1", 0x200000, paddr=0xff600000)
    bus2 = MemoryRegion(sdf, "bus2", 0x200000, paddr=0xff800000)
    bus3a = MemoryRegion(sdf, "bus3a", 0xf000, paddr=0xffd00000)
    bus3b = MemoryRegion(sdf, "bus3b", 0xf0000, paddr=0xffd10000)
    bench = MemoryRegion(sdf, "bench", 0x200000, paddr=0x10000000)

    for mr in (bus1, bus2, bus3a, bus3b, bench):
        sdf.add_mr(mr)

    mibench_vm.add_map(Map(bus1, 0xff600000, perms="rw", cached=False))
    mibench_vm.add_map(Map(bus2, 0xff800000, perms="rw", cached=False))
    mibench_vm.add_map(Map(bus3a, 0xffd00000, perms="rw", cached=False))
    mibench_vm.add_map(Map(bus3b, 0xffd10000, perms="rw", cached=False))
    mibench_vm.add_map(Map(bench, 0x10000000, perms="rw", cached=False))

    mibench_vmm_pd.add_map(Map(bench, 0x30000000, perms="rw", cached=False))

    mibench_vmm.add_passthrough_irq(
        IrqConventional(irq=225, trigger=IrqConventional.Trigger.EDGE, id=1)
    )

    # for device_path in board.passthrough:
    #     node = dtb.node(device_path)
    #     assert node is not None
    #     mibench_vmm.add_passthrough_device(node)

    sdf.add_pd(timer_driver)
    sdf.add_pd(mibench_vmm_pd)

    assert mibench_vmm.connect()
    assert timer_system.connect()
    assert mibench_vmm.serialise_config(output_dir)
    assert timer_system.serialise_config(output_dir)

    with open(f"{output_dir}/{sdf_file}", "w+") as f:
        f.write(sdf.render())


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--dtb_native", required=True)
    parser.add_argument("--dtb_guest", required=True)
    parser.add_argument("--sddf", required=True)
    parser.add_argument("--board", required=True, choices=[b.name for b in BOARDS])
    parser.add_argument("--output", required=True)
    parser.add_argument("--sdf", required=True)

    args = parser.parse_args()

    board = next(filter(lambda b: b.name == args.board, BOARDS))

    sdf = SystemDescription(board.arch, board.paddr_top)
    sddf = Sddf(args.sddf)

    with open(args.dtb_native, "rb") as f:
        dtb_native = DeviceTree(f.read())

    with open(args.dtb_guest, "rb") as f:
        dtb_guest = DeviceTree(f.read())

    generate(args.sdf, args.output, dtb_native, dtb_guest)
