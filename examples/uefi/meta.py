# Copyright 2025, UNSW
# SPDX-License-Identifier: BSD-2-Clause
import argparse
from dataclasses import dataclass
from typing import List, Optional
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
        name="x86_64_generic_vtx",
        arch=SystemDescription.Arch.X86_64,
        paddr_top=0x2000_0000,
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

def x86_virtio_net(eth_driver):
    hw_net_rings = SystemDescription.MemoryRegion(
        sdf, "hw_net_rings", 0x10000, paddr=0x10000000
    )
    sdf.add_mr(hw_net_rings)
    hw_net_rings_map = SystemDescription.Map(hw_net_rings, 0x7000_0000, "rw")
    eth_driver.add_map(hw_net_rings_map)

    virtio_net_regs = SystemDescription.MemoryRegion(
        sdf, "virtio_net_regs", 0x4000, paddr=0xe0000000000
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

def x86_virtio_blk(blk_driver):
    blk_requests_mr = SystemDescription.MemoryRegion(
        sdf, "virtio_blk_requests", 65536, paddr=0x1100_0000
    )
    sdf.add_mr(blk_requests_mr)
    blk_requests_map = SystemDescription.Map(blk_requests_mr, 0x20200000, "rw")
    blk_driver.add_map(blk_requests_map)

    blk_virtio_metadata_mr = SystemDescription.MemoryRegion(
        sdf, "virtio_blk_metadata", 65536, paddr=0x1200_0000
    )
    sdf.add_mr(blk_virtio_metadata_mr)
    blk_virtio_metadata_map = SystemDescription.Map(
        blk_virtio_metadata_mr, 0x20210000, "rw"
    )
    blk_driver.add_map(blk_virtio_metadata_map)

    virtio_blk_regs = SystemDescription.MemoryRegion(
        sdf, "virtio_blk_regs", 0x4000, paddr=0xe0000004000
    )
    sdf.add_mr(virtio_blk_regs)
    virtio_blk_regs_map = SystemDescription.Map(
        virtio_blk_regs, 0x6000_0000, "rw", cached=False
    )
    blk_driver.add_map(virtio_blk_regs_map)

    virtio_blk_irq = SystemDescription.IrqIoapic(
        ioapic_id=0, pin=11, vector=45, id=17
    )
    blk_driver.add_irq(virtio_blk_irq)

def generate(sdf_file: str, output_dir: str, dtb: Optional[DeviceTree], client_dtb: Optional[DeviceTree]):
    # Client VM
    vmm_client0 = ProtectionDomain("CLIENT_VMM", "vmm_x86_64.elf", priority=1)
    vm_client0 = VirtualMachine("client_linux", [VirtualMachine.Vcpu(id=0)])
    client0 = Vmm(sdf, vmm_client0, vm_client0, client_dtb)
    sdf.add_pd(vmm_client0)

    guest_ram_mr = MemoryRegion(sdf, name="guest_ram", size=0xB000_0000, paddr=0x20000000)
    sdf.add_mr(guest_ram_mr)
    vmm_client0.add_map(Map(guest_ram_mr, vaddr=0x3000_0000, perms="rw"))
    vm_client0.add_map(Map(guest_ram_mr, vaddr=0x0, perms="rwx"))

    # Second guest RAM region above 4G, note that this is one-to-one mapped between GPA and VMM Vaddr
    guest_ram_high_mr = MemoryRegion(sdf, name="guest_ram_high", size=0x1_8000_0000, paddr=0x1_0000_0000)
    sdf.add_mr(guest_ram_high_mr)
    vmm_client0.add_map(Map(guest_ram_high_mr, vaddr=0x100000000, perms="rw"))
    vm_client0.add_map(Map(guest_ram_high_mr, vaddr=0x100000000, perms="rwx"))

    scratch_mr = MemoryRegion(sdf, name="guest_scratch", size=0x10000)
    sdf.add_mr(scratch_mr)
    vm_client0.add_map(Map(scratch_mr, vaddr=0xfe000000, perms="rwx"))

    guest_flash_mr = MemoryRegion(sdf, name="guest_flash", size=0x600000)
    sdf.add_mr(guest_flash_mr)
    vmm_client0.add_map(Map(guest_flash_mr, vaddr=0x200_0000, perms="rw"))
    vm_client0.add_map(Map(guest_flash_mr, vaddr=0xffa0_0000, perms="rwx"))

    qemu_fw_cfg_sel_port = SystemDescription.IoPort(0x510, 8, 20)
    vmm_client0.add_ioport(qemu_fw_cfg_sel_port)

    qemu_fw_cfg_dma_port = SystemDescription.IoPort(0x518, 4, 21)
    vmm_client0.add_ioport(qemu_fw_cfg_dma_port)

    # PS/2 KB+M passthrough
    # @billn, the ps2 data port is only 1 byte, but GRUB seems to hang if we only pass through
    # 0x60 and return zero on 0x61, 0x62, and 0x63 in software. Need to investigate what is at
    # those addresses and change 4 to 1 below
    ps2_data_port = SystemDescription.IoPort(0x60, 4, 22)
    vmm_client0.add_ioport(ps2_data_port)

    ps2_sts_cmd_port = SystemDescription.IoPort(0x64, 1, 23)
    vmm_client0.add_ioport(ps2_sts_cmd_port)

    ps2_first_irq = SystemDescription.IrqIoapic(0, 1, 32, id=3)
    vmm_client0.add_irq(ps2_first_irq)

    ps2_second_irq = SystemDescription.IrqIoapic(0, 12, 33, id=4)
    vmm_client0.add_irq(ps2_second_irq)

    # CD-ROM passthrough
    primary_ata_cmd_port = SystemDescription.IoPort(0x1f0, 8, 30)
    vmm_client0.add_ioport(primary_ata_cmd_port)

    primary_ata_ctrl_port = SystemDescription.IoPort(0x3f6, 1, 31)
    vmm_client0.add_ioport(primary_ata_ctrl_port)

    second_ata_cmd_port = SystemDescription.IoPort(0x170, 8, 32)
    vmm_client0.add_ioport(second_ata_cmd_port)

    second_ata_ctrl_port = SystemDescription.IoPort(0x376, 1, 33)
    vmm_client0.add_ioport(second_ata_ctrl_port)

    primary_ata_irq = SystemDescription.IrqIoapic(0, 14, 50, id=5)
    vmm_client0.add_irq(primary_ata_irq)

    second_ata_irq = SystemDescription.IrqIoapic(0, 15, 51, id=6)
    vmm_client0.add_irq(second_ata_irq)

    # PCI Host bridge access for VMM, for CD-ROM passthrough to work correctly.
    pci_conf_addr_port = SystemDescription.IoPort(0xcf8, 4, 34)
    vmm_client0.add_ioport(pci_conf_addr_port)

    pci_conf_data_port = SystemDescription.IoPort(0xcfc, 4, 35)
    vmm_client0.add_ioport(pci_conf_data_port)

    # CMOS passthrough hack for windows
    cmos_addr_port = SystemDescription.IoPort(0x70, 1, 36)
    vmm_client0.add_ioport(cmos_addr_port)

    cmos_data_port = SystemDescription.IoPort(0x71, 1, 37)
    vmm_client0.add_ioport(cmos_data_port)

    # QEMU Framebuffer
    fb_mr = MemoryRegion(sdf, name="fb", size=0x200_000, paddr=0x700_0000)
    vmm_client0.add_map(Map(fb_mr, vaddr=0x800000, perms="rw"))
    sdf.add_mr(fb_mr)

    fw_cfg_dma_cmd_mr = MemoryRegion(sdf, name="fw_cfg_dma_cmd", size=0x1000, paddr=0x600_0000)
    vmm_client0.add_map(Map(fw_cfg_dma_cmd_mr, vaddr=0xD00000, perms="rw"))
    sdf.add_mr(fw_cfg_dma_cmd_mr)

    # Serial subsystem
    serial_driver = ProtectionDomain("serial_driver", "serial_driver.elf", priority=200)
    serial_virt_tx = ProtectionDomain("serial_virt_tx", "serial_virt_tx.elf", priority=199)
    # Increase the stack size as running with UBSAN uses more stack space than normal.
    serial_virt_rx = ProtectionDomain("serial_virt_rx", "serial_virt_rx.elf",
                                      priority=199, stack_size=0x2000)

    serial_port = SystemDescription.IoPort(0x3F8, 8, 0)
    serial_driver.add_ioport(serial_port)
    serial_irq = SystemDescription.IrqIoapic(0, 4, 34, id=1)
    serial_driver.add_irq(serial_irq)

    serial_system = Sddf.Serial(sdf, None, serial_driver,
                                serial_virt_tx, virt_rx=serial_virt_rx, enable_color=True)
    serial_system.add_client(vmm_client0)

    pds = [
        serial_driver,
        serial_virt_tx,
        serial_virt_rx,
    ]
    for pd in pds:
        sdf.add_pd(pd)

    eth_driver = ProtectionDomain("eth_driver", "eth_driver.elf",
                                  priority=254)
    net_virt_tx = ProtectionDomain("net_virt_tx", "network_virt_tx.elf", priority=100, budget=20000)
    net_virt_rx = ProtectionDomain("net_virt_rx", "network_virt_rx.elf", priority=99)
    net_system = Sddf.Net(sdf, None, eth_driver, net_virt_tx, net_virt_rx)
    client0_net_copier = ProtectionDomain(
        "client0_net_copier", "network_copy.elf", priority=98, budget=20000)

    x86_virtio_net(eth_driver)

    pds = [
        eth_driver,
        net_virt_rx,
        net_virt_tx,
        client0_net_copier,
    ]
    for pd in pds:
        sdf.add_pd(pd)

    net_system.add_client_with_copier(vmm_client0, client0_net_copier, mac_addr="52:54:01:00:00:10")

    # Block subsystem
    blk_driver = ProtectionDomain("blk_driver", "blk_driver.elf", priority=200)
    blk_virt = ProtectionDomain("blk_virt", "blk_virt.elf", priority=199, stack_size=0x2000)

    blk_system = Sddf.Blk(sdf, None, blk_driver, blk_virt)
    partition = int(args.partition) if args.partition else board.partition
    blk_system.add_client(vmm_client0, partition=partition)

    x86_virtio_blk(blk_driver)

    pds = [
        blk_driver,
        blk_virt
    ]
    for pd in pds:
        sdf.add_pd(pd)

    # Timer subsystem
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
    vmm_client0.add_map(Map(config_space, vaddr=0x800_0000, perms="rw"))

    memory_resource = MemoryRegion(sdf, name="memory_resource", size=0x80000)
    sdf.add_mr(memory_resource)
    vmm_client0.add_map(Map(memory_resource, vaddr=0x100_0000, perms="rw"))

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
