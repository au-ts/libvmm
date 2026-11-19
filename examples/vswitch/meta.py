# Copyright 2026, UNSW
# SPDX-License-Identifier: BSD-2-Clause
import argparse
import sys
from pathlib import Path

from acacia import (
    Arch,
    DeviceTreeBlob,
    IrqIoapic,
    Map,
    MemoryRegion,
    ProtectionDomain,
    SchedulingProperties,
    System,
    VirtualMachine,
)
from acacia_sddf import BOARDS, Board, sDDFEthernet, sDDFSerial

from acacia_libvmm import LibvmmVirtMachine

NUM_CLIENT_VMS = 4

# @billn very hacky, resolve properly once PCI driver is merged in sDDF
# these need to match what the driver hardcoded
X86_VIRTIO_NET_VQUEUES_PADDR = 0x7A00_0000
# these need to match what QEMU sets up, check readme for more info
X86_VIRTIO_NET_PCI_BAR_PADDR = 0xFEBF_C000
X86_VIRTIO_NET_PCI_IRQ = 10

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


def generate(
    sdf: System,
    board: Board,
    client_dtb: DeviceTreeBlob | None,
    output_dir: str,
    sdf_file: str,
):
    libvmm_vms: list[LibvmmVirtMachine] = []
    for i in range(NUM_CLIENT_VMS):
        vmm_client = ProtectionDomain(
            sdf,
            name=f"vmm_client{i!s}",
            prog_image=f"client_vmm{i!s}.elf",
            stack_size=0x4000,
            priority=0,
            cpu=i,
        )
        vm_client = VirtualMachine(name=f"linux{i}", vcpus=[VirtualMachine.VCPU(id=0)])
        vmm_client.add_vm(vm_client)

        libvmm_vm = LibvmmVirtMachine(sdf, vmm=vmm_client, guest_dtb=client_dtb)
        if board.name == "x86_64_generic_vtx":
            libvmm_vm.add_x86_ram(X86_GUEST_RAM_SIZE)

        libvmm_vms.append(libvmm_vm)

    # Serial subsystem
    serial = sDDFSerial(
        sdf,
        board.serial.compatible,
        board.serial.node_path,
        driver_prio=200,
        virt_tx_prio=199,
        allow_rx=True,
        enable_color=True,
        baud_rate=board.baud_rate if board.baud_rate else 115200,
    )
    for i in range(NUM_CLIENT_VMS):
        serial.add_client(libvmm_vms[i].vmm)

    # Net subsystem
    vswitch = ProtectionDomain(
        sdf,
        "net_vswitch",
        "network_vswitch.elf",
        scheduling=SchedulingProperties(priority=98),
    )
    ethernet = sDDFEthernet(
        sdf,
        board.ethernet.compatible,
        board.ethernet.node_path,
        driver_prio=101,
        virt_tx_prio=100,
        virt_rx_prio=99,
        vswitch=vswitch,
    )

    if board.name == "x86_64_generic_vtx":
        x86_virtio_net(sdf, ethernet.driver)

    for i in range(NUM_CLIENT_VMS):
        client_copier = ProtectionDomain(
            sdf,
            f"client{i}_net_copier",
            f"network_copy{i}.elf",
            scheduling=SchedulingProperties(priority=98, budget=20000),
            cpu=i,
        )
        ethernet.add_client(
            libvmm_vms[i].vmm, copier=client_copier, vswitch_client=True
        )

    # Add ACLs - by default bidirectional
    # 0 <-> 1, 3, V
    # 1 <-> 0, 3, V
    # 2 <-> V
    # 3 <-> 0, 1, V
    ethernet.add_acl_rule(libvmm_vms[0].vmm, libvmm_vms[3].vmm)
    ethernet.add_acl_rule(libvmm_vms[0].vmm, ethernet.virt_tx)
    ethernet.add_acl_rule(libvmm_vms[1].vmm, libvmm_vms[0].vmm)
    ethernet.add_acl_rule(libvmm_vms[1].vmm, ethernet.virt_tx)
    ethernet.add_acl_rule(libvmm_vms[2].vmm, ethernet.virt_tx)
    ethernet.add_acl_rule(libvmm_vms[3].vmm, libvmm_vms[0].vmm)
    ethernet.add_acl_rule(libvmm_vms[3].vmm, libvmm_vms[1].vmm)
    ethernet.add_acl_rule(libvmm_vms[3].vmm, ethernet.virt_tx)

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

    args = parser.parse_args()

    board = next(filter(lambda b: b.name == args.board, BOARDS))

    dtb = None
    client_dtb = None
    if not Arch.is_x86(board.arch):
        if args.dtb is None or args.client_dtb is None:
            print("--dtb and --client-dtb must be provided for non x86 targets")
            sys.exit(1)

        with open(args.dtb, "rb") as f:
            dtb = DeviceTreeBlob(args.dtb)

        with open(args.client_dtb, "rb") as f:
            client_dtb = DeviceTreeBlob(args.client_dtb)

    sdf = System(board.arch, board.paddr_top, dtb)

    generate(sdf, board, client_dtb, args.output, args.sdf)
