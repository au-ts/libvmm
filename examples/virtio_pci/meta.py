# Copyright 2026, UNSW
# SPDX-License-Identifier: BSD-2-Clause
import argparse
from pathlib import Path
import os, sys
from typing import Optional

from acacia import (
    System,
    Arch,
    Map,
    MemoryRegion,
    ProtectionDomain,
    VirtualMachine,
    DeviceTreeBlob,
    SchedulingProperties,
    IrqIoapic,
    IOPort,
)

sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), "../../"))
from acacia_libvmm import (
    BOARDS,
    Board,
    sDDFBlk,
    sDDFSerial,
    sDDFTimer,
    sDDFEthernet,
    LibvmmVirtMachine,
)

# @billn very hacky, resolve properly once PCI driver is merged in sDDF
# these need to match what the driver hardcoded
X86_VIRTIO_NET_VQUEUES_PADDR = 0x7A00_0000
X86_VIRTIO_BLK_VQUEUES_PADDR = 0x5FDF_0000
X86_VIRTIO_BLK_DATA_PADDR = 0x5FFF_0000
# these need to match what QEMU sets up, check readme for more info
X86_VIRTIO_NET_PCI_BAR_PADDR = 0xFEBF_8000
X86_VIRTIO_NET_PCI_IRQ = 10
X86_VIRTIO_BLK_PCI_BAR_PADDR = 0xFEBF_C000
X86_VIRTIO_BLK_PCI_IRQ = 11

X86_GUEST_RAM_SIZE = 0x1000_0000


def x86_virtio_net(sdf: System, eth_driver: ProtectionDomain):
    hw_net_rings = MemoryRegion(
        sdf, "hw_net_rings", 0x10000, paddr=X86_VIRTIO_NET_VQUEUES_PADDR, cached=False
    )
    hw_net_rings_map = Map(hw_net_rings, 0x7000_0000, "rw")
    eth_driver.add_map(hw_net_rings_map)

    virtio_net_regs = MemoryRegion(
        sdf, "virtio_net_regs", 0x4000, paddr=X86_VIRTIO_NET_PCI_BAR_PADDR, cached=False
    )
    virtio_net_regs_map = Map(virtio_net_regs, 0x6000_0000, "rw")
    eth_driver.add_map(virtio_net_regs_map)

    virtio_net_irq = IrqIoapic(
        ioapic_id=0,
        pin=X86_VIRTIO_NET_PCI_IRQ,
        vector=1,
        id=16,
        trigger=IrqIoapic.Trigger.LEVEL,
        polarity=IrqIoapic.Polarity.ACTIVELOW,
    )
    eth_driver.add_irq(virtio_net_irq)


def x86_virtio_blk(sdf: System, blk_driver: ProtectionDomain):
    blk_requests_mr = MemoryRegion(
        sdf,
        "virtio_requests",
        0x10000,
        paddr=X86_VIRTIO_BLK_VQUEUES_PADDR,
        cached=False,
    )
    blk_requests_map = Map(blk_requests_mr, 0x2020_0000, "rw")
    blk_driver.add_map(blk_requests_map)

    blk_virtio_metadata_mr = MemoryRegion(
        sdf, "virtio_metadata", 0x10000, paddr=X86_VIRTIO_BLK_DATA_PADDR
    )
    blk_virtio_metadata_map = Map(blk_virtio_metadata_mr, 0x2021_0000, "rw")
    blk_driver.add_map(blk_virtio_metadata_map)

    virtio_blk_regs = MemoryRegion(
        sdf, "virtio_blk_regs", 0x4000, paddr=X86_VIRTIO_BLK_PCI_BAR_PADDR, cached=False
    )
    virtio_blk_regs_map = Map(virtio_blk_regs, 0x6000_0000, "rw")
    blk_driver.add_map(virtio_blk_regs_map)

    virtio_blk_irq = IrqIoapic(
        ioapic_id=0,
        pin=X86_VIRTIO_BLK_PCI_IRQ,
        vector=2,
        id=17,
        trigger=IrqIoapic.Trigger.LEVEL,
        polarity=IrqIoapic.Polarity.ACTIVELOW,
    )
    blk_driver.add_irq(virtio_blk_irq)


def generate(
    sdf: System,
    board: Board,
    client_dtb: Optional[DeviceTreeBlob],
    partition: int,
    output_dir: str,
    sdf_file: str,
):
    # Client VM
    # We build the VMM with LLVM UBSAN to detect UB which can use more than the default amount of
    # stack space.
    vmm_client = ProtectionDomain(
        sdf,
        name="vmm_client",
        prog_image="client_vmm.elf",
        stack_size=0x4000,
        priority=0,
    )
    vm_client = VirtualMachine(name="linux", vcpus=[VirtualMachine.VCPU(id=0)])
    vmm_client.add_vm(vm_client)

    libvmm_vm = LibvmmVirtMachine(sdf, vmm=vmm_client, guest_dtb=client_dtb)

    # Serial subsystem
    serial = sDDFSerial(
        sdf,
        board.serial.compatible,
        board.serial.node_path,
        driver_prio=200,
        virt_tx_prio=199,
        allow_rx=True,
        enable_color=False,
        baud_rate=board.baud_rate if board.baud_rate else 115200,
    )
    serial.add_client(libvmm_vm.vmm)

    # Net subsystem
    ethernet = sDDFEthernet(
        sdf,
        board.ethernet.compatible,
        board.ethernet.node_path,
        driver_prio=101,
        virt_tx_prio=100,
        virt_rx_prio=99,
    )
    client_net_copier = ProtectionDomain(
        sdf,
        name="client_net_copier",
        prog_image="network_copy.elf",
        scheduling=SchedulingProperties(priority=98, budget=20000),
    )
    ethernet.add_client(libvmm_vm.vmm, copier=client_net_copier)

    # Block subsystem
    blk = sDDFBlk(
        sdf, board.blk.compatible, board.blk.node_path, driver_prio=200, virt_prio=199
    )
    blk.add_client(libvmm_vm.vmm, partition_number=partition)

    # Timer subsystem (Maaxboard specific as its blk driver needs a timer)
    if board.name == "maaxboard":
        timer = sDDFTimer(sdf, board.timer.compatible, board.timer.node_path)
        timer.add_client(blk.driver)

    # x86 specific stuff
    elif board.name == "x86_64_generic_vtx":
        x86_virtio_net(sdf, ethernet.driver)
        x86_virtio_blk(sdf, blk.driver)

        libvmm_vm.add_x86_ram(X86_GUEST_RAM_SIZE)

        timer = sDDFTimer(sdf, board.timer.compatible, board.timer.node_path)
        timer.add_client(libvmm_vm.vmm)

    out_file = f"{output_dir}/{sdf_file}"
    sdf.make_config_structs(Path(output_dir))
    print(f"Saving to {out_file}")
    sdf.write_xml_file(out_file)


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

    dtb = None
    client_dtb = None
    if not Arch.is_x86(board.arch):
        if args.dtb is None or args.client_dtb is None:
            print("--dtb and --client-dtb must be provided for non x86 targets")

        with open(args.dtb, "rb") as f:
            dtb = DeviceTreeBlob(args.dtb)

        with open(args.client_dtb, "rb") as f:
            client_dtb = DeviceTreeBlob(args.client_dtb)

    sdf = System(board.arch, board.paddr_top, dtb)
    partition = int(args.partition) if args.partition else board.partition

    generate(sdf, board, client_dtb, partition, args.output, args.sdf)
