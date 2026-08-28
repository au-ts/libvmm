# Copyright 2025, UNSW
# SPDX-License-Identifier: BSD-2-Clause
import argparse
import struct
import random
from dataclasses import dataclass
from typing import List, Tuple
from sdfgen import SystemDescription, Sddf, DeviceTree, Vmm
from importlib.metadata import version

# @billn update once https://github.com/au-ts/microkit_sdf_gen/pull/25 is merged
# assert version("sdfgen").split(".")[1] == "26", "Unexpected sdfgen version"

ProtectionDomain = SystemDescription.ProtectionDomain
MemoryRegion = SystemDescription.MemoryRegion
Map = SystemDescription.Map
Channel = SystemDescription.Channel
VirtualMachine = SystemDescription.VirtualMachine
Irq = SystemDescription.Irq


@dataclass
class Board:
    name: str
    arch: SystemDescription.Arch
    paddr_top: int
    serial: str
    guest_serial: str
    timer: str
    ethernet: str
    passthrough: list[str]


BOARDS: List[Board] = [
    Board(
        name="odroidc4",
        arch=SystemDescription.Arch.AARCH64,
        paddr_top=0x60000000,
        serial="soc/bus@ff800000/serial@3000",
        guest_serial="virtio-console@100000",
        timer="soc/bus@ffd00000/watchdog@f0d0",
        ethernet="soc/ethernet@ff3f0000",
        passthrough=[
            "soc/bus@ff600000/bus@34400",   # pinmux
            "soc/bus@ff600000/bus@3c000",   # power controller
            "soc/bus@ff600000/mdio-multiplexer@4c000", # ethernet PHY
            "soc/bus@ff800000/sys-ctrl@0",  # pinmux
        ],
    ),
]

"""
Below are classes to serialise into custom configuration for the benchmarking component.
All serialised definitions are little endian and pointers are 64-bit integers.
"""


class BenchmarkIdleConfig:
    def __init__(self, cycle_counters: int, ch_init: int):
        self.cycle_counters = cycle_counters
        self.ch_init = ch_init

    """
        Matches struct definition:
        {
            void *;
            uint8_t;
        }
    """

    def serialise(self) -> bytes:
        return struct.pack(
            "<qc", self.cycle_counters, self.ch_init.to_bytes(1, "little")
        )


class BenchmarkClientConfig:
    def __init__(self, cycle_counters: int, ch_start: int, ch_stop: int):
        self.cycle_counters = cycle_counters
        self.ch_start = ch_start
        self.ch_stop = ch_stop

    """
        Matches struct definition:
        {
            void *;
            uint8_t;
            uint8_t;
        }
    """

    def serialise(self) -> bytes:
        return struct.pack("<qBB", self.cycle_counters, self.ch_start, self.ch_stop)


class BenchmarkConfig:
    def __init__(
        self, ch_start: int, ch_stop: int, ch_init: int, children: List[Tuple[int, str]]
    ):
        self.ch_start = ch_start
        self.ch_stop = ch_stop
        self.ch_init = ch_init
        self.children = children

    def serialise(self) -> bytes:
        child_config_format = "c" * 65
        pack_str = "<BBBB" + child_config_format * 64
        child_bytes = bytearray()
        for child in self.children:
            c_name = child[1].encode("utf-8")
            c_name_padded = c_name.ljust(64, b"\0")
            assert len(c_name_padded) == 64
            child_bytes.extend(c_name_padded)
            child_bytes.extend(child[0].to_bytes(1, "little"))

        child_bytes = child_bytes.ljust(64 * 65, b"\0")

        child_bytes_list = [x.to_bytes(1, "little") for x in child_bytes]

        return struct.pack(
            pack_str,
            self.ch_start,
            self.ch_stop,
            self.ch_init,
            len(self.children),
            *child_bytes_list,
        )


