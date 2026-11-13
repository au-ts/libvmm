# Copyright 2026, UNSW
# SPDX-License-Identifier: BSD-2-Clause
"""
Mirror of ../include/libvmm/config.h.

See the config struct documentation of Acacia for the operation of this file.
"""

from __future__ import annotations

from dataclasses import dataclass, field
from enum import IntEnum
from typing import List, Optional

from acacia import IRQ, ConfigStruct, IOPort, Map, MemoryRegion

VMM_MAGIC = "vmm" + chr(0x1)

VMM_MAX_RAM_REGIONS = 3  # matches GUEST_MAX_RAM_REGIONS in include/libvmm/guest_ram.h
VMM_MAX_IRQS = 32
VMM_MAX_VCPUS = 32
VMM_MAX_DT_DEVICES = 32
VMM_MAX_IOPORTS = 16
VMM_DT_DEV_NAME_MAX_LEN = 32  # Including the NUL terminator


class GuestMemoryKind(IntEnum):
    """vmm_config_ram_kind_t"""

    RAM = 0
    FLASH = 1


@dataclass
class GuestMemoryRegion:
    """vmm_config_ram_region_t: guest memory, and where the VMM sees it."""

    kind: GuestMemoryKind
    gpa: int
    # The size the guest is told about. The MR, and so its mappings, can be
    # larger because Acacia rounds big MRs up to whole large pages.
    size: int
    mr: MemoryRegion
    vmm_map: Map

    @property
    def end(self) -> int:
        return self.gpa + self.size

    @property
    def vmm_vaddr(self) -> int:
        return self.vmm_map.vaddr

    def to_config(self) -> ConfigStruct:
        return ConfigStruct(
            {
                "gpa": self.gpa,
                "size": self.size,
                "vmm_vaddr": self.vmm_vaddr,
                "kind": int(self.kind),
            },
            "vmm_config_ram_region_t",
        )


@dataclass
class PassthroughIrq:
    """vmm_config_irq_t: a host IRQ the VMM receives and injects into the guest."""

    irq: IRQ
    # What the guest sees: a GIC INTID on ARM, an I/O APIC pin on x86.
    guest_irq: int

    def to_config(self) -> ConfigStruct:
        return ConfigStruct(
            {"id": self.irq.id, "irq": self.guest_irq}, "vmm_config_irq_t"
        )


@dataclass
class GuestDtDevice:
    """vmm_config_guest_dt_device_t: a device described in the guest's device tree
    that require handling from the VMM. For example, virtIO MMIO emulation or Linux UIO
    for guest<->VMM communication."""

    name: str
    gpa_base: int
    size: int
    vmm_vaddr: int
    irq: int

    @property
    def gpa_end(self) -> int:
        return self.gpa_base + self.size

    def to_config(self) -> ConfigStruct:
        return ConfigStruct(
            {
                "name": self.name,
                "gpa_base": self.gpa_base,
                "size": self.size,
                "vmm_vaddr": self.vmm_vaddr,
                "irq": self.irq,
            },
            "vmm_config_guest_dt_device_t",
        )


def _ioport_to_config(ioport: IOPort) -> ConfigStruct:
    return ConfigStruct(
        {"id": ioport.id, "addr": ioport.addr, "size": ioport.size},
        "vmm_config_ioport_t",
    )


@dataclass
class VmmConfig:
    ram_regions: List[GuestMemoryRegion] = field(default_factory=list)
    vcpu_ids: List[int] = field(default_factory=list)
    irqs: List[PassthroughIrq] = field(default_factory=list)

    # DTB guests only.
    dtb: int = 0
    initrd: int = 0
    guest_dt_devices: List[GuestDtDevice] = field(default_factory=list)

    # x86 guests only.
    ioports: List[IOPort] = field(default_factory=list)

    def to_config_struct(self, target_file: str) -> ConfigStruct:
        fields = {
            "magic": VMM_MAGIC,
            "num_ram_regions": len(self.ram_regions),
            "ram_regions": [r.to_config() for r in self.ram_regions],
            "dtb_gpa": self.dtb,
            "initrd_gpa": self.initrd,
            "num_dt_devices": len(self.guest_dt_devices),
            "dt_devices": [dt_dev.to_config() for dt_dev in self.guest_dt_devices],
            "num_irqs": len(self.irqs),
            "irqs": [i.to_config() for i in self.irqs],
            "num_vcpus": len(self.vcpu_ids),
            "vcpus": [
                ConfigStruct({"id": i}, "vmm_config_vcpu_t") for i in self.vcpu_ids
            ],
            "num_ioports": len(self.ioports),
            "ioports": [_ioport_to_config(io) for io in self.ioports],
        }
        return ConfigStruct(
            fields,
            type_name="vmm_config_t",
            section_name="vmm_config",
            target_file=target_file,
        )
