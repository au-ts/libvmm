# Copyright 2025, UNSW
# SPDX-License-Identifier: BSD-2-Clause
import argparse
from typing import Optional
from board import BOARDS, add_x86_hpet
from sdfgen import SystemDescription, Sddf, DeviceTree, Vmm
from importlib.metadata import version

assert version("sdfgen").split(".")[1] == "33", "Unexpected sdfgen version"

ProtectionDomain = SystemDescription.ProtectionDomain
VirtualMachine = SystemDescription.VirtualMachine
MemoryRegion = SystemDescription.MemoryRegion
Map = SystemDescription.Map
Channel = SystemDescription.Channel
IrqIoapic = SystemDescription.IrqIoapic


# @billn very hacky, resolve properly once PCI driver is merged in sDDF
# these need to match what the driver hardcoded
VIRTIO_NET_VQUEUES_PADDR = 0x7A00_0000
VIRTIO_BLK_VQUEUES_PADDR = 0x5FDF_0000
VIRTIO_BLK_DATA_PADDR = 0x5FFF_0000
# these need to match what QEMU sets up, check readme for more info
VIRTIO_NET_PCI_BAR_PADDR = 0xFEBF_8000
VIRTIO_NET_PCI_IRQ = 10
VIRTIO_BLK_PCI_BAR_PADDR = 0xFEBF_C000
VIRTIO_BLK_PCI_IRQ = 11


def x86_virtio_net(eth_driver):
    hw_net_rings = SystemDescription.MemoryRegion(
        sdf, "hw_net_rings", 0x10000, paddr=VIRTIO_NET_VQUEUES_PADDR
    )
    sdf.add_mr(hw_net_rings)
    hw_net_rings_map = SystemDescription.Map(hw_net_rings, 0x7000_0000, "rw", cached=False)
    eth_driver.add_map(hw_net_rings_map)

    virtio_net_regs = SystemDescription.MemoryRegion(
        sdf, "virtio_net_regs", 0x4000, paddr=VIRTIO_NET_PCI_BAR_PADDR
    )
    sdf.add_mr(virtio_net_regs)
    virtio_net_regs_map = SystemDescription.Map(
        virtio_net_regs, 0x6000_0000, "rw", cached=False
    )
    eth_driver.add_map(virtio_net_regs_map)

    virtio_net_irq = IrqIoapic(
        ioapic_id=0,
        pin=VIRTIO_NET_PCI_IRQ,
        vector=1,
        id=16,
        trigger=IrqIoapic.Trigger.LEVEL,
        polarity=IrqIoapic.Polarity.ACTIVELOW,
    )
    eth_driver.add_irq(virtio_net_irq)


def x86_virtio_blk(blk_driver):
    blk_requests_mr = SystemDescription.MemoryRegion(
        sdf, "virtio_requests", 0x10000, paddr=VIRTIO_BLK_VQUEUES_PADDR
    )
    sdf.add_mr(blk_requests_mr)
    blk_requests_map = SystemDescription.Map(blk_requests_mr, 0x2020_0000, "rw", cached=False)
    blk_driver.add_map(blk_requests_map)

    blk_virtio_metadata_mr = SystemDescription.MemoryRegion(
        sdf, "virtio_metadata", 0x10000, paddr=VIRTIO_BLK_DATA_PADDR
    )
    sdf.add_mr(blk_virtio_metadata_mr)
    blk_virtio_metadata_map = SystemDescription.Map(
        blk_virtio_metadata_mr, 0x2021_0000, "rw"
    )
    blk_driver.add_map(blk_virtio_metadata_map)

    virtio_blk_regs = SystemDescription.MemoryRegion(
        sdf, "virtio_blk_regs", 0x4000, paddr=VIRTIO_BLK_PCI_BAR_PADDR
    )
    sdf.add_mr(virtio_blk_regs)
    virtio_blk_regs_map = SystemDescription.Map(
        virtio_blk_regs, 0x6000_0000, "rw", cached=False
    )
    blk_driver.add_map(virtio_blk_regs_map)

    virtio_blk_irq = IrqIoapic(
        ioapic_id=0,
        pin=VIRTIO_BLK_PCI_IRQ,
        vector=2,
        id=17,
        trigger=IrqIoapic.Trigger.LEVEL,
        polarity=IrqIoapic.Polarity.ACTIVELOW,
    )
    blk_driver.add_irq(virtio_blk_irq)


def x86_serial(serial_driver):
    serial_port = SystemDescription.IoPort(0x3F8, 8, 0)
    serial_driver.add_ioport(serial_port)
    serial_irq = SystemDescription.IrqIoapic(0, 4, 0, id=1)
    serial_driver.add_irq(serial_irq)


