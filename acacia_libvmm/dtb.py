# Copyright 2026, UNSW
# SPDX-License-Identifier: BSD-2-Clause
"""
The device tree queries libvmm needs, on top of acacia.DeviceTreeBlob.
"""

from __future__ import annotations

from typing import List, Optional, Tuple

from acacia import IRQ, Arch, DeviceTreeBlob, DTBNode

# (address, size)
Range = Tuple[int, int]


class LibvmmDtbReader:
    """
    Read-only view of a DTB with useful helpers for libvmm. Addresses come back
    as CPU physical addresses (translated through any bus 'ranges'), and missing
    properties come back as empty rather than raising an error.
    """

    def __init__(self, dtb: DeviceTreeBlob, arch: Arch) -> None:
        self.dtb = dtb
        self.arch = arch

    def node(self, node_path: str) -> DTBNode:
        return self.dtb.get_node_by_path(node_path)

    def regs(self, node: DTBNode) -> List[Range]:
        if self.dtb.get_node_prop(node, "reg") is None:
            return []
        # get_reg_paddr() rounds down to a page, so add the in-page offset back.
        page_offset_mask = self.arch.default_page_size_bytes() - 1
        return [
            (
                self.dtb.get_reg_paddr(self.arch, node, addr)
                + (addr & page_offset_mask),
                size,
            )
            for addr, size in self.dtb.get_node_regs(node)
        ]

    def irqs(self, node: DTBNode) -> List[IRQ]:
        # get_parsed_irqs() crashes on nodes without 'interrupts', so check first.
        if self.dtb.get_node_prop(node, "interrupts") is None:
            return []
        return list(self.dtb.get_parsed_irqs(node, self.arch))

    def memory(self) -> List[Range]:
        """Every range of every device_type = "memory" node, lowest first."""
        ranges: List[Range] = []
        for node in self.dtb.nodes.values():
            device_type = self.dtb.get_node_prop(node, "device_type")
            if (
                device_type is not None
                and bytes(device_type).rstrip(b"\0") == b"memory"
            ):
                ranges.extend(self.regs(node))
        return sorted(ranges)

    def initrd(self) -> Optional[Range]:
        """The initrd that /chosen points Linux at, or None if there isn't one."""
        chosen = next((n for n in self.dtb.nodes.values() if n.path == "/chosen"), None)
        if chosen is None:
            return None

        start = self._int_prop(chosen, "linux,initrd-start")
        end = self._int_prop(chosen, "linux,initrd-end")
        if start is None and end is None:
            return None
        if start is None or end is None:
            raise ValueError(
                "/chosen has only one of linux,initrd-start and linux,initrd-end"
            )
        if end <= start:
            raise ValueError(f"initrd end {end:#x} is not after its start {start:#x}")
        return (start, end - start)

    def _int_prop(self, node: DTBNode, name: str) -> Optional[int]:
        raw = self.dtb.get_node_prop(node, name)
        return None if raw is None else int.from_bytes(bytes(raw), "big")