def generate(sdf_file: str, output_dir: str, dtb: DeviceTree, driver_vm_dtb: DeviceTree):
    uart_node = dtb.node(board.serial)
    assert uart_node is not None
    guest_serial_node = driver_vm_dtb.node(board.guest_serial)
    assert guest_serial_node is not None
    ethernet_node = dtb.node(board.ethernet)
    assert ethernet_node is not None
    timer_node = dtb.node(board.timer)
    assert timer_node is not None

    timer_driver = ProtectionDomain("timer_driver", "timer_driver.elf", priority=110)
    timer_system = Sddf.Timer(sdf, timer_node, timer_driver)

    uart_driver = ProtectionDomain("serial_driver", "serial_driver.elf", priority=109)
    serial_virt_tx = ProtectionDomain(
        "serial_virt_tx", "serial_virt_tx.elf", priority=108
    )
    serial_virt_rx = ProtectionDomain("serial_virt_rx", "serial_virt_rx.elf",
                                      priority=107, stack_size=0x2000)
    serial_system = Sddf.Serial(sdf, uart_node, uart_driver, serial_virt_tx, virt_rx=serial_virt_rx)

    eth_driver_vmm_pd = ProtectionDomain("net_driver_vm", "eth_driver_vmm.elf", priority=10)
    eth_driver_vm = VirtualMachine("linux", [VirtualMachine.Vcpu(id=0)])
    # one_to_one_ram=True is very important as the network drivers in Linux uses DMA!
    eth_driver_vmm = Vmm(sdf, eth_driver_vmm_pd, eth_driver_vm, driver_vm_dtb, one_to_one_ram=True)

    eth_driver_vmm.add_virtio_mmio_console(guest_serial_node, serial_system)

    for device_path in board.passthrough:
        node = dtb.node(device_path)
        assert node is not None
        eth_driver_vmm.add_passthrough_device(node)

    net_virt_tx = ProtectionDomain(
        "net_virt_tx", "network_virt_tx.elf", priority=100, budget=20000
    )
    net_virt_rx = ProtectionDomain("net_virt_rx", "network_virt_rx.elf", priority=99)
    net_system = Sddf.Net(sdf, ethernet_node, eth_driver_vmm_pd, net_virt_tx, net_virt_rx, driver_vm_system=eth_driver_vmm)

    client0 = ProtectionDomain("client0", "echo0.elf", priority=97, budget=20000)
    client0_net_copier = ProtectionDomain(
        "client0_net_copier", "network_copy0.elf", priority=98, budget=20000
    )
    client1 = ProtectionDomain("client1", "echo1.elf", priority=97, budget=20000)
    client1_net_copier = ProtectionDomain(
        "client1_net_copier", "network_copy1.elf", priority=98, budget=20000
    )

    serial_system.add_client(client0)
    serial_system.add_client(client1)
    timer_system.add_client(client0)
    timer_system.add_client(client1)
    net_system.add_client_with_copier(client0, client0_net_copier)
    net_system.add_client_with_copier(client1, client1_net_copier)

    client0_lib_sddf_lwip = Sddf.Lwip(sdf, net_system, client0)
    client1_lib_sddf_lwip = Sddf.Lwip(sdf, net_system, client1)

    # Benchmark specific resources

    bench_idle = ProtectionDomain("bench_idle", "idle.elf", priority=1)
    bench = ProtectionDomain("bench", "benchmark.elf", priority=253)

    serial_system.add_client(bench)

    benchmark_pds = [
        uart_driver,
        serial_virt_tx,
        serial_virt_rx,
        eth_driver_vmm_pd,
        net_virt_tx,
        net_virt_rx,
        client0,
        client0_net_copier,
        client1,
        client1_net_copier,
        timer_driver,
    ]
    pds = [
        bench_idle,
        bench,
    ]
    bench_children = []
    for pd in benchmark_pds:
        child_id = bench.add_child_pd(pd)
        bench_children.append((child_id, pd.name))
    for pd in pds:
        sdf.add_pd(pd)

    # Benchmark START channel
    bench_start_ch = Channel(client0, bench)
    sdf.add_channel(bench_start_ch)
    # Benchmark STOP channel
    bench_stop_ch = Channel(client0, bench)
    sdf.add_channel(bench_stop_ch)

    bench_idle_ch = Channel(bench_idle, bench)
    sdf.add_channel(bench_idle_ch)

    cycle_counters_mr = MemoryRegion(sdf, "cycle_counters", 0x1000)
    sdf.add_mr(cycle_counters_mr)

    bench_idle.add_map(Map(cycle_counters_mr, 0x5_000_000, perms="rw"))
    client0.add_map(Map(cycle_counters_mr, 0x20_000_000, perms="rw"))
    bench_idle_config = BenchmarkIdleConfig(0x5_000_000, bench_idle_ch.pd_a_id)

    bench_client_config = BenchmarkClientConfig(
        0x20_000_000, bench_start_ch.pd_a_id, bench_stop_ch.pd_a_id
    )

    benchmark_config = BenchmarkConfig(
        bench_start_ch.pd_b_id,
        bench_stop_ch.pd_b_id,
        bench_idle_ch.pd_b_id,
        bench_children,
    )

    assert eth_driver_vmm.connect()
    assert serial_system.connect()
    assert net_system.connect()
    assert timer_system.connect()
    assert client0_lib_sddf_lwip.connect()
    assert client1_lib_sddf_lwip.connect()
    assert eth_driver_vmm.serialise_config(output_dir)
    assert serial_system.serialise_config(output_dir)
    assert net_system.serialise_config(output_dir)
    assert timer_system.serialise_config(output_dir)
    assert client0_lib_sddf_lwip.serialise_config(output_dir)
    assert client1_lib_sddf_lwip.serialise_config(output_dir)

    with open(f"{output_dir}/benchmark_config.data", "wb+") as f:
        f.write(benchmark_config.serialise())

    with open(f"{output_dir}/benchmark_idle_config.data", "wb+") as f:
        f.write(bench_idle_config.serialise())

    with open(f"{output_dir}/benchmark_client_config.data", "wb+") as f:
        f.write(bench_client_config.serialise())

    with open(f"{output_dir}/{sdf_file}", "w+") as f:
        f.write(sdf.render())


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--dtb_native", required=True)
    parser.add_argument("--dtb_guest", required=True)
    parser.add_argument("--sddf", required=True)
    parser.add_argument("--board", required=True, choices=[b.name for b in BOARDS])
    parser.add_argument("--output", required=True)
    parser.add_argument("--sdf", required=True)

    args = parser.parse_args()

    board = next(filter(lambda b: b.name == args.board, BOARDS))

    sdf = SystemDescription(board.arch, board.paddr_top)
    sddf = Sddf(args.sddf)

    with open(args.dtb_native, "rb") as f:
        dtb_native = DeviceTree(f.read())

    with open(args.dtb_guest, "rb") as f:
        dtb_guest = DeviceTree(f.read())

    generate(args.sdf, args.output, dtb_native, dtb_guest)
