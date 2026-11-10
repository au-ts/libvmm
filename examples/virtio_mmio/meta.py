# Copyright 2026, UNSW
# SPDX-License-Identifier: BSD-2-Clause
import argparse
from pathlib import Path
import os, sys

from acacia import (
    System,
    ProtectionDomain,
    VirtualMachine,
    DeviceTreeBlob,
    SchedulingProperties,
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

VIRTIO_MMIO_CONSOLE_DT_NOTE_PATH = "/virtio-console"
VIRTIO_MMIO_BLK_DT_NOTE_PATH = "/virtio-blk"
VIRTIO_MMIO_NET_DT_NOTE_PATH = "/virtio-net"


def generate(
    sdf: System,
    board: Board,
    client_dtb: DeviceTreeBlob,
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
    libvmm_vm.add_virtio_mmio_console(
        serial, node_path=VIRTIO_MMIO_CONSOLE_DT_NOTE_PATH
    )

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
    libvmm_vm.add_virtio_mmio_net(
        ethernet, copier=client_net_copier, node_path=VIRTIO_MMIO_NET_DT_NOTE_PATH
    )

    # Block subsystem
    blk = sDDFBlk(
        sdf, board.blk.compatible, board.blk.node_path, driver_prio=200, virt_prio=199
    )
    libvmm_vm.add_virtio_mmio_blk(
        blk, partition_number=partition, node_path=VIRTIO_MMIO_BLK_DT_NOTE_PATH
    )

    # Timer subsystem (Maaxboard specific as its blk driver needs a timer)
    if board.name == "maaxboard":
        timer = sDDFTimer(sdf, board.timer.compatible, board.timer.node_path)
        timer.add_client(blk.driver)

    out_file = f"{output_dir}/{sdf_file}"
    sdf.make_config_structs(Path(output_dir))
    print(f"Saving to {out_file}")
    sdf.write_xml_file(out_file)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--dtb", required=True)
    parser.add_argument("--client-dtb", required=True)
    parser.add_argument("--sddf", required=True)
    parser.add_argument("--board", required=True, choices=[b.name for b in BOARDS])
    parser.add_argument("--output", required=True)
    parser.add_argument("--sdf", required=True)
    parser.add_argument("--partition")

    args = parser.parse_args()

    board = next(filter(lambda b: b.name == args.board, BOARDS))

    dtb = DeviceTreeBlob(args.dtb)
    client_dtb = DeviceTreeBlob(args.client_dtb)
    sdf = System(board.arch, board.paddr_top, dtb)
    partition = int(args.partition) if args.partition else board.partition

    generate(sdf, board, client_dtb, partition, args.output, args.sdf)
