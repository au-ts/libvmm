# Copyright 2025, UNSW
# SPDX-License-Identifier: BSD-2-Clause
import argparse
from typing import Optional
from board import BOARDS, add_x86_hpet
from sdfgen import SystemDescription, Sddf, DeviceTree, Vmm
from importlib.metadata import version

assert version("sdfgen").split(".")[1] == "35", "Unexpected sdfgen version"

ProtectionDomain = SystemDescription.ProtectionDomain
VirtualMachine = SystemDescription.VirtualMachine
MemoryRegion = SystemDescription.MemoryRegion
Map = SystemDescription.Map
Channel = SystemDescription.Channel
IrqIoapic = SystemDescription.IrqIoapic

# Memory Regions for Intel APICv operations, need to be in normal memory
APICV_VIRTUAL_APIC_PADDR = 0x1300_0000
APICV_APIC_ACCESS_PADDR = 0x1300_1000

# @billn very hacky, resolve properly once PCI driver is merged in sDDF
# these need to match what the driver hardcoded
VIRTIO_NET_VQUEUES_PADDR = 0x7A00_0000
VIRTIO_BLK_VQUEUES_PADDR = 0x5FDF_0000
VIRTIO_BLK_DATA_PADDR = 0x5FFF_0000
# these need to match what QEMU sets up, check readme for more info
VIRTIO_NET_PCI_BAR_PADDR = 0xE00_0000_0000
VIRTIO_NET_PCI_IRQ = 10
VIRTIO_BLK_PCI_BAR_PADDR = 0xE00_0000_4000
VIRTIO_BLK_PCI_IRQ = 11
BOCHS_DISPLAY_FB_BAR_PADDR = 0xFD000000
BOCHS_DISPLAY_FB_SIZE = 0x1000000
BOCHS_DISPLAY_REGS_BAR_PADDR = 0xFEBCA000
# these need to match how the guest OVMF sets up the virtual bus at run time:
# Look for this in the boot log:
# PciBus: HostBridge->NotifyPhase(AllocateResources) - Success
# Process Option ROM: BAR Base/Length = 0/0
# PciBus: Resource Map for Root Bridge PciRoot(0x0)
# Type =  Mem32; Base = 0xE0000000;       Length = 0x1100000;     Alignment = 0xFFFFFF
#    Base = 0xE0000000;   Length = 0x1000000;     Alignment = 0xFFFFFF;   Owner = PCI [00|03|00:10]
#    Base = 0xE1000000;   Length = 0x4000;        Alignment = 0x3FFF;     Owner = PCI [00|05|00:10]
#    Base = 0xE1004000;   Length = 0x4000;        Alignment = 0x3FFF;     Owner = PCI [00|04|00:10]
#    Base = 0xE1008000;   Length = 0x1000;        Alignment = 0xFFF;      Owner = PCI [00|03|00:18]
BOCHS_DISPLAY_FB_BAR_GPA = 0xE0000000
BOCHS_DISPLAY_REGS_BAR_GPA = 0xE1008000


def x86_bochs_display(vm: VirtualMachine):
    fb_mr = MemoryRegion(
        sdf,
        name="framebuffer",
        size=BOCHS_DISPLAY_FB_SIZE,
        paddr=BOCHS_DISPLAY_FB_BAR_PADDR,
    )
    sdf.add_mr(fb_mr)
    vm.add_map(Map(fb_mr, vaddr=BOCHS_DISPLAY_FB_BAR_GPA, perms="rwx"))

    regs_mr = MemoryRegion(
        sdf, name="video_regs", size=0x1000, paddr=BOCHS_DISPLAY_REGS_BAR_PADDR
    )
    sdf.add_mr(regs_mr)
    vm.add_map(Map(regs_mr, vaddr=BOCHS_DISPLAY_REGS_BAR_GPA, perms="rwx"))


def x86_virtio_net(eth_driver):
    hw_net_rings = SystemDescription.MemoryRegion(
        sdf, "hw_net_rings", 0x10000, paddr=VIRTIO_NET_VQUEUES_PADDR
    )
    sdf.add_mr(hw_net_rings)
    hw_net_rings_map = SystemDescription.Map(
        hw_net_rings, 0x7000_0000, "rw", cached=False
    )
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
    blk_requests_map = SystemDescription.Map(
        blk_requests_mr, 0x2020_0000, "rw", cached=False
    )
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


def x86_ps2_keyboard_mouse(vmm: ProtectionDomain):
    # PS/2 KB+M passthrough
    # @billn, the ps2 data port is only 1 byte, but GRUB seems to hang if we only pass through
    # 0x60 and return zero on 0x61, 0x62, and 0x63 in software. Need to investigate what is at
    # those addresses and change 4 to 1 below
    ps2_data_port = SystemDescription.IoPort(addr=0x60, size=1, id=40)
    vmm.add_ioport(ps2_data_port)

    ps2_sts_cmd_port = SystemDescription.IoPort(addr=0x64, size=1, id=41)
    vmm.add_ioport(ps2_sts_cmd_port)

    ps2_first_irq = SystemDescription.IrqIoapic(0, 1, 32, id=42)
    vmm.add_irq(ps2_first_irq)

    ps2_second_irq = SystemDescription.IrqIoapic(0, 12, 33, id=43)
    vmm.add_irq(ps2_second_irq)


