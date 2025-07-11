# Copyright 2025, UNSW
# SPDX-License-Identifier: BSD-2-Clause
import argparse
import struct
import random
from dataclasses import dataclass
from typing import List, Tuple
from sdfgen import SystemDescription, Sddf, DeviceTree, Vmm
from importlib.metadata import version

assert version('sdfgen').split(".")[1] == "24", "Unexpected sdfgen version"

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
    timer: str
    blk: str
    guest_blk: str
    net: str
    guest_net: str
    partition: int


BOARDS: List[Board] = [
    Board(
        name="qemu_virt_aarch64",
        arch=SystemDescription.Arch.AARCH64,
        paddr_top=0x6_0000_000,
        serial="pl011@9000000",
        guest_serial="virtio-console@130000",
        timer=None,
        blk="virtio_mmio@a003e00",
        guest_blk="virtio-blk@150000",
        net="virtio_mmio@a003c00",
        guest_net="virtio-net@160000",
        partition=0
    ),
    Board(
        name="maaxboard",
        arch=SystemDescription.Arch.AARCH64,
        paddr_top=0x90000000,
        serial="soc@0/bus@30800000/serial@30860000",
        guest_serial="virtio-console@130000",
        timer="soc@0/bus@30000000/timer@302d0000",
        blk="soc@0/bus@30800000/mmc@30b40000",
        guest_blk="virtio-blk@150000",
        net="soc@0/bus@30800000/ethernet@30be0000",
        guest_net="virtio-net@160000",
        partition=0
    ),
]


def generate(sdf_file: str, output_dir: str, dtb: DeviceTree, client_dtb: DeviceTree):
    # Client VM
    vmm_client0 = ProtectionDomain("CLIENT_VMM", "client_vmm.elf", priority=100)
    vm_client0 = VirtualMachine("client_linux", [VirtualMachine.Vcpu(id=0)])
    client0 = Vmm(sdf, vmm_client0, vm_client0, client_dtb)
    sdf.add_pd(vmm_client0)

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
    guest_net_node = client_dtb.node(board.guest_net)
    assert guest_net_node is not None

    eth_driver = ProtectionDomain("eth_driver", "eth_driver.elf",
                                  priority=101, budget=100, period=400)
    net_virt_tx = ProtectionDomain("net_virt_tx", "network_virt_tx.elf", priority=100, budget=20000)
    net_virt_rx = ProtectionDomain("net_virt_rx", "network_virt_rx.elf", priority=99)
    net_system = Sddf.Net(sdf, net_node, eth_driver, net_virt_tx, net_virt_rx)
    client0_net_copier = ProtectionDomain(
        "client0_net_copier", "network_copy.elf", priority=98, budget=20000)

    pds = [
        eth_driver,
        net_virt_rx,
        net_virt_tx,
        client0_net_copier,
    ]
    for pd in pds:
        sdf.add_pd(pd)

    client0.add_virtio_mmio_net(guest_net_node, net_system, client0_net_copier)

    # Block subsystem
    blk_driver = ProtectionDomain("blk_driver", "blk_driver.elf", priority=200)
    blk_virt = ProtectionDomain("blk_virt", "blk_virt.elf", priority=199, stack_size=0x2000)

    blk_node = dtb.node(board.blk)
    assert blk_node is not None
    guest_blk_node = client_dtb.node(board.guest_blk)
    assert guest_blk_node is not None

    blk_system = Sddf.Blk(sdf, blk_node, blk_driver, blk_virt)
    partition = int(args.partition) if args.partition else board.partition
    client0.add_virtio_mmio_blk(guest_blk_node, blk_system, partition=partition)
    pds = [
        blk_driver,
        blk_virt
    ]
    for pd in pds:
        sdf.add_pd(pd)

    # Timer subsystem (Maaxboard specific as its blk driver needs a timer)
    if board.name == "maaxboard":
        timer_node = dtb.node(board.timer)
        assert timer_node is not None

        timer_driver = ProtectionDomain("timer_driver", "timer_driver.elf", priority=210)
        timer_system = Sddf.Timer(sdf, timer_node, timer_driver)

        timer_system.add_client(blk_driver)
        sdf.add_pd(timer_driver)

        assert timer_system.connect()
        assert timer_system.serialise_config(output_dir)

    assert serial_system.connect()
    assert serial_system.serialise_config(output_dir)
    assert blk_system.connect()
    assert blk_system.serialise_config(output_dir)
    assert net_system.connect()
    assert net_system.serialise_config(output_dir)
    assert client0.connect()
    assert client0.serialise_config(output_dir)

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
