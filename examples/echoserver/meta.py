# Copyright 2025, UNSW
# SPDX-License-Identifier: BSD-2-Clause
import argparse
import struct
import random
from dataclasses import dataclass
from typing import List, Tuple
from sdfgen import SystemDescription, Sddf, DeviceTree, Vmm

ProtectionDomain = SystemDescription.ProtectionDomain
VirtualMachine = SystemDescription.VirtualMachine
MemoryRegion = SystemDescription.MemoryRegion
Map = SystemDescription.Map
Channel = SystemDescription.Channel
Irq = SystemDescription.Irq


@dataclass
class Board:
    name: str
    arch: SystemDescription.Arch
    paddr_top: int
    serial: str
    timer: str
    net: str
    guest_net: str
    passthrough: str

BOARDS: List[Board] = [
    Board(
        name="qemu_virt_aarch64",
        arch=SystemDescription.Arch.AARCH64,
        paddr_top=0x60000000,
        serial="pl011@9000000",
        net="virtio_mmio@a003e00",
        timer="timer",
        guest_net="virtio-net@0160000",
        passthrough=[],
    ),
    Board(
        name="odroidc4",
        arch=SystemDescription.Arch.AARCH64,
        paddr_top=0x60000000,
        serial="soc/bus@ff800000/serial@3000",
        net="soc/ethernet@ff3f0000",
        timer="soc/bus@ffd00000/watchdog@f0d0",
        guest_net="virtio-net@0160000",
        passthrough=[
            "soc/bus@ff600000",
            "soc/bus@ffd00000/reset-controller@1004",
            "soc/bus@ffd00000/interrupt-controller@f080",
            "soc/bus@ffd00000/interrupt-controller@f080",
            "soc/bus@ff800000/sys-ctrl@0",
            "soc/ethernet@ff3f0000",
        ],
    ),
    Board(
        name="maaxboard",
        arch=SystemDescription.Arch.AARCH64,
        paddr_top=0x90000000,
        serial="soc@0/bus@30800000/serial@30860000",
        timer="soc@0/bus@30000000/timer@302d0000",
        net="soc@0/bus@30800000/ethernet@30be0000",
        guest_net="virtio-net@0160000",
        passthrough=[
            "soc@0/bus@30000000/syscon@30360000",
            "soc@0/bus@30000000/gpc@303a0000",
            "soc@0/bus@32c00000/interrupt-controller@32e2d000",
            "soc@0/bus@30000000/ocotp-ctrl@30350000",
            "soc@0/bus@30000000/iomuxc@30330000",
            "soc@0/bus@30000000/gpio@30200000",
            "soc@0/bus@30000000/gpio@30210000",
            "soc@0/bus@30000000/gpio@30220000",
            "soc@0/bus@30000000/gpio@30230000",
            "soc@0/bus@30000000/gpio@30240000",
            "soc@0/bus@30800000/ethernet@30be0000",
            "soc@0/bus@30400000/timer@306a0000",
            "soc@0/bus@30000000/clock-controller@30380000",
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

    '''
        Matches struct definition:
        {
            void *;
            uint8_t;
        }
    '''
    def serialise(self) -> bytes:
        return struct.pack("<qc", self.cycle_counters, self.ch_init.to_bytes(1, "little"))


class BenchmarkClientConfig:
    def __init__(self, cycle_counters: int, ch_start: int, ch_stop: int):
        self.cycle_counters = cycle_counters
        self.ch_start = ch_start
        self.ch_stop = ch_stop

    '''
        Matches struct definition:
        {
            void *;
            uint8_t;
            uint8_t;
        }
    '''
    def serialise(self) -> bytes:
        return struct.pack("<qBB", self.cycle_counters, self.ch_start, self.ch_stop)


class BenchmarkConfig:
    def __init__(self, ch_start: int, ch_stop: int, ch_init: int, children: List[Tuple[int, str]]):
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
            c_name_padded = c_name.ljust(64, b'\0')
            assert len(c_name_padded) == 64
            child_bytes.extend(c_name_padded)
            child_bytes.extend(child[0].to_bytes(1, "little"))

        child_bytes = child_bytes.ljust(64 * 65, b'\0')

        child_bytes_list = [x.to_bytes(1, "little") for x in child_bytes]

        return struct.pack(
            pack_str, self.ch_start, self.ch_stop,
            self.ch_init, len(self.children), *child_bytes_list
        )


def generate(sdf_file: str, output_dir: str, dtb: DeviceTree, client_dtb: DeviceTree):
    uart_node = dtb.node(board.serial)
    assert uart_node is not None
    ethernet_node = dtb.node(board.net)
    assert ethernet_node is not None
    timer_node = dtb.node(board.timer)
    assert uart_node is not None

    # Benchmark specific resources
    bench_idle = ProtectionDomain("bench_idle", "idle.elf", priority=1)
    bench = ProtectionDomain("bench", "benchmark.elf", priority=254)

    timer_driver = ProtectionDomain("timer_driver", "timer_driver.elf", priority=101)
    timer_system = Sddf.Timer(sdf, timer_node, timer_driver)

    uart_driver = ProtectionDomain("uart_driver", "uart_driver.elf", priority=100)
    serial_virt_tx = ProtectionDomain("serial_virt_tx", "serial_virt_tx.elf", priority=99)
    serial_virt_rx = ProtectionDomain("serial_virt_rx", "serial_virt_rx.elf",
                                      priority=199, stack_size=0x2000)
    serial_system = Sddf.Serial(sdf, uart_node, uart_driver, serial_virt_tx, virt_rx=serial_virt_rx)

    ethernet_driver = ProtectionDomain(
        "ethernet_driver", "eth_driver.elf", priority=101, budget=100, period=400
    )
    net_virt_tx = ProtectionDomain("net_virt_tx", "network_virt_tx.elf", priority=100, budget=20000)
    net_virt_rx = ProtectionDomain("net_virt_rx", "network_virt_rx.elf", priority=99)
    net_system = Sddf.Net(sdf, ethernet_node, ethernet_driver, net_virt_tx, net_virt_rx)

    # Client 1
    client0 = ProtectionDomain("client0", "client_vmm0.elf", priority=97, budget=20000)
    vm_client0 = VirtualMachine("client_linux-0", [VirtualMachine.Vcpu(id=0)], priority=96)
    vmm_client0 = Vmm(sdf, client0, vm_client0, client_dtb, one_to_one_ram=True)

    # client0_net_copier = ProtectionDomain(
        # "client0_net_copier", "network_copy0.elf", priority=98, budget=20000
    # )

    # Client 2
    # client1 = ProtectionDomain("client1", "client_vmm1.elf", priority=97, budget=20000)
    # vm_client1 = VirtualMachine("client_linux-1", [VirtualMachine.Vcpu(id=0)])
    # vmm_client1 = Vmm(sdf, client1, vm_client1, client_dtb)
    for device_path in board.passthrough:
        node = dtb.node(device_path)
        assert node is not None
        vmm_client0.add_passthrough_device(node, regions=[0])
        # vmm_client1.add_passthrough_device(node)

    if board.name == "odroidc4":
        eth_phy_irq = Irq(96)
        vmm_client0.add_passthrough_irq(eth_phy_irq)
        irq_work_irq = Irq(5)
        vmm_client0.add_passthrough_irq(irq_work_irq)

    # client1_net_copier = ProtectionDomain(
    #     "client1_net_copier", "network_copy1.elf", priority=98, budget=20000
    # )

    # mac_random_part = random.randint(0, 0xfe)
    # client0_mac_addr = f"52:54:01:00:00:{hex(mac_random_part)[2:]:0>2}"
    # client1_mac_addr = f"52:54:01:00:00:{hex(mac_random_part + 1)[2:]:0>2}"
    # assert client0_mac_addr != client1_mac_addr

    # vmm_client0.add_virtio_mmio_net(client_dtb.node(board.guest_net), net_system, client0_net_copier, mac_addr=client0_mac_addr)
    # vmm_client1.add_virtio_mmio_net(client_dtb.node(board.guest_net), net_system, client1_net_copier, mac_addr=client1_mac_addr)

    serial_system.add_client(client0)
    # serial_system.add_client(client1)
    timer_system.add_client(client0)
    # timer_system.add_client(client1)
    serial_system.add_client(bench)

    benchmark_pds = [
        uart_driver,
        serial_virt_tx,
        serial_virt_rx,
        # ethernet_driver,
        # net_virt_tx,
        # net_virt_rx,
        client0,
        # client1,
        # client0_net_copier,
        # client1_net_copier,
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

    assert vmm_client0.connect()
    assert vmm_client0.serialise_config(output_dir)
    # assert vmm_client1.connect()
    # assert vmm_client1.serialise_config(output_dir)

    # Benchmark START channel
    bench_start_ch = Channel(client0, bench)
    sdf.add_channel(bench_start_ch)
    # Benchmark STOP channel
    bench_stop_ch = Channel(client0, bench)
    sdf.add_channel(bench_stop_ch)

    bench_idle_ch = Channel(bench_idle, bench)
    sdf.add_channel(bench_idle_ch)

    cycle_counters_mr = MemoryRegion("cycle_counters", 0x1000)
    sdf.add_mr(cycle_counters_mr)

    bench_idle.add_map(Map(cycle_counters_mr, 0x5_000_000, perms=Map.Perms(r=True, w=True)))
    client0.add_map(Map(cycle_counters_mr, 0x30_000_000, perms=Map.Perms(r=True, w=True)))
    bench_idle_config = BenchmarkIdleConfig(0x5_000_000, bench_idle_ch.pd_a_id)

    bench_client_config = BenchmarkClientConfig(
        0x20_000_000,
        bench_start_ch.pd_a_id,
        bench_stop_ch.pd_a_id
    )

    benchmark_config = BenchmarkConfig(
        bench_start_ch.pd_b_id,
        bench_stop_ch.pd_b_id,
        bench_idle_ch.pd_b_id,
        bench_children
    )

    assert serial_system.connect()
    assert serial_system.serialise_config(output_dir)
    # assert net_system.connect()
    # assert net_system.serialise_config(output_dir)
    assert timer_system.connect()
    assert timer_system.serialise_config(output_dir)

    with open(f"{output_dir}/benchmark_config.data", "wb+") as f:
        f.write(benchmark_config.serialise())

    with open(f"{output_dir}/benchmark_idle_config.data", "wb+") as f:
        f.write(bench_idle_config.serialise())

    with open(f"{output_dir}/benchmark_client_config.data", "wb+") as f:
        f.write(bench_client_config.serialise())

    with open(f"{output_dir}/{sdf_file}", "w+") as f:
        f.write(sdf.render())


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument("--dtb", required=True)
    parser.add_argument("--client_dtb", required=True)
    parser.add_argument("--sddf", required=True)
    parser.add_argument("--board", required=True, choices=[b.name for b in BOARDS])
    parser.add_argument("--output", required=True)
    parser.add_argument("--sdf", required=True)

    args = parser.parse_args()

    board = next(filter(lambda b: b.name == args.board, BOARDS))

    sdf = SystemDescription(board.arch, board.paddr_top)
    sddf = Sddf(args.sddf)

    with open(args.dtb, "rb") as f:
        dtb = DeviceTree(f.read())

    with open(args.client_dtb, "rb") as f:
        client_dtb = DeviceTree(f.read())

    generate(args.sdf, args.output, dtb, client_dtb)
