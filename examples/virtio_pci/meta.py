# Copyright 2025, UNSW
# SPDX-License-Identifier: BSD-2-Clause
import argparse
import struct
import random
from dataclasses import dataclass
from typing import List, Tuple, Optional
from sdfgen import SystemDescription, Sddf, DeviceTree, Vmm
from importlib.metadata import version

assert version('sdfgen').split(".")[1] == "28", "Unexpected sdfgen version"

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
    serial: Optional[str]
    guest_serial: Optional[str]
    timer: Optional[str]
    blk: Optional[str]
    guest_blk: Optional[str]
    net: Optional[str]
    guest_net: Optional[str]
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
    Board(
        name="x86_64_generic_vtx",
        arch=SystemDescription.Arch.X86_64,
        paddr_top=0x7FFDF000,
        serial=None,
        guest_serial=None,
        timer=None,
        blk=None,
        guest_blk=None,
        net=None,
        guest_net=None,
        partition=0
    ),
]


def generate(sdf_file: str, output_dir: str, dtb: Optional[DeviceTree], client_dtb: Optional[DeviceTree]):
    # Client VM
    vmm_client0 = ProtectionDomain("CLIENT_VMM", "client_vmm.elf", priority=1)
    vm_client0 = VirtualMachine("client_linux", [VirtualMachine.Vcpu(id=0)])
    client0 = Vmm(sdf, vmm_client0, vm_client0, client_dtb)
    sdf.add_pd(vmm_client0)

    if board.arch == SystemDescription.Arch.X86_64:
        guest_ram_mr = MemoryRegion(sdf, name="guest_ram", size=0x1000_0000)
        sdf.add_mr(guest_ram_mr)
        vmm_client0.add_map(Map(guest_ram_mr, vaddr=0x20000000, perms="rw"))
        vm_client0.add_map(Map(guest_ram_mr, vaddr=0x0, perms="rwx"))

    # Serial subsystem
    serial_driver = ProtectionDomain("serial_driver", "serial_driver.elf", priority=200)
    serial_virt_tx = ProtectionDomain("serial_virt_tx", "serial_virt_tx.elf", priority=199)
    # Increase the stack size as running with UBSAN uses more stack space than normal.
    serial_virt_rx = ProtectionDomain("serial_virt_rx", "serial_virt_rx.elf",
                                      priority=199, stack_size=0x2000)

    serial_node = None
    if board.arch != SystemDescription.Arch.X86_64:
        serial_node = dtb.node(board.serial)
        assert serial_node is not None
    else:
        serial_port = SystemDescription.IoPort(0x3F8, 8, 0)
        serial_driver.add_ioport(serial_port)
        serial_irq = SystemDescription.IrqIoapic(0, 4, 0, id=1)
        serial_driver.add_irq(serial_irq)

    # guest_serial_node = client_dtb.node(board.guest_serial)
    # assert guest_serial_node is not None

    serial_system = Sddf.Serial(sdf, serial_node, serial_driver,
                                serial_virt_tx, virt_rx=serial_virt_rx, enable_color=True)
    # client0.add_virtio_mmio_console(guest_serial_node, serial_system)
    serial_system.add_client(vmm_client0)

    pds = [
        serial_driver,
        serial_virt_tx,
        serial_virt_rx,
    ]
    for pd in pds:
        sdf.add_pd(pd)



    # # Net subsystem
    # net_node = dtb.node(board.net)
    # assert net_node is not None
    # guest_net_node = client_dtb.node(board.guest_net)
    # assert guest_net_node is not None

    net_node = None

    eth_driver = ProtectionDomain("eth_driver", "eth_driver.elf",
                                  priority=254)
    net_virt_tx = ProtectionDomain("net_virt_tx", "network_virt_tx.elf", priority=100, budget=20000)
    net_virt_rx = ProtectionDomain("net_virt_rx", "network_virt_rx.elf", priority=99)
    net_system = Sddf.Net(sdf, net_node, eth_driver, net_virt_tx, net_virt_rx)
    client0_net_copier = ProtectionDomain(
        "client0_net_copier", "network_copy.elf", priority=98, budget=20000)

    if board.arch == SystemDescription.Arch.X86_64:
        hw_net_rings = SystemDescription.MemoryRegion(
            sdf, "hw_net_rings", 0x10000, paddr=0x7A000000
        )
        sdf.add_mr(hw_net_rings)
        hw_net_rings_map = SystemDescription.Map(hw_net_rings, 0x7000_0000, "rw")
        eth_driver.add_map(hw_net_rings_map)

        virtio_net_regs = SystemDescription.MemoryRegion(
            sdf, "virtio_net_regs", 0x4000, paddr=0xfebf8000
        )
        sdf.add_mr(virtio_net_regs)
        virtio_net_regs_map = SystemDescription.Map(
            virtio_net_regs, 0x6000_0000, "rw", cached=False
        )
        eth_driver.add_map(virtio_net_regs_map)

        virtio_net_irq = SystemDescription.IrqIoapic(
            ioapic_id=0, pin=10, vector=44, id=16
        )
        eth_driver.add_irq(virtio_net_irq)

        pci_config_address_port = SystemDescription.IoPort(0xCF8, 4, 1)
        eth_driver.add_ioport(pci_config_address_port)

        pci_config_data_port = SystemDescription.IoPort(0xCFC, 4, 2)
        eth_driver.add_ioport(pci_config_data_port)

    pds = [
        eth_driver,
        net_virt_rx,
        net_virt_tx,
        client0_net_copier,
    ]
    for pd in pds:
        sdf.add_pd(pd)

    # client0.add_virtio_mmio_net(guest_net_node, net_system, client0_net_copier)
    net_system.add_client_with_copier(vmm_client0, client0_net_copier, mac_addr="52:54:01:00:00:10")



    # # Block subsystem
    # blk_driver = ProtectionDomain("blk_driver", "blk_driver.elf", priority=200)
    # blk_virt = ProtectionDomain("blk_virt", "blk_virt.elf", priority=199, stack_size=0x2000)

    # blk_node = dtb.node(board.blk)
    # assert blk_node is not None
    # # guest_blk_node = client_dtb.node(board.guest_blk)
    # # assert guest_blk_node is not None

    # blk_system = Sddf.Blk(sdf, blk_node, blk_driver, blk_virt)
    # partition = int(args.partition) if args.partition else board.partition
    # # client0.add_virtio_mmio_blk(guest_blk_node, blk_system, partition=partition)
    # blk_system.add_client(vmm_client0, partition=partition)
    # pds = [
    #     blk_driver,
    #     blk_virt
    # ]
    # for pd in pds:
    #     sdf.add_pd(pd)

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
    elif board.arch == SystemDescription.Arch.X86_64:
        timer_driver = ProtectionDomain("timer_driver", "timer_driver.elf", priority=180)
        hpet_irq = SystemDescription.IrqIoapic(ioapic_id=0, pin=2, vector=40, id=0)
        timer_driver.add_irq(hpet_irq)
        sdf.add_pd(timer_driver)

        hpet_regs = SystemDescription.MemoryRegion(
            sdf, "hept_regs", 0x1000, paddr=0xFED00000
        )
        hept_regs_map = SystemDescription.Map(
            hpet_regs, 0x5000_0000, "rw", cached=False
        )
        timer_driver.add_map(hept_regs_map)
        sdf.add_mr(hpet_regs)

        lapic_timer_ch = Channel(timer_driver, vmm_client0, a_id=1, b_id=11, pp_b=True)
        hpet0_timer_ch = Channel(timer_driver, vmm_client0, a_id=2, b_id=12, pp_b=True)
        hpet1_timer_ch = Channel(timer_driver, vmm_client0, a_id=3, b_id=13, pp_b=True)
        hpet2_timer_ch = Channel(timer_driver, vmm_client0, a_id=4, b_id=14, pp_b=True)
        pit_timer_ch = Channel(timer_driver, vmm_client0, a_id=5, b_id=15, pp_b=True)

        sdf.add_channel(lapic_timer_ch)
        sdf.add_channel(hpet0_timer_ch)
        sdf.add_channel(hpet1_timer_ch)
        sdf.add_channel(hpet2_timer_ch)
        sdf.add_channel(pit_timer_ch)

    ############ VIRTIO PCI ############
    config_space = MemoryRegion(sdf, name="ecam", size=0x100000)
    sdf.add_mr(config_space)
    vmm_client0.add_map(Map(config_space, vaddr=0x100000, perms="rw"))

    memory_resource = MemoryRegion(sdf, name="memory_resource", size=0x80000)
    sdf.add_mr(memory_resource)
    vmm_client0.add_map(Map(memory_resource, vaddr=0x4000_0000, perms="rw"))

    assert serial_system.connect()
    assert serial_system.serialise_config(output_dir)
    # assert blk_system.connect()
    # assert blk_system.serialise_config(output_dir)
    assert net_system.connect()
    assert net_system.serialise_config(output_dir)
    assert client0.connect()
    assert client0.serialise_config(output_dir)

    with open(f"{output_dir}/{sdf_file}", "w+") as f:
        f.write(sdf.render())


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument("--dtb", required=False)
    parser.add_argument("--client-dtb", required=False)
    parser.add_argument("--sddf", required=True)
    parser.add_argument("--board", required=True, choices=[b.name for b in BOARDS])
    parser.add_argument("--output", required=True)
    parser.add_argument("--sdf", required=True)
    parser.add_argument("--partition")

    args = parser.parse_args()

    board = next(filter(lambda b: b.name == args.board, BOARDS))

    sdf = SystemDescription(board.arch, board.paddr_top)

    sddf = Sddf(args.sddf)

    dtb = None
    client_dtb = None
    if board.arch != SystemDescription.Arch.X86_64:
        if args.dtb is None or args.client_dtb is None:
            print("--dtb and --client-dtb must be provided for non x86 targets")

        with open(args.dtb, "rb") as f:
            dtb = DeviceTree(f.read())

        with open(args.client_dtb, "rb") as f:
            client_dtb = DeviceTree(f.read())

    generate(args.sdf, args.output, dtb, client_dtb)
