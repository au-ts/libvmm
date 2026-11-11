# Copyright 2026, UNSW
# SPDX-License-Identifier: BSD-2-Clause
import argparse
from pathlib import Path
import os, sys
from typing import Optional, List

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

NUM_CLIENT_VMS = 4


def generate(
    sdf: System,
    board: Board,
    client_dtb: Optional[DeviceTreeBlob],
    output_dir: str,
    sdf_file: str,
):
    libvmm_vms: List[LibvmmVirtMachine] = []
    for i in range(0, NUM_CLIENT_VMS):
        vmm_client = ProtectionDomain(
            sdf,
            name=f"vmm_client{str(i)}",
            prog_image=f"client_vmm{str(i)}.elf",
            stack_size=0x4000,
            priority=0,
            cpu=i,
        )
        vm_client = VirtualMachine(name=f"linux{i}", vcpus=[VirtualMachine.VCPU(id=0)])
        vmm_client.add_vm(vm_client)

        libvmm_vms.append(LibvmmVirtMachine(sdf, vmm=vmm_client, guest_dtb=client_dtb))

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
    for i in range(0, NUM_CLIENT_VMS):
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

    for i in range(0, NUM_CLIENT_VMS):
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

        with open(args.dtb, "rb") as f:
            dtb = DeviceTreeBlob(args.dtb)

        with open(args.client_dtb, "rb") as f:
            client_dtb = DeviceTreeBlob(args.client_dtb)

    sdf = System(board.arch, board.paddr_top, dtb)

    generate(sdf, board, client_dtb, args.output, args.sdf)
