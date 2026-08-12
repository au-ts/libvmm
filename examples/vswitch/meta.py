# Copyright 2025, UNSW
# SPDX-License-Identifier: BSD-2-Clause
import argparse
from dataclasses import dataclass
from board import BOARDS, add_x86_hpet
from sdfgen import SystemDescription, Sddf, DeviceTree, Vmm
from importlib.metadata import version
from typing import Optional

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
# these need to match what QEMU sets up, check readme for more info
VIRTIO_NET_PCI_BAR_PADDR = 0xFEBF_C000
VIRTIO_NET_PCI_IRQ = 10

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
    # Client0 VM
    vmm_client0 = ProtectionDomain("CLIENT_VMM0", "client_vmm0.elf", priority=0, cpu=0, stack_size=0x4000)
    vm_client0 = VirtualMachine("client0_linux", [VirtualMachine.Vcpu(id=0)])
    client0 = Vmm(sdf, vmm_client0, vm_client0, client_dtb)
    sdf.add_pd(vmm_client0)

    # Client1 VM
    vmm_client1 = ProtectionDomain("CLIENT_VMM1", "client_vmm1.elf", priority=0, cpu=1, stack_size=0x4000)
    vm_client1 = VirtualMachine("client1_linux", [VirtualMachine.Vcpu(id=0)])
    client1 = Vmm(sdf, vmm_client1, vm_client1, client_dtb)
    sdf.add_pd(vmm_client1)

    # Client2 VM
    vmm_client2 = ProtectionDomain("CLIENT_VMM2", "client_vmm2.elf", priority=0, cpu=2, stack_size=0x4000)
    vm_client2 = VirtualMachine("client2_linux", [VirtualMachine.Vcpu(id=0)])
    client2 = Vmm(sdf, vmm_client2, vm_client2, client_dtb)
    sdf.add_pd(vmm_client2)

    # Client3 VM - Outside vswitch
    vmm_client3 = ProtectionDomain("CLIENT_VMM3", "client_vmm3.elf", priority=0, cpu=3, stack_size=0x4000)
    vm_client3 = VirtualMachine("client3_linux", [VirtualMachine.Vcpu(id=0)])
    client3 = Vmm(sdf, vmm_client3, vm_client3, client_dtb)
    sdf.add_pd(vmm_client3)

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
        enable_color=True,
    )
    serial_system.add_client(vmm_client0)
    serial_system.add_client(vmm_client1)
    serial_system.add_client(vmm_client2)
    serial_system.add_client(vmm_client3)

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
    vswitch = ProtectionDomain("net_vswitch", "network_vswitch.elf", priority=98)
    net_system = Sddf.Net(
        sdf, net_node, eth_driver, net_virt_tx, net_virt_rx, vswitch=vswitch
    )
    client0_net_copier = ProtectionDomain(
        "client0_net_copier", "network_copy0.elf", priority=97, budget=20000
    )
    client1_net_copier = ProtectionDomain(
        "client1_net_copier", "network_copy1.elf", priority=97, budget=20000
    )
    client2_net_copier = ProtectionDomain(
        "client2_net_copier", "network_copy2.elf", priority=97, budget=20000
    )
    client3_net_copier = ProtectionDomain(
        "client3_net_copier", "network_copy3.elf", priority=97, budget=20000
    )

    pds = [
        eth_driver,
        net_virt_rx,
        net_virt_tx,
        client0_net_copier,
        client1_net_copier,
        client2_net_copier,
        client3_net_copier,
        vswitch,
    ]
    for pd in pds:
        sdf.add_pd(pd)

    net_system.add_client_with_copier(
        vmm_client0, copier=client0_net_copier, vswitch=True
    )
    net_system.add_client_with_copier(
        vmm_client1, copier=client1_net_copier, vswitch=True
    )
    net_system.add_client_with_copier(
        vmm_client2, copier=client2_net_copier, vswitch=True
    )
    net_system.add_client_with_copier(
        vmm_client3, copier=client3_net_copier, vswitch=True
    )

    if board.name == "x86_64_generic_vtx":
        x86_serial(serial_driver)
        x86_virtio_net(eth_driver)

        timer_driver = ProtectionDomain(
            "timer_driver", "timer_driver.elf", priority=254
        )
        timer_system = Sddf.Timer(sdf, None, timer_driver)
        sdf.add_pd(timer_driver)
        add_x86_hpet(sdf, timer_driver)

        vmms = [vmm_client0, vmm_client1, vmm_client2, vmm_client3]
        vms = [vm_client0, vm_client1, vm_client2, vm_client3]

        for i, (vmm, vm) in enumerate(zip(vmms, vms)):
            guest_ram_mr = MemoryRegion(sdf, name=f"guest{i}_ram", size=0x1000_0000)
            sdf.add_mr(guest_ram_mr)
            vmm.add_map(Map(guest_ram_mr, vaddr=0x20000000, perms="rw"))
            vm.add_map(Map(guest_ram_mr, vaddr=0x0, perms="rwx"))

            timer_system.add_client(vmm)

        assert timer_system.connect()
        assert timer_system.serialise_config(output_dir)


    assert serial_system.connect()
    assert serial_system.serialise_config(output_dir)
    assert net_system.connect()

    # Add ACLs - by default bidirectional
    # 0 <-> 1, 3, V
    # 1 <-> 0, 3, V
    # 2 <-> V
    # 3 <-> 0, 1, V
    net_system.add_acl_rule(vmm_client0, vmm_client3)
    net_system.add_acl_rule(vmm_client0, net_virt_tx)
    net_system.add_acl_rule(vmm_client1, vmm_client0)
    net_system.add_acl_rule(vmm_client1, net_virt_tx)
    net_system.add_acl_rule(vmm_client2, net_virt_tx)
    net_system.add_acl_rule(vmm_client3, vmm_client0)
    net_system.add_acl_rule(vmm_client3, vmm_client1)
    net_system.add_acl_rule(vmm_client3, net_virt_tx)

    assert net_system.serialise_config(output_dir)
    assert client0.connect()
    assert client0.serialise_config(output_dir)
    assert client1.connect()
    assert client1.serialise_config(output_dir)
    assert client2.connect()
    assert client2.serialise_config(output_dir)
    assert client3.connect()
    assert client3.serialise_config(output_dir)

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
