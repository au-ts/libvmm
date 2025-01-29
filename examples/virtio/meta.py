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
    blk: str

BOARDS: List[Board] = [
    Board(
        name="qemu_virt_aarch64",
        arch=SystemDescription.Arch.AARCH64,
        paddr_top=0x6_0000_000,
        serial="pl011@9000000",
        blk="virtio_mmio@a003e00",
    ),
]

def generate(sdf_file: str, output_dir: str, dtb: DeviceTree, client_dtb: DeviceTree):
    # Client 1
    vmm_client1 = ProtectionDomain("CLIENT_VMM-1", "client_vmm.elf", priority=100)
    vm_client1 = VirtualMachine("client_linux-1", [VirtualMachine.Vcpu(id=0)])
    client1 = Vmm(sdf, vmm_client1, vm_client1, client_dtb)
    sdf.add_pd(vmm_client1)
    assert client1.connect()

    # Client 2
    vmm_client2 = ProtectionDomain("CLIENT_VMM-2", "client_vmm.elf", priority=100)
    vm_client2 = VirtualMachine("client_linux-2", [VirtualMachine.Vcpu(id=0)])
    client2 = Vmm(sdf, vmm_client2, vm_client2, client_dtb)
    sdf.add_pd(vmm_client2)
    assert client2.connect()

    # Serial subsystem
    serial_driver = ProtectionDomain("serial_driver", "uart_driver.elf", priority=200)
    serial_virt_tx = ProtectionDomain("serial_virt_tx", "serial_virt_tx.elf", priority=199)
    # Increase the stack size as running with UBSAN uses more stack space than normal.
    serial_virt_rx = ProtectionDomain("serial_virt_rx", "serial_virt_rx.elf",
                                      priority=199, stack_size=0x2000)
    serial_node = dtb.node(board.serial)
    assert serial_node is not None

    serial_system = Sddf.Serial(sdf, serial_node, serial_driver,
                                serial_virt_tx, virt_rx=serial_virt_rx)
    serial_system.add_client(vmm_client1)
    serial_system.add_client(vmm_client2)

    pds = [
        serial_driver,
        serial_virt_tx,
        serial_virt_rx,
    ]
    for pd in pds:
        sdf.add_pd(pd)

    assert serial_system.connect()
    assert serial_system.serialise_config(output_dir)

    # Block subsystem
    blk_driver = ProtectionDomain("blk_driver", "blk_driver.elf", priority=200)
    blk_virt = ProtectionDomain("blk_virt", "blk_virt.elf", priority=199, stack_size=0x2000)

    blk_node = dtb.node(board.blk)
    assert blk_node is not None

    blk_system = Sddf.Blk(sdf, blk_node, blk_driver, blk_virt)
    blk_system.add_client(vmm_client1, partition=0)
    blk_system.add_client(vmm_client2, partition=1)
    pds = [
        blk_driver,
        blk_virt
    ]
    for pd in pds:
        sdf.add_pd(pd)

    assert blk_system.connect()
    assert blk_system.serialise_config(output_dir)

    # generate XML
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

    args = parser.parse_args()

    board = next(filter(lambda b: b.name == args.board, BOARDS))

    sdf = SystemDescription(board.arch, board.paddr_top)

    sddf = Sddf(args.sddf)

    with open(args.dtb, "rb") as f:
        dtb = DeviceTree(f.read())

    with open(args.client_dtb, "rb") as f:
        client_dtb = DeviceTree(f.read())

    generate(args.sdf, args.output, dtb, client_dtb)