def generate(
    sdf_file: str,
    output_dir: str,
    dtb: Optional[DeviceTree],
    client_dtb: Optional[DeviceTree],
):
    # Client VM
    # We build the VMM with LLVM UBSAN to detect UB which can use more than the default amount of
    # stack space.
    vmm_client0 = ProtectionDomain(
        "CLIENT_VMM", "client_vmm.elf", priority=0, stack_size=0x4000
    )
    vm_client0 = VirtualMachine("client_linux", [VirtualMachine.Vcpu(id=0)])
    client0 = Vmm(sdf, vmm_client0, vm_client0, client_dtb)
    sdf.add_pd(vmm_client0)

    # Serial subsystem
    serial_driver = ProtectionDomain("serial_driver", "serial_driver.elf", priority=200)
    serial_virt_tx = ProtectionDomain(
        "serial_virt_tx", "serial_virt_tx.elf", priority=199
    )
    # Increase the stack size as running with UBSAN uses more stack space than normal.
    serial_virt_rx = ProtectionDomain(
        "serial_virt_rx", "serial_virt_rx.elf", priority=199, stack_size=0x2000
    )

    serial_node = None
    if board.arch != SystemDescription.Arch.X86_64:
        serial_node = dtb.node(board.serial)
        assert serial_node is not None

    serial_system = Sddf.Serial(
        sdf,
        serial_node,
        serial_driver,
        serial_virt_tx,
        virt_rx=serial_virt_rx,
        enable_color=False,
    )
    serial_system.add_client(vmm_client0)

    pds = [
        serial_driver,
        serial_virt_tx,
        serial_virt_rx,
    ]
    for pd in pds:
        sdf.add_pd(pd)

    # Net subsystem
    net_node = None
    if board.arch != SystemDescription.Arch.X86_64:
        net_node = dtb.node(board.ethernet)
        assert net_node is not None

    eth_driver = ProtectionDomain(
        "eth_driver", "eth_driver.elf", priority=101, budget=100, period=400
    )
    net_virt_tx = ProtectionDomain(
        "net_virt_tx", "network_virt_tx.elf", priority=100, budget=20000
    )
    net_virt_rx = ProtectionDomain("net_virt_rx", "network_virt_rx.elf", priority=99)
    net_system = Sddf.Net(sdf, net_node, eth_driver, net_virt_tx, net_virt_rx)
    client0_net_copier = ProtectionDomain(
        "client0_net_copier", "network_copy.elf", priority=98, budget=20000
    )

    pds = [
        eth_driver,
        net_virt_rx,
        net_virt_tx,
        client0_net_copier,
    ]
    for pd in pds:
        sdf.add_pd(pd)

    net_system.add_client_with_copier(vmm_client0, client0_net_copier)

    # Block subsystem
    blk_driver = ProtectionDomain("blk_driver", "blk_driver.elf", priority=200)
    blk_virt = ProtectionDomain(
        "blk_virt", "blk_virt.elf", priority=199, stack_size=0x2000
    )

    blk_node = None
    if board.arch != SystemDescription.Arch.X86_64:
        blk_node = dtb.node(board.blk)
        assert blk_node is not None

    blk_system = Sddf.Blk(sdf, blk_node, blk_driver, blk_virt)
    partition = int(args.partition) if args.partition else board.partition
    blk_system.add_client(vmm_client0, partition=partition)
    pds = [blk_driver, blk_virt]
    for pd in pds:
        sdf.add_pd(pd)

    # Timer subsystem (Maaxboard specific as its blk driver needs a timer)
    if board.name == "maaxboard":
        timer_node = dtb.node(board.timer)
        assert timer_node is not None

        timer_driver = ProtectionDomain(
            "timer_driver", "timer_driver.elf", priority=210
        )
        timer_system = Sddf.Timer(sdf, timer_node, timer_driver)

        timer_system.add_client(blk_driver)
        sdf.add_pd(timer_driver)

        assert timer_system.connect()
        assert timer_system.serialise_config(output_dir)

    # x86 specific stuff
    elif board.name == "x86_64_generic_vtx":
        x86_serial(serial_driver)
        x86_virtio_net(eth_driver)
        x86_virtio_blk(blk_driver)

        guest_ram_mr = MemoryRegion(sdf, name="guest_ram", size=0x1000_0000)
        sdf.add_mr(guest_ram_mr)
        vmm_client0.add_map(Map(guest_ram_mr, vaddr=0x20000000, perms="rw"))
        vm_client0.add_map(Map(guest_ram_mr, vaddr=0x0, perms="rwx"))

        timer_driver = ProtectionDomain(
            "timer_driver", "timer_driver.elf", priority=254
        )
        timer_system = Sddf.Timer(sdf, None, timer_driver)
        timer_system.add_client(vmm_client0)
        sdf.add_pd(timer_driver)
        add_x86_hpet(sdf, timer_driver)
        assert timer_system.connect()
        assert timer_system.serialise_config(output_dir)

    ############ VIRTIO PCI ############
    config_space = MemoryRegion(sdf, name="ecam", size=0x100000)
    sdf.add_mr(config_space)
    vmm_client0.add_map(Map(config_space, vaddr=0x10000000, perms="rw"))

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


if __name__ == "__main__":
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

    paddr_top = board.paddr_top
    if board.name == "x86_64_generic_vtx":
        # We don't use the value from sDDF common files so that we have
        # freedom to move memory regions around.
        paddr_top = 0x5000_0000

    sdf = SystemDescription(board.arch, paddr_top)

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
