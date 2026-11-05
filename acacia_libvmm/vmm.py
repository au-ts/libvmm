# Copyright 2026, UNSW
# SPDX-License-Identifier: BSD-2-Clause
"""
Acacia subsystem for a libvmm VMM and its guest.

The reason why it is represented as a subsystem rather than just a normal
class is to leverage Acacia's config struct machineries that are only available
to a "Subsystem".

One class covers both families of guest:

* DTB guests (ARM, RISC-V): everything the DTB describes is created in __init__.
  That is guest RAM, the GIC vCPU interface (ARM), initrd and DTB placement and
  UIO regions. You only add passthrough and virtIO devices on top.
* x86 guests: the user describe the guest with the same
  primitives the DTB path uses internally: add_ram / add_x86_ram /
  add_x86_flash, add_passthrough_mmio, add_passthrough_irq and
  add_passthrough_ioport.

Everything get encoded in a vmm_config_t for the C side.
"""

from __future__ import annotations

from dataclasses import dataclass, field
from enum import IntEnum
from typing import TYPE_CHECKING, List, Optional, Sequence, Union

from acacia import (
    IRQ,
    ConfigStruct,
    DeviceTreeBlob,
    DTBNode,
    IOPort,
    IrqIoapic,
    Map,
    MemoryRegion,
    ProtectionDomain,
    Subsystem,
    SubsystemBuildError,
    System,
    VirtualMachine,
    build_hook,
)
from acacia.dtb import Arm_GIC

if TYPE_CHECKING:
    from acacia_sddf import sDDFBlk, sDDFEthernet, sDDFSerial

# Must match include/libvmm/config.h
VMM_MAGIC = "vmm"
VMM_MAX_RAM_REGIONS = 3  # == GUEST_MAX_RAM_REGIONS
VMM_MAX_IRQS = 32
VMM_MAX_VCPUS = 32
VMM_MAX_VIRTIO_MMIO_DEVICES = 32
VMM_MAX_IOPORTS = 16
VMM_MAX_UIOS = 16
VMM_UIO_NAME_MAX_LEN = 32

# Must match include/libvmm/arch/x86_64/memory_space.h
X86_LOW_RAM_START_GPA = 0x0
X86_ECAM_GPA = 0xD000_0000
X86_HIGH_RAM_START_GPA = 0x1_0000_0000
# Low RAM must end before the PCI MMIO window your DSDT advertises.
X86_DEFAULT_LOW_RAM_LIMIT = 0x8000_0000
X86_LOW_RAM_LIMIT = X86_ECAM_GPA

LINUX_UIO_COMPATIBLE = "generic-uio"


class VirtioDeviceType(IntEnum):
    NET = 1
    BLK = 2
    CONSOLE = 3
    SOUND = 25


class GuestMemoryKind(IntEnum):
    RAM = 0
    FLASH = 1


@dataclass
class GuestMemoryRegion:
    kind: GuestMemoryKind
    gpa: int
    # Size the guest is told about. The MR may be larger due to Acacia's
    # large page rounding.
    size: int
    mr: MemoryRegion
    vmm_map: Map

    @property
    def end(self) -> int:
        return self.gpa + self.size

    @property
    def vmm_vaddr(self) -> int:
        return self.vmm_map.vaddr


@dataclass
class PassthroughIrq:
    irq: IRQ
    # GIC INTID on ARM, guest I/O APIC pin on x86.
    guest_irq: int


@dataclass
class VirtioMmioDevice:
    type: VirtioDeviceType
    base: int
    size: int
    irq: int


@dataclass
class UioRegion:
    name: str
    guest_paddr: int
    size: int
    irq: Optional[int]
    vmm_vaddr: int = 0


@dataclass
class PassthroughResources:
    maps: List[Map] = field(default_factory=list)
    irq_ids: List[int] = field(default_factory=list)


def _overlaps(a_start: int, a_end: int, b_start: int, b_end: int) -> bool:
    return a_start < b_end and b_start < a_end


