# Copyright 2025, UNSW
# SPDX-License-Identifier: BSD-2-Clause
import argparse
import struct
import random
from dataclasses import dataclass
from typing import List, Tuple
from sdfgen import SystemDescription, Sddf, DeviceTree, Vmm

ProtectionDomain = SystemDescription.ProtectionDomain
VirtualMachine = SystemDescription.VirtualMachine
MemoryRegion = SystemDescription.MemoryRegion
Map = SystemDescription.Map
Channel = SystemDescription.Channel


@dataclass
class Board:
    name: str
    arch: SystemDescription.Arch
    paddr_top: int
    serial: str
    guest_serial: str


BOARDS: List[Board] = [
    Board(
        name="qemu_virt_aarch64",
        arch=SystemDescription.Arch.AARCH64,
        paddr_top=0x6_0000_000,
        serial="pl011@9000000",
        guest_serial="virtio-console@130000",
    ),
    Board(
        name="maaxboard",
        arch=SystemDescription.Arch.AARCH64,
        paddr_top=0x90000000,
        serial="soc@0/bus@30800000/serial@30860000",
        guest_serial="virtio-console@130000",
    ),
]


def generate(sdf_file: str, output_dir: str, dtb: DeviceTree, client_dtb: DeviceTree):
    # Client VM 0
    vmm_client0 = ProtectionDomain("CLIENT_VMM0", "client_vmm0.elf", priority=101)
    vm_client0 = VirtualMachine("client_linux0", [VirtualMachine.Vcpu(id=0)])
    client0 = Vmm(sdf, vmm_client0, vm_client0, client_dtb)
    sdf.add_pd(vmm_client0)

    # Client VM 1
    vmm_client1 = ProtectionDomain("CLIENT_VMM1", "client_vmm1.elf", priority=100)
    vm_client1 = VirtualMachine("client_linux1", [VirtualMachine.Vcpu(id=0)])
    client1 = Vmm(sdf, vmm_client1, vm_client1, client_dtb)
    sdf.add_pd(vmm_client1)

    # Serial subsystem
    serial_driver = ProtectionDomain("serial_driver", "serial_driver.elf", priority=200)
    serial_virt_tx = ProtectionDomain("serial_virt_tx", "serial_virt_tx.elf", priority=199)
    # Increase the stack size as running with UBSAN uses more stack space than normal.
    serial_virt_rx = ProtectionDomain("serial_virt_rx", "serial_virt_rx.elf",
                                      priority=199, stack_size=0x2000)
    serial_node = dtb.node(board.serial)
    assert serial_node is not None
    guest_serial_node = client_dtb.node(board.guest_serial)
    assert guest_serial_node is not None

    serial_system = Sddf.Serial(sdf, serial_node, serial_driver,
                                serial_virt_tx, virt_rx=serial_virt_rx)
    client0.add_virtio_mmio_console(guest_serial_node, serial_system)
    client1.add_virtio_mmio_console(guest_serial_node, serial_system)

    pds = [
        serial_driver,
        serial_virt_tx,
        serial_virt_rx,
    ]
    for pd in pds:
        sdf.add_pd(pd)

    assert serial_system.connect()
    assert serial_system.serialise_config(output_dir)
    assert client0.connect()
    assert client0.serialise_config(output_dir)
    assert client1.connect()
    assert client1.serialise_config(output_dir)

    with open(f"{output_dir}/{sdf_file}", "w+") as f:
        f.write(sdf.render())


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument("--dtb", required=True)
    parser.add_argument("--client_dtb", required=True)
    parser.add_argument("--sddf", required=True)
    parser.add_argument("--board", required=True, choices=[b.name for b in BOARDS])
    parser.add_argument("--output", required=True)
    parser.add_argument("--sdf", required=True)
    parser.add_argument("--partition")

    args = parser.parse_args()

    board = next(filter(lambda b: b.name == args.board, BOARDS))

    sdf = SystemDescription(board.arch, board.paddr_top)

    sddf = Sddf(args.sddf)

    with open(args.dtb, "rb") as f:
        dtb = DeviceTree(f.read())

    with open(args.client_dtb, "rb") as f:
        client_dtb = DeviceTree(f.read())

    generate(args.sdf, args.output, dtb, client_dtb)
