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
    net: str
    guest_net: str

BOARDS: List[Board] = [
    Board(
        name="qemu_virt_aarch64",
        arch=SystemDescription.Arch.AARCH64,
        paddr_top=0x6_0000_000,
        serial="pl011@9000000",
        blk="virtio_mmio@a003e00",
        net="virtio_mmio@a003c00",
        guest_net="virtio-net@0160000",
    ),
]

def generate(sdf_file: str, output_dir: str, dtb: DeviceTree, client_dtb: DeviceTree):
    # Client 1
    vmm_client0 = ProtectionDomain("CLIENT_VMM-1", "client_vmm.elf", priority=100)
    vm_client0 = VirtualMachine("client_linux-1", [VirtualMachine.Vcpu(id=0)])
    client0 = Vmm(sdf, vmm_client0, vm_client0, client_dtb)
    sdf.add_pd(vmm_client0)
    assert client0.connect()
    assert client0.serialise_config(output_dir)

    # Client 2
    # vmm_client2 = ProtectionDomain("CLIENT_VMM-2", "client_vmm.elf", priority=100)
    # vm_client2 = VirtualMachine("client_linux-2", [VirtualMachine.Vcpu(id=0)])
    # client2 = Vmm(sdf, vmm_client2, vm_client2, client_dtb)
    # sdf.add_pd(vmm_client2)
    # assert client2.connect()

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
    serial_system.add_client(vmm_client0)
    # serial_system.add_client(vmm_client2)

    pds = [
        serial_driver,
        serial_virt_tx,
        serial_virt_rx,
    ]
    for pd in pds:
        sdf.add_pd(pd)

    # Net subsystem
    net_node = dtb.node(board.net)
    assert net_node is not None

    eth_driver = ProtectionDomain("eth_driver", "eth_driver.elf", priority=101, budget=100, period=400)
    net_virt_tx = ProtectionDomain("net_virt_tx", "network_virt_tx.elf", priority=100, budget=20000)
    net_virt_rx = ProtectionDomain("net_virt_rx", "network_virt_rx.elf", priority=99)
    net_system = Sddf.Net(sdf, net_node, eth_driver, net_virt_tx, net_virt_rx)
    client0_net_copier = ProtectionDomain("client0_net_copier", "network_copy0.elf", priority=98, budget=20000)

    pds = [
        eth_driver,
        net_virt_rx,
        net_virt_tx,
        client0_net_copier,
    ]
    for pd in pds:
        sdf.add_pd(pd)

    mac_random_part = random.randint(0, 0xfe)
    client0_mac_addr = f"52:54:01:00:00:{hex(mac_random_part)[2:]:0>2}"

    client0.add_virtio_mmio_net(client_dtb.node(board.guest_net), net_system, client0_net_copier, mac_addr=client0_mac_addr)

    # Block subsystem
    blk_driver = ProtectionDomain("blk_driver", "blk_driver.elf", priority=200)
    blk_virt = ProtectionDomain("blk_virt", "blk_virt.elf", priority=199, stack_size=0x2000)

    blk_node = dtb.node(board.blk)
    assert blk_node is not None

    blk_system = Sddf.Blk(sdf, blk_node, blk_driver, blk_virt)
    blk_system.add_client(vmm_client0, partition=0)
    # blk_system.add_client(vmm_client2, partition=1)
    pds = [
        blk_driver,
        blk_virt
    ]
    for pd in pds:
        sdf.add_pd(pd)

    assert serial_system.connect()
    assert serial_system.serialise_config(output_dir)
    assert blk_system.connect()
    assert blk_system.serialise_config(output_dir)
    assert net_system.connect()
    assert net_system.serialise_config(output_dir)

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