class LibvmmVirtMachine(Subsystem):
    def __init__(
        self,
        sdf: System,
        vmm: ProtectionDomain,
        guest_dtb: Optional[DeviceTreeBlob] = None,
        *,
        guest_dtb_size: Optional[int] = None,
        one_to_one_ram: bool = False,
    ) -> None:
        """
        :param vmm: VMM PD. Must already own the guest via `vmm.add_vm()`.
        :param guest_dtb: Guest DTB. Required on ARM/RISC-V, must be None on x86.
        :param guest_dtb_size: Size of the DTB the VMM will load. Defaults to the
            size of `guest_dtb`.
        :param one_to_one_ram: DTB guests only. Back guest RAM with physical
            memory at the same address as the GPA.
        """
        if vmm.vm is None:
            raise ValueError(f"VMM PD '{vmm.name}' does not contain a virtual machine!")

        if sdf.arch.is_x86():
            if guest_dtb is not None:
                raise ValueError("x86 guests are described by ACPI, not a DTB")
            if one_to_one_ram:
                raise ValueError("one_to_one_ram cannot be used on x86, please use the IOMMU")
        elif guest_dtb is None:
            raise ValueError("Guest VM's DTB must be provided for ARM or RISC-V targets")

        if len(vmm.vm.vcpus) > VMM_MAX_VCPUS:
            raise ValueError(f"libvmm supports at most {VMM_MAX_VCPUS} vCPUs")

        super().__init__(sdf, f"libvmm_{vmm.name}", clients_allowed=False)

        self.vmm = vmm
        self.vm = vmm.vm
        self.guest_dtb = guest_dtb
        self.one_to_one_ram = one_to_one_ram

        self.memory: List[GuestMemoryRegion] = []
        self.irqs: List[PassthroughIrq] = []
        self.ioports: List[IOPort] = []
        self.virtio_mmio_devices: List[VirtioMmioDevice] = []
        self.uios: List[UioRegion] = []
        self.gic_vcpu_mr: Optional[MemoryRegion] = None

        # Only meaningful for DTB guests, 0 otherwise.
        self.dtb_gpa = 0
        self.initrd_gpa = 0

        if guest_dtb is not None:
            if guest_dtb_size is None:
                guest_dtb_size = guest_dtb.fdt.totalsize()
            self._configure_from_dtb(guest_dtb, guest_dtb_size)


    def add_ram(
        self,
        gpa: int,
        size: int,
        *,
        kind: GuestMemoryKind = GuestMemoryKind.RAM,
        name: Optional[str] = None,
        paddr: Optional[int] = None,
        vmm_vaddr: Optional[int] = None,
    ) -> GuestMemoryRegion:
        """
        Add a region of guest memory, mapped rwx into the guest at `gpa` and rw
        into the VMM. The VMM vaddr is auto-allocated unless `vmm_vaddr` is given.
        `paddr` backs the region with specific physical memory.
        """
        arch = self.sdf.arch
        if not arch.addr_is_page_aligned(gpa) or not arch.addr_is_page_aligned(size):
            raise ValueError(f"Guest memory [{gpa:#x}, +{size:#x}) is not page aligned")
        if len(self.memory) >= VMM_MAX_RAM_REGIONS:
            raise ValueError(f"libvmm supports at most {VMM_MAX_RAM_REGIONS} guest memory regions")
        for r in self.memory:
            if _overlaps(gpa, gpa + size, r.gpa, r.end):
                raise ValueError(
                    f"Guest memory [{gpa:#x}, {gpa + size:#x}) overlaps "
                    f"existing region [{r.gpa:#x}, {r.end:#x})"
                )

        if name is None:
            kind_name = "ram" if kind == GuestMemoryKind.RAM else "flash"
            n = sum(1 for r in self.memory if r.kind == kind)
            name = f"guest_{self.vm.name}_{kind_name}" + (f"_{n}" if n else "")

        mr = MemoryRegion(self.sdf, name, size, paddr=paddr)
        self.vm.add_map(Map(mr, gpa, "rwx"))
        if vmm_vaddr is None:
            vmm_map = self.vmm.create_automap(mr, "rw")
        else:
            vmm_map = Map(mr, vmm_vaddr, "rw")
            self.vmm.add_map(vmm_map)

        region = GuestMemoryRegion(kind, gpa, size, mr, vmm_map)
        self.memory.append(region)
        return region


    def ram_region_containing(self, gpa: int, size: int = 1) -> Optional[GuestMemoryRegion]:
        for r in self.memory:
            if r.gpa <= gpa and gpa + size <= r.end:
                return r
        return None


    def add_x86_ram(
        self, size: int, low_ram_limit: int = X86_DEFAULT_LOW_RAM_LIMIT
    ) -> List[GuestMemoryRegion]:
        """
        Add `size` bytes of guest RAM using the PC layout: up to `low_ram_limit`
        bytes from GPA 0, the rest from 4GiB. `low_ram_limit` must not reach the
        PCI MMIO window your DSDT advertises.
        """
        self._require_x86("add_x86_ram")
        if low_ram_limit > X86_LOW_RAM_LIMIT_MAX:
            raise ValueError(
                f"Low RAM limit {low_ram_limit:#x} runs into ECAM at {X86_ECAM_GPA:#x}"
            )
        low = min(size, low_ram_limit)
        regions = [
            self.add_ram(X86_LOW_RAM_START_GPA, low, name=f"guest_ram_low_{self.guest.name}")
        ]
        if size > low:
            regions.append(
                self.add_ram(
                    X86_HIGH_RAM_START_GPA,
                    size - low,
                    name=f"guest_ram_high_{self.guest.name}",
                )
            )
        return regions

    def add_x86_flash(self, size: int) -> GuestMemoryRegion:
        """Add firmware flash (e.g. OVMF) ending at the 4GiB boundary."""
        self._require_x86("add_x86_flash")
        return self.add_ram(X86_HIGH_RAM_START_GPA - size, size, kind=GuestMemoryKind.FLASH)

    # ------------------------------------------------------------------
    # Passthrough primitives (all architectures)
    # ------------------------------------------------------------------

    def add_passthrough_mmio(
        self,
        paddr: int,
        size: int,
        *,
        gpa: Optional[int] = None,
        name: Optional[str] = None,
    ) -> Map:
        """
        Map device MMIO at `paddr` into the guest, uncached. The range is
        widened to whole pages. `gpa` defaults to the host physical address.
        An existing MR at the same paddr is reused, which handles several
        devices sharing a page.
        """
        arch = self.sdf.arch
        base = arch.rounddown_to_page(paddr)
        size = arch.roundup_to_page(paddr + size) - base
        if gpa is None:
            gpa = base
        elif not arch.addr_is_page_aligned(gpa):
            raise ValueError(f"Passthrough GPA {gpa:#x} is not page aligned")

        for g_mem in self.memory:
            if _overlaps(gpa, gpa + size, g_mem.gpa, g_mem.end):
                raise ValueError(f"Passthrough MMIO at GPA {gpa:#x} overlaps guest memory")

        # @billn sus need to check the actual range not just hte base
        mr = next((m for m in self.sdf.mrs if m.paddr == base), None)
        if mr is not None:
            if mr.size < size:
                raise ValueError(
                    f"Existing {mr} at {base:#x} is smaller than the requested {size:#x}"
                )
            for m in self.vm.maps:
                if m.mr is mr and m.vaddr == gpa:
                    return m
        else:
            mr = MemoryRegion(
                self.sdf, name or f"{self.vm.name}_mmio_{base:x}", size, paddr=base, cached=False
            )

        guest_map = Map(mr, gpa, "rw")
        self.vm.add_map(guest_map)
        return guest_map

    def add_passthrough_irq(self, irq: IRQ, guest_irq: Optional[int] = None) -> int:
        """
        Route a host IRQ to the VMM for injection into the guest. Returns the
        VMM's channel ID for it.

        `guest_irq` is what the guest sees: a GIC INTID on ARM, an I/O APIC pin
        on x86. It defaults to the host number (ARM) or host pin (x86 I/O APIC).
        MSIs have no sensible default and need it given explicitly.
        """
        if guest_irq is None:
            if isinstance(irq, IrqIoapic):
                guest_irq = irq.pin
            elif self.sdf.arch.is_x86():
                raise ValueError("guest_irq must be given for x86 MSI passthrough")
            else:
                guest_irq = irq.number
        if len(self.irqs) >= VMM_MAX_IRQS:
            raise ValueError(f"libvmm supports at most {VMM_MAX_IRQS} passthrough IRQs")

        irq_id = self.vmm.add_irq(irq)
        self.irqs.append(PassthroughIrq(irq, guest_irq))
        return irq_id

    def add_passthrough_ioport(self, addr: int, size: int) -> IOPort:
        """Give the guest direct access to an x86 I/O port range."""
        self._require_x86("I/O port passthrough")
        if len(self.ioports) >= VMM_MAX_IOPORTS:
            raise ValueError(f"libvmm supports at most {VMM_MAX_IOPORTS} passthrough I/O ports")
        if not (0 <= addr and addr + size <= 0x10000):
            raise ValueError(f"I/O port range [{addr:#x}, +{size:#x}) is out of range")
        ioport = IOPort(addr, size)
        self.vmm.add_ioport(ioport)
        self.ioports.append(ioport)
        return ioport

    def add_passthrough_device(
        self,
        node: Union[DTBNode, str],
        *,
        dtb: Optional[DeviceTreeBlob] = None,
        regions: Optional[Sequence[int]] = None,
        irqs: Optional[Sequence[int]] = None,
    ) -> PassthroughResources:
        """
        DTB convenience over add_passthrough_mmio/irq. `node` is a node (or
        path) in the host DTB (`sdf.dtb` unless `dtb` is given). `regions` and
        `irqs` are indices into its 'reg' and 'interrupts'; None means all.
        """
        dtb = dtb if dtb is not None else self.sdf.dtb
        if dtb is None:
            raise ValueError("add_passthrough_device needs a host DTB on the System or dtb=")
        node = self._resolve_node(dtb, node)
        arch = self.sdf.arch
        page_mask = arch.default_page_size_bytes() - 1

        regs = dtb.get_node_regs(node) if dtb.get_node_prop(node, "reg") is not None else []
        reg_idxs = range(len(regs)) if regions is None else regions
        if any(i >= len(regs) for i in reg_idxs):
            raise ValueError(f"{node.path}: region indices {list(reg_idxs)} out of range ({len(regs)} regs)")

        parsed_irqs = self._node_irqs(dtb, node)
        irq_idxs = range(len(parsed_irqs)) if irqs is None else irqs
        if any(i >= len(parsed_irqs) for i in irq_idxs):
            raise ValueError(f"{node.path}: irq indices {list(irq_idxs)} out of range ({len(parsed_irqs)} irqs)")

        out = PassthroughResources()
        for i in reg_idxs:
            addr, size = regs[i]
            paddr = dtb.get_reg_paddr(arch, node, addr) + (addr & page_mask)
            name = node.name if len(regs) == 1 else f"{node.name}/{i}"
            out.maps.append(self.add_passthrough_mmio(paddr, size, name=name))
        for i in irq_idxs:
            out.irq_ids.append(self.add_passthrough_irq(parsed_irqs[i]))
        return out

    # ------------------------------------------------------------------
    # virtIO. The VMM always becomes an sDDF client. On DTB guests the device
    # is virtIO MMIO and described by a guest DTB node. On x86 it is virtIO PCI,
    # which the VMM sets up in C for now, so no node is taken.
    # ------------------------------------------------------------------

    def add_virtio_mmio_device(
        self, node: Union[DTBNode, str], type: VirtioDeviceType
    ) -> VirtioMmioDevice:
        if self.guest_dtb is None:
            raise ValueError("virtIO MMIO devices need a guest DTB")
        if len(self.virtio_mmio_devices) >= VMM_MAX_VIRTIO_MMIO_DEVICES:
            raise ValueError(f"libvmm supports at most {VMM_MAX_VIRTIO_MMIO_DEVICES} virtIO MMIO devices")
        dtb = self.guest_dtb
        node = self._resolve_node(dtb, node)
        arch = self.sdf.arch

        if dtb.get_node_prop(node, "reg") is None:
            raise ValueError(f"virtIO device '{node.path}': missing 'reg'")
        regs = dtb.get_node_regs(node)
        if len(regs) != 1:
            raise ValueError(f"virtIO device '{node.path}': expected 1 region, found {len(regs)}")
        node_irqs = self._node_irqs(dtb, node)
        if len(node_irqs) != 1:
            raise ValueError(f"virtIO device '{node.path}': expected 1 interrupt, found {len(node_irqs)}")

        dev = VirtioMmioDevice(
            type=type,
            base=dtb.get_reg_paddr(arch, node, regs[0][0]),
            size=arch.roundup_to_page(regs[0][1]),
            irq=node_irqs[0].number,
        )
        self.virtio_mmio_devices.append(dev)
        return dev

    def add_virtio_console(self, serial: "sDDFSerial", node: Union[DTBNode, str, None] = None) -> None:
        self._check_virtio_node(node)
        serial.add_client(self.vmm)
        if node is not None:
            self.add_virtio_mmio_device(node, VirtioDeviceType.CONSOLE)

    def add_virtio_blk(
        self,
        blk: "sDDFBlk",
        partition_number: int,
        node: Union[DTBNode, str, None] = None,
        **client_options,
    ) -> None:
        self._check_virtio_node(node)
        blk.add_client(self.vmm, partition_number, **client_options)
        if node is not None:
            self.add_virtio_mmio_device(node, VirtioDeviceType.BLK)

    def add_virtio_net(
        self,
        net: "sDDFEthernet",
        copier: Optional[ProtectionDomain] = None,
        node: Union[DTBNode, str, None] = None,
        **client_options,
    ) -> None:
        self._check_virtio_node(node)
        net.add_client(self.vmm, copier=copier, **client_options)
        if node is not None:
            self.add_virtio_mmio_device(node, VirtioDeviceType.NET)

    # ------------------------------------------------------------------
    # Linux UIO
    # ------------------------------------------------------------------

    def uio(self, name: str) -> Optional[UioRegion]:
        return next((u for u in self.uios if u.name == name), None)

    def map_uio(
        self,
        name: str,
        mr: Optional[MemoryRegion] = None,
        *,
        into_guest: bool = True,
    ) -> Map:
        """
        Back a UIO region with `mr` (a fresh MR if None) and map it into the
        VMM, recording the vaddr in config. Pass the same MR to another VMM's
        map_uio to share it. UIO regions that only exist to generate faults
        should not be mapped at all.
        """
        u = self.uio(name)
        if u is None:
            raise ValueError(f"No UIO region named '{name}'")
        if u.vmm_vaddr != 0:
            raise ValueError(f"UIO region '{name}' is already mapped")
        if mr is None:
            mr = MemoryRegion(self.sdf, f"{self.vm.name}_uio_{name}", u.size)
        elif mr.size < u.size:
            raise ValueError(f"{mr} is smaller than UIO region '{name}' ({u.size:#x})")
        if into_guest:
            self.vm.add_map(Map(mr, u.guest_paddr, "rw"))
        vmm_map = self.vmm.create_automap(mr, "rw")
        u.vmm_vaddr = vmm_map.vaddr
        return vmm_map

    # ------------------------------------------------------------------
    # Build
    # ------------------------------------------------------------------

    @build_hook
    def check_guest(self) -> None:
        if not any(r.kind == GuestMemoryKind.RAM for r in self.memory):
            raise SubsystemBuildError(f"{self}: guest '{self.vm.name}' has no RAM")

    def generate_config_structs(self) -> List[ConfigStruct]:
        fields = {
            "magic": VMM_MAGIC,
            "num_ram_regions": len(self.memory),
            "ram_regions": [
                ConfigStruct(
                    {"gpa": r.gpa, "size": r.size, "vmm_vaddr": r.vmm_vaddr, "kind": int(r.kind)},
                    "vmm_config_ram_region_t",
                )
                for r in self.memory
            ],
            "dtb": self.dtb_gpa,
            "initrd": self.initrd_gpa,
            "num_irqs": len(self.irqs),
            "irqs": [
                ConfigStruct({"id": p.irq.id, "irq": p.guest_irq}, "vmm_config_irq_t")
                for p in self.irqs
            ],
            "num_vcpus": len(self.vm.vcpus),
            "vcpus": [ConfigStruct({"id": v.id}, "vmm_config_vcpu_t") for v in self.vm.vcpus],
            "num_virtio_mmio_devices": len(self.virtio_mmio_devices),
            "virtio_mmio_devices": [
                ConfigStruct(
                    {"type": int(d.type), "base": d.base, "size": d.size, "irq": d.irq},
                    "vmm_config_virtio_mmio_device_t",
                )
                for d in self.virtio_mmio_devices
            ],
            "num_ioports": len(self.ioports),
            "ioports": [
                ConfigStruct({"id": io.id, "addr": io.addr, "size": io.size}, "vmm_config_ioport_t")
                for io in self.ioports
            ],
            "num_uio_regions": len(self.uios),
            "uios": [
                ConfigStruct(
                    {
                        "name": u.name,
                        "guest_paddr": u.guest_paddr,
                        "vmm_vaddr": u.vmm_vaddr,
                        "size": u.size,
                        "irq": u.irq or 0,
                    },
                    "vmm_config_uio_region_t",
                )
                for u in self.uios
            ],
        }
        return [
            ConfigStruct(
                fields,
                type_name="vmm_config_t",
                section_name="vmm_config",
                target_file=self.vmm.prog_image,
            )
        ]

    # ------------------------------------------------------------------
    # DTB auto-configuration
    # ------------------------------------------------------------------

    def _configure_from_dtb(self, dtb: DeviceTreeBlob, dtb_size: int) -> None:
        arch = self.sdf.arch
        if arch.is_arm():
            self._map_gic_vcpu(dtb)

        memory_node = self._find_memory_node(dtb)
        if memory_node is None:
            raise ValueError(f"Guest DTB for '{self.vm.name}' has no memory node")
        for gpa, size in dtb.get_node_regs(memory_node):
            # VMM vaddr == GPA keeps the existing ARM layout.
            self.add_ram(
                gpa,
                size,
                paddr=gpa if self.one_to_one_ram else None,
                vmm_vaddr=gpa,
            )

        initrd = self._initrd_range(dtb)
        home = self.memory[0]
        if initrd is not None:
            start, end = initrd
            home = self.ram_region_containing(start, end - start)
            if home is None:
                raise ValueError(
                    f"initrd [{start:#x}, {end:#x}) is not inside guest RAM of '{self.vm.name}'"
                )
            self.initrd_gpa = start
        self.dtb_gpa = self._allocate_dtb_gpa(home, dtb_size, initrd)

        self._parse_uios(dtb)

    def _map_gic_vcpu(self, dtb: DeviceTreeBlob) -> None:
        gic = Arm_GIC.create(self.sdf.arch, dtb)
        if gic is None:
            raise ValueError(f"Guest DTB for '{self.vm.name}' has no GIC node")
        if not gic.has_mmio_cpu_interface():
            return  # GICv3 with system register CPU interface
        if gic.vcpu_paddr is None or gic.vcpu_size is None:
            raise ValueError("GIC has an MMIO CPU interface but no vCPU interface 'reg' entry")

        # One MR for the vCPU interface, shared by every VMM in the system.
        name = "gic_vcpu"
        mr = next((m for m in self.sdf.mrs if m.name == name), None)
        if mr is None:
            mr = MemoryRegion(
                self.sdf,
                name,
                self.sdf.arch.roundup_to_page(gic.vcpu_size),
                paddr=gic.vcpu_paddr,
                cached=False,
            )
        self.gic_vcpu_mr = mr
        # The guest sees the vCPU interface where it expects the CPU interface.
        self.vm.add_map(Map(mr, gic.cpu_paddr, "rw"))

    @staticmethod
    def _find_memory_node(dtb: DeviceTreeBlob) -> Optional[DTBNode]:
        for node in dtb.nodes.values():
            dt = dtb.get_node_prop(node, "device_type")
            if dt is not None and bytes(dt).rstrip(b"\0") == b"memory":
                return node
        return None

    @staticmethod
    def _initrd_range(dtb: DeviceTreeBlob) -> Optional[tuple]:
        try:
            chosen = dtb.get_node_by_path("/chosen")
        except Exception:
            return None
        start = dtb.get_node_prop(chosen, "linux,initrd-start")
        end = dtb.get_node_prop(chosen, "linux,initrd-end")
        if start is None and end is None:
            return None
        if start is None or end is None:
            raise ValueError("/chosen has only one of linux,initrd-start and linux,initrd-end")
        s, e = int.from_bytes(bytes(start), "big"), int.from_bytes(bytes(end), "big")
        if e <= s:
            raise ValueError(f"initrd end {e:#x} is not after start {s:#x}")
        return (s, e)

    def _allocate_dtb_gpa(
        self, region: GuestMemoryRegion, dtb_size: int, initrd: Optional[tuple]
    ) -> int:
        """
        Page aligned DTB placement inside `region`: straight after the initrd,
        else straight before it, else at the top of RAM when there is no initrd.
        """
        arch = self.sdf.arch
        size = arch.roundup_to_page(dtb_size)
        if initrd is None:
            gpa = arch.rounddown_to_page(region.end - size)
            if gpa >= region.gpa:
                return gpa
        else:
            after = arch.roundup_to_page(initrd[1])
            if after + size <= region.end:
                return after
            before = arch.rounddown_to_page(initrd[0]) - size
            if before >= region.gpa:
                return before
        raise ValueError(f"No room for a {dtb_size:#x} byte DTB in guest RAM of '{self.vm.name}'")

    def _parse_uios(self, dtb: DeviceTreeBlob) -> None:
        """
        Record every generic-uio node. Nothing is mapped: see map_uio().
        """
        arch = self.sdf.arch
        nodes = dtb.get_nodes_by_compatible(LINUX_UIO_COMPATIBLE)
        if len(nodes) > VMM_MAX_UIOS:
            raise ValueError(f"Found {len(nodes)} UIO nodes, the limit is {VMM_MAX_UIOS}")

        for node in nodes:
            compat = dtb.get_compatible(node)
            if len(compat) != 2:
                raise ValueError(
                    f"UIO node '{node.path}' compatible {compat} is not 'generic-uio\\0<name>'"
                )
            name = compat[1]
            if len(name) > VMM_UIO_NAME_MAX_LEN - 1:
                raise ValueError(f"UIO name '{name}' is longer than {VMM_UIO_NAME_MAX_LEN - 1} chars")
            if self.uio(name) is not None:
                raise ValueError(f"Duplicate UIO name '{name}'")

            if dtb.get_node_prop(node, "reg") is None:
                raise ValueError(f"UIO node '{node.path}' has no 'reg'")
            regs = dtb.get_node_regs(node)
            if len(regs) != 1:
                raise ValueError(f"UIO node '{node.path}' must have 1 region, found {len(regs)}")
            addr, size = regs[0]
            if not arch.addr_is_page_aligned(addr) or not arch.addr_is_page_aligned(size):
                raise ValueError(f"UIO node '{node.path}' region is not page aligned")

            node_irqs = self._node_irqs(dtb, node)
            if len(node_irqs) > 1:
                raise ValueError(f"UIO node '{node.path}' must have at most 1 interrupt")

            self.uios.append(
                UioRegion(
                    name=name,
                    guest_paddr=dtb.get_reg_paddr(arch, node, addr),
                    size=size,
                    irq=node_irqs[0].number if node_irqs else None,
                )
            )

    # ------------------------------------------------------------------
    # Helpers
    # ------------------------------------------------------------------

    def _node_irqs(self, dtb: DeviceTreeBlob, node: DTBNode) -> List[IRQ]:
        # DeviceTreeBlob.get_node_irqs() crashes on nodes without interrupts,
        # so check first.
        if dtb.get_node_prop(node, "interrupts") is None:
            return []
        return list(dtb.get_parsed_irqs(node, self.sdf.arch))

    @staticmethod
    def _resolve_node(dtb: DeviceTreeBlob, node: Union[DTBNode, str]) -> DTBNode:
        return dtb.get_node_by_path(node) if isinstance(node, str) else node

    def _check_virtio_node(self, node) -> None:
        if self.sdf.arch.is_x86():
            if node is not None:
                raise ValueError("x86 guests use virtIO PCI, no DTB node is taken")
        elif node is None:
            raise ValueError("DTB guests use virtIO MMIO, pass the guest DTB node")

    def _require_x86(self, what: str) -> None:
        if not self.sdf.arch.is_x86():
            raise ValueError(f"{what} is x86 only")