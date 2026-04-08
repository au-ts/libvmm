# inject_bench_config.py
import struct
import argparse
import subprocess
import os

class BenchmarkIdleConfig:
    def __init__(self, cycle_counters_addr: int, ch_init: int):
        self.cycle_counters = cycle_counters_addr
        self.ch_init = ch_init

    def serialise(self) -> bytes:
        # struct { void *; uint8_t; } mapped to 64-bit bounds
        return struct.pack("<QB", self.cycle_counters, self.ch_init)

class BenchmarkConfig:
    def __init__(self, ch_rx_start, ch_rx_stop, ch_init, core, last_core, children=[]):
        self.ch_rx_start = ch_rx_start
        self.ch_tx_start = 0       # Not cascading cores
        self.ch_rx_stop = ch_rx_stop
        self.ch_tx_stop = 0        # Not cascading cores
        self.ch_init = ch_init
        self.core = core
        self.last_core = last_core
        self.children = children

    def serialise(self) -> bytes:
        data = struct.pack("<BBBBBB?B",
        self.ch_rx_start, self.ch_tx_start,
        self.ch_rx_stop,  self.ch_tx_stop,
        self.ch_init, self.core, self.last_core,
        len(self.children))
    
        for (child_id, name) in self.children:
            c_name = name.encode("utf-8")[:63].ljust(64, b"\0")
            data += c_name + child_id.to_bytes(1, "little")
        
        # Pad remaining child slots to 64 total
        remaining = 64 - len(self.children)
        data += b"\0" * (65 * remaining)
        return data 

def update_elf_section(objcopy, elf_name, section_name, data_bytes):
    assert os.path.isfile(elf_name), f"Missing {elf_name}"
    data_file = f"{section_name}.data"
    with open(data_file, "wb") as f:
        f.write(data_bytes)
    
    subprocess.run([
        objcopy, "--update-section", f".{section_name}={data_file}", elf_name
    ], check=True)
    os.remove(data_file)

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--objcopy", required=True)
    args = parser.parse_args()

    # Create the configs based purely on your simple.system layout bounds
    # bench_idle maps cycle_counters0 at 0x5000000, talks to bench on ch 0
    idle_config = BenchmarkIdleConfig(cycle_counters_addr=0x5000000, ch_init=0)
    
    pds_to_trace = [
        (1, "VMM")
    ]

    bench_config = BenchmarkConfig(
        ch_rx_start=2, 
        ch_rx_stop=3, 
        ch_init=0, 
        core=0, 
        last_core=True, 
        children=pds_to_trace
    )

    # Inject into the pre-compiled elfs before Microkit runs
    update_elf_section(args.objcopy, "idle.elf", "benchmark_config", idle_config.serialise())
    update_elf_section(args.objcopy, "benchmark.elf", "benchmark_config", bench_config.serialise())
    print("Successfully patched benchmark capabilities into ELFs")