def x86_serial(serial_client):
    serial_port = SystemDescription.IoPort(0x3F8, 8, id=50)
    serial_client.add_ioport(serial_port)
    serial_irq = SystemDescription.IrqIoapic(0, 4, 0, id=51)
    serial_client.add_irq(serial_irq)


def x86_apicv(vmm: ProtectionDomain, vm: VirtualMachine):
    guest_virtual_apic_mr = MemoryRegion(
        sdf, name="guest_virtual_apic", size=0x1000, paddr=APICV_VIRTUAL_APIC_PADDR
    )
    guest_apic_access_mr = MemoryRegion(
        sdf, name="guest_apic_access", size=0x1000, paddr=APICV_APIC_ACCESS_PADDR
    )
    sdf.add_mr(guest_virtual_apic_mr)
    sdf.add_mr(guest_apic_access_mr)

    vmm.add_map(Map(guest_virtual_apic_mr, vaddr=0x30_0000_0000, perms="rw"))
    vm.add_map(Map(guest_apic_access_mr, vaddr=0xFEE0_0000, perms="rw"))


def generate(
    sdf_file: str,
    output_dir: str,
):
    # Client VM
    # We build the VMM with LLVM UBSAN to detect UB which can use more than the default amount of
    # stack space.
    vmm_client0 = ProtectionDomain(
        "CLIENT_VMM", "client_vmm.elf", priority=0, stack_size=0x8000
    )
    vm_client0 = VirtualMachine("client_linux", [VirtualMachine.Vcpu(id=0)])
    client0 = Vmm(sdf, vmm_client0, vm_client0, None)
    sdf.add_pd(vmm_client0)

    # Net subsystem
    eth_driver = ProtectionDomain(
        "eth_driver", "eth_driver.elf", priority=101, budget=100, period=400
    )
    net_virt_tx = ProtectionDomain(
        "net_virt_tx", "network_virt_tx.elf", priority=100, budget=20000
    )
    net_virt_rx = ProtectionDomain("net_virt_rx", "network_virt_rx.elf", priority=99)
    net_system = Sddf.Net(sdf, None, eth_driver, net_virt_tx, net_virt_rx)
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
    blk_system = Sddf.Blk(sdf, None, blk_driver, blk_virt)
    partition = int(args.partition) if args.partition else board.partition
    blk_system.add_client(vmm_client0, partition=partition, data_size=0x8000000) # 128MiB available for block reqs
    pds = [blk_driver, blk_virt]
    for pd in pds:
        sdf.add_pd(pd)

    if board.name == "x86_64_generic_vtx":
        x86_serial(vmm_client0)
        x86_virtio_net(eth_driver)
        x86_virtio_blk(blk_driver)
        x86_bochs_display(vm_client0)
        x86_ps2_keyboard_mouse(vmm_client0)
        x86_apicv(vmm_client0, vm_client0)

        # 8GB RAM

        guest_ram_low_mr = MemoryRegion(sdf, name="guest_ram_low", size=0xD000_0000)
        sdf.add_mr(guest_ram_low_mr)
        vmm_client0.add_map(Map(guest_ram_low_mr, vaddr=0x20000000, perms="rw"))
        vm_client0.add_map(Map(guest_ram_low_mr, vaddr=0x0, perms="rwx"))

        guest_ram_high_mr = MemoryRegion(sdf, name="guest_ram_high", size=0x1_2000_0000)
        sdf.add_mr(guest_ram_high_mr)
        vmm_client0.add_map(Map(guest_ram_high_mr, vaddr=0x1_0000_0000, perms="rw"))
        vm_client0.add_map(Map(guest_ram_high_mr, vaddr=0x1_0000_0000, perms="rwx"))

        guest_flash_mr = MemoryRegion(sdf, name="guest_flash", size=0x60_0000)
        sdf.add_mr(guest_flash_mr)
        vmm_client0.add_map(Map(guest_flash_mr, vaddr=0x10000000, perms="rw"))
        # Flash's GPA + size == top of 4G
        vm_client0.add_map(Map(guest_flash_mr, vaddr=0xFFA00000, perms="rwx"))

        timer_driver = ProtectionDomain(
            "timer_driver", "timer_driver.elf", priority=254
        )
        timer_system = Sddf.Timer(sdf, None, timer_driver)
        timer_system.add_client(vmm_client0)
        sdf.add_pd(timer_driver)
        add_x86_hpet(sdf, timer_driver)
        assert timer_system.connect()
        assert timer_system.serialise_config(output_dir)

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

    if board.name != "x86_64_generic_vtx":
        print("--board must be x86_64_generic_vtx")
        exit(1)

    # We don't use the value from sDDF common files so that we have
    # freedom to move memory regions around.
    paddr_top = 0x5000_0000

    sdf = SystemDescription(board.arch, paddr_top)

    sddf = Sddf(args.sddf)

    generate(args.sdf, args.output)
