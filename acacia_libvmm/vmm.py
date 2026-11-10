# Copyright 2026, UNSW
# SPDX-License-Identifier: BSD-2-Clause
"""
Acacia subsystem for a libvmm VMM and its guest.

`LibvmmVirtMachine` creates the Microkit resources a guest needs (memory
regions, maps, IRQs, I/O ports, sDDF connections) and records what the VMM
needs to know about them in a `VmmConfig` (see config.py), which becomes the
VMM's `vmm_config_t`.

It is an Acacia Subsystem only so that Acacia collects that config struct.

One class covers both kinds of guest:

* DTB guests (ARM, and in the future RISC-V): the guest DTB is the source of
  truth. Guest RAM, the GIC vCPU interface (ARM), initrd and DTB placement
  are all created from it in __init__. You then add passthrough
  and virtIO MMIO devices on top.
* x86 guests: there is no DTB, so the user describes the guest with
  add_x86_ram, add_x86_flash and the add_passthrough_* methods. virtIO devices
  are virtIO PCI, which the VMM sets up in C, so no device node is taken.
"""

from __future__ import annotations

from typing import TYPE_CHECKING, List, Optional, Sequence

from acacia import (
    IRQ,
    Arch,
    ConfigStruct,
    DeviceTreeBlob,
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

from .config import (
    VMM_MAX_IOPORTS,
    VMM_MAX_IRQS,
    VMM_MAX_RAM_REGIONS,
    VMM_MAX_VCPUS,
    VMM_MAX_DT_DEVICES,
    GuestMemoryKind,
    GuestMemoryRegion,
    PassthroughIrq,
    GuestDtDevice,
    VmmConfig,
)
from .dtb import LibvmmDtbReader, Range

if TYPE_CHECKING:
    from acacia_sddf import sDDFBlk, sDDFEthernet, sDDFSerial

# x86 guest physical memory map. Must match include/libvmm/arch/x86_64/memory_space.h.
X86_LOW_RAM_START_GPA = 0x0
X86_ECAM_GPA = 0xD000_0000
X86_LAPIC_GPA = 0xFEE0_0000
X86_LAPIC_SIZE = 0x1000
X86_HIGH_RAM_START_GPA = 0x1_0000_0000
# Firmware flash ends at 4GiB, where the CPU's reset vector is.
X86_FLASH_END_GPA = 0x1_0000_0000
# How much RAM add_x86_ram() puts below 4GiB by default. It must stay below the
# PCI MMIO window that the guest's DSDT advertises, amongst other things like the LAPIC,
# IOAPIC, HPET, etc.
X86_DEFAULT_LOW_RAM_LIMIT = 0x8000_0000
X86_IOPORT_SPACE_SIZE = 0x1_0000

# One MR for the physical GIC vCPU interface, shared by every VMM in the system.
GIC_VCPU_MR_NAME = "gic_vcpu"


class LibvmmVirtMachine(Subsystem):
    def __init__(
        self,
        sdf: System,
        vmm: ProtectionDomain,
        guest_dtb: Optional[DeviceTreeBlob] = None,
        *,
        one_to_one_ram: bool = False,
    ) -> None:
        """
        :param vmm: VMM PD. Must already own the guest via `vmm.add_vm()`.
        :param guest_dtb: Guest DTB. Required on ARM/RISC-V, must be None on x86.
        :param guest_dtb_size: Size of the DTB the VMM copies into the guest, if
            it differs from `guest_dtb`. DTB guests only.
        :param one_to_one_ram: Back guest RAM with the physical memory at the
            same address as its GPA. Non-x86 guests only.
        """
        if vmm.vm is None:
            raise ValueError(
                f"VMM PD '{vmm.name}' has no virtual machine, call vmm.add_vm() first"
            )
        _check_guest_description(sdf.arch, guest_dtb, one_to_one_ram)
        _check_vcpus(sdf.arch, vmm.vm)

        super().__init__(sdf, f"libvmm_{vmm.name}", clients_allowed=False)

        self.vmm: ProtectionDomain = vmm
        self.vm: VirtualMachine = vmm.vm
        self.guest_dtb = guest_dtb
        self.config = VmmConfig(vcpu_ids=[v.id for v in self.vm.vcpus])
        self.gic_vcpu_mr: Optional[MemoryRegion] = None

        self._guest_dtb_reader: Optional[LibvmmDtbReader] = None
        if guest_dtb is not None:
            self._guest_dtb_reader = LibvmmDtbReader(guest_dtb, sdf.arch)
            self._configure_from_guest_dtb(self._guest_dtb_reader, one_to_one_ram)

    # ------------------------------------------------------------------
    # Guest memory
    # ------------------------------------------------------------------

    def _add_ram(
        self,
        gpa: int,
        size: int,
        *,
        kind: GuestMemoryKind = GuestMemoryKind.RAM,
        name: Optional[str] = None,
        paddr: Optional[int] = None,
        vmm_vaddr: Optional[int] = None,
    ):
        """
        Add guest memory at `gpa`, automatically mapped rwx into the guest and rw into the VMM.

        :param kind: RAM, or FLASH for firmware.
        :param name: MR name. Defaults to guest_<vm>_<kind>, plus _<n> after the first.
        :param paddr: Back the region with this physical memory. It is not recommended to
                      use this option on x86 due to its dynamic memory nature.
        :param vmm_vaddr: Where the VMM sees the region. Automatically allocated by default.
        """
        arch = self.sdf.arch
        if not (arch.addr_is_page_aligned(gpa) and arch.addr_is_page_aligned(size)):
            raise ValueError(f"Guest memory [{gpa:#x}, +{size:#x}) is not page aligned")
        _check_room(
            self.config.ram_regions, VMM_MAX_RAM_REGIONS, "guest memory regions"
        )

        mr = MemoryRegion(
            self.sdf, name or self._default_memory_name(kind), size, paddr=paddr
        )
        self._check_gpa_range_free(gpa, mr.size, f"Guest memory '{mr.name}'")
        self.vm.add_map(Map(mr, gpa, "rwx"))
        vmm_map = self._map_into_vmm(mr, vmm_vaddr)

        region = GuestMemoryRegion(kind, gpa, size, mr, vmm_map)
        self.config.ram_regions.append(region)

    def _ram_region_containing(
        self, gpa: int, size: int = 1
    ) -> Optional[GuestMemoryRegion]:
        return next(
            (
                r
                for r in self.config.ram_regions
                if r.gpa <= gpa and gpa + size <= r.end
            ),
            None,
        )

    def add_x86_ram(self, size: int, low_ram_limit: int = X86_DEFAULT_LOW_RAM_LIMIT):
        """
        Add `size` bytes of guest RAM using the classical PC layout: up to `low_ram_limit`
        bytes from GPA 0, and the rest from 4GiB onwards.

        `low_ram_limit` must stay below the PCI MMIO window that your DSDT
        advertises, amongst other things like the APIC and HPET.
        """
        self._require_x86("add_x86_ram")
        if low_ram_limit > X86_ECAM_GPA:
            raise ValueError(
                f"Low RAM limit {low_ram_limit:#x} runs into ECAM at {X86_ECAM_GPA:#x}"
            )

        low_size = min(size, low_ram_limit)
        high_size = size - low_size
        regions = [
            self._add_ram(
                X86_LOW_RAM_START_GPA, low_size, name=f"guest_{self.vm.name}_ram_low"
            )
        ]
        if high_size > 0:
            regions.append(
                self._add_ram(
                    X86_HIGH_RAM_START_GPA,
                    high_size,
                    name=f"guest_{self.vm.name}_ram_high",
                )
            )

    def add_x86_flash(self, size: int):
        """Add firmware flash (e.g. OVMF) that ends at 4GiB."""
        self._require_x86("add_x86_flash")
        gpa = X86_FLASH_END_GPA - size
        lapic_end = X86_LAPIC_GPA + X86_LAPIC_SIZE
        if gpa < lapic_end:
            raise ValueError(
                f"{size:#x} bytes of flash would cover the emulated IOAPIC, HPET and "
                f"LAPIC, which end at {lapic_end:#x}"
            )
        self._add_ram(gpa, size, kind=GuestMemoryKind.FLASH)

    # ------------------------------------------------------------------
    # Passthrough
    # ------------------------------------------------------------------

    def add_passthrough_mmio(
        self,
        paddr: int,
        size: int,
        *,
        gpa: Optional[int] = None,
        name: Optional[str] = None,
    ):
        """
        Map device MMIO at host physical `paddr` into the guest, uncached, at
        `gpa` (by default the same address). The range is widened to whole
        pages.

        Devices can share a page, so an MR that already covers exactly this
        range is reused rather than duplicated.
        """
        arch = self.sdf.arch
        base = arch.rounddown_to_page(paddr)
        size = arch.roundup_to_page(paddr + size) - base
        if gpa is None:
            gpa = base
        elif not arch.addr_is_page_aligned(gpa):
            raise ValueError(f"Passthrough GPA {gpa:#x} is not page aligned")

        mr = self._device_mr(base, size, name)
        for m in self.vm.maps:
            if m.mr is mr and m.vaddr == gpa:
                return m  # Another device on the same page already mapped it.

        self._check_gpa_range_free(gpa, mr.size, f"Passthrough MMIO '{mr.name}'")
        guest_map = Map(mr, gpa, "rw")
        self.vm.add_map(guest_map)

    def add_passthrough_irq(self, irq: IRQ, guest_irq: Optional[int] = None):
        """
        Route a host IRQ to the VMM for injection into the guest. Returns the
        VMM's channel ID for it.

        `guest_irq` is what the guest sees: a GIC INTID on ARM, an I/O APIC pin
        on x86. It defaults to the host IRQ number (ARM) or I/O APIC pin (x86).
        """
        if guest_irq is None:
            guest_irq = self._default_guest_irq(irq)
        _check_room(self.config.irqs, VMM_MAX_IRQS, "passthrough IRQs")

        self.vmm.add_irq(irq)
        self.config.irqs.append(PassthroughIrq(irq, guest_irq))

    def add_passthrough_ioport(self, addr: int, size: int):
        """Give the guest access to the x86 I/O ports [addr, addr + size)."""
        self._require_x86("I/O port passthrough")
        if size <= 0 or addr < 0 or addr + size > X86_IOPORT_SPACE_SIZE:
            raise ValueError(f"I/O port range [{addr:#x}, +{size:#x}) is invalid")
        _check_room(self.config.ioports, VMM_MAX_IOPORTS, "passthrough I/O port ranges")
        for other in self.config.ioports:
            if _overlaps(addr, addr + size, other.addr, other.addr + other.size):
                raise ValueError(
                    f"I/O port range [{addr:#x}, +{size:#x}) overlaps {other}"
                )

        ioport = IOPort(addr, size)
        self.vmm.add_ioport(ioport)
        self.config.ioports.append(ioport)

    def add_passthrough_dt_device(
        self,
        node_path: str,
        *,
        dtb: Optional[DeviceTreeBlob] = None,
        regions: Optional[Sequence[int]] = None,
        irqs: Optional[Sequence[int]] = None,
    ):
        """
        Pass through the device at `node` (a node or path) in the host DTB,
        which is `sdf.dtb` unless `dtb` is given. `regions` and `irqs` are
        indices into its 'reg' and 'interrupts', and default to all of them.
        """
        dtb = dtb if dtb is not None else self.sdf.dtb
        if dtb is None:
            raise ValueError(
                "add_passthrough_dt_device needs a host DTB, on the System or as dtb="
            )
        reader = LibvmmDtbReader(dtb, self.sdf.arch)
        node = reader.node(node_path)
        node_regs = reader.regs(node)
        node_irqs = reader.irqs(node)
        reg_indices = _select(regions, len(node_regs), f"{node.path}: 'reg'")
        irq_indices = _select(irqs, len(node_irqs), f"{node.path}: 'interrupts'")

        for i in reg_indices:
            paddr, size = node_regs[i]
            name = node.name if len(node_regs) == 1 else f"{node.name}/{i}"
            self.add_passthrough_mmio(paddr, size, name=name)
        for i in irq_indices:
            self.add_passthrough_irq(node_irqs[i])

    # ------------------------------------------------------------------
    # virtIO MMIO helpers. The VMM always becomes a client of the sDDF subsystem backing
    # the device. DTB guests get virtIO MMIO, described by a node in the guest
    # DTB. virtIO PCI is a dynamic construct, that is the devices on PCI are not specified
    # by the device tree. So we leave the setup to the VMM.
    # ------------------------------------------------------------------

    def add_virtio_mmio_device(self, node_path: str):
        """
        Record the virtIO MMIO device at `node_path` in the guest DTB. Its region
        stays unmapped, so guest accesses fault into the VMM, which emulates it.
        """
        dtb = self._require_guest_dtb("virtIO MMIO devices")
        node = dtb.node(node_path)
        regs, irqs = dtb.regs(node), dtb.irqs(node)
        if len(regs) != 1:
            raise ValueError(
                f"virtIO device '{node.path}': expected 1 region, found {len(regs)}"
            )
        if len(irqs) != 1:
            raise ValueError(
                f"virtIO device '{node.path}': expected 1 interrupt, found {len(irqs)}"
            )
        _check_room(
            self.config.guest_dt_devices,
            VMM_MAX_DT_DEVICES,
            "guest DT-based devices",
        )

        arch = self.sdf.arch
        paddr, size = regs[0]
        base = arch.rounddown_to_page(paddr)
        size = arch.roundup_to_page(paddr + size) - base
        self._check_gpa_range_free(base, size, f"virtIO device '{node.path}'")

        device = GuestDtDevice(node.name, base, size, 0, irqs[0].number)
        self.config.guest_dt_devices.append(device)

    def add_virtio_mmio_console(self, serial: "sDDFSerial", node_path: str) -> None:
        self._add_virtio_mmio_device(node_path)
        serial.add_client(self.vmm)

    def add_virtio_mmio_blk(
        self,
        blk: "sDDFBlk",
        partition_number: int,
        node_path: str,
        **client_options,
    ) -> None:
        self._add_virtio_mmio_device(node_path)
        blk.add_client(self.vmm, partition_number, **client_options)

    def add_virtio_mmio_net(
        self,
        net: "sDDFEthernet",
        node_path: str,
        copier: Optional[ProtectionDomain] = None,
        **client_options,
    ) -> None:
        self._add_virtio_mmio_device(node_path)
        net.add_client(self.vmm, copier=copier, **client_options)

    def _add_virtio_mmio_device(self, node_path: str) -> None:
        if self.sdf.arch.is_x86():
            raise ValueError(
                "x86 guests use virtIO PCI and is not DT-based. So just add the VMM as a client of the appropriate sDDF subsystem and set it up in C"
            )
        self.add_virtio_mmio_device(node_path)

    # ------------------------------------------------------------------
    # Build
    # ------------------------------------------------------------------

    @build_hook
    def check_guest(self) -> None:
        if not any(r.kind == GuestMemoryKind.RAM for r in self.config.ram_regions):
            raise SubsystemBuildError(f"{self}: guest '{self.vm.name}' has no RAM")

    def generate_config_structs(self) -> List[ConfigStruct]:
        return [self.config.to_config_struct(target_file=self.vmm.prog_image)]

    # ------------------------------------------------------------------
    # Setting up a DTB guest
    # ------------------------------------------------------------------

    def _configure_from_guest_dtb(
        self, dtb_rdr: LibvmmDtbReader, one_to_one_ram: bool
    ) -> None:
        if self.sdf.arch.is_arm():
            self._map_gic_vcpu_interface()

        memory = dtb_rdr.memory()
        if not memory:
            raise ValueError(f"Guest DTB for '{self.vm.name}' has no memory node")
        for gpa, size in memory:
            # The VMM sees guest RAM at vaddr == GPA, as the hand-written ARM VMMs did.
            self._add_ram(
                gpa, size, paddr=gpa if one_to_one_ram else None, vmm_vaddr=gpa
            )

        dtb_size = len(dtb_rdr.dtb.fdt.as_bytearray())
        self._place_initrd_and_dtb(dtb_rdr.initrd(), dtb_size)

    def _map_gic_vcpu_interface(self) -> None:
        """Show the guest the GIC's vCPU interface where it expects the CPU interface."""
        gic = Arm_GIC.create(self.sdf.arch, self.guest_dtb)
        if gic is None:
            raise ValueError(f"Guest DTB for '{self.vm.name}' has no GIC node")
        if not gic.has_mmio_cpu_interface():
            return  # GICv3 with a system register CPU interface.
        if gic.vcpu_paddr is None or gic.vcpu_size is None:
            raise ValueError(
                "GIC has an MMIO CPU interface but no vCPU interface 'reg' entry"
            )

        mr = next((m for m in self.sdf.mrs if m.name == GIC_VCPU_MR_NAME), None)
        if mr is None:
            mr = MemoryRegion(
                self.sdf,
                GIC_VCPU_MR_NAME,
                self.sdf.arch.roundup_to_page(gic.vcpu_size),
                paddr=gic.vcpu_paddr,
                cached=False,
            )
        self.gic_vcpu_mr = mr
        self._check_gpa_range_free(gic.cpu_paddr, mr.size, "GIC CPU interface")
        self.vm.add_map(Map(mr, gic.cpu_paddr, "rw"))

    def _place_initrd_and_dtb(self, initrd: Optional[Range], dtb_size: int) -> None:
        """
        The initrd stays where /chosen says. The DTB goes in the same RAM
        region, page aligned: straight after the initrd, else straight before
        it. Without an initrd it goes at the top of the first RAM region.
        """
        if initrd is None:
            region = self.config.ram_regions[0]
        else:
            start, size = initrd
            region = self._ram_region_containing(start, size)
            if region is None:
                raise ValueError(
                    f"initrd [{start:#x}, +{size:#x}) is not inside guest RAM of '{self.vm.name}'"
                )
            self.config.initrd = start

        dtb_gpa = _find_dtb_gpa(self.sdf.arch, region, dtb_size, initrd)
        if dtb_gpa is None:
            raise ValueError(
                f"No room for a {dtb_size:#x} byte DTB in guest RAM of '{self.vm.name}'"
            )
        self.config.dtb = dtb_gpa

    # ------------------------------------------------------------------
    # Helpers
    # ------------------------------------------------------------------

    def _check_gpa_range_free(self, gpa: int, size: int, what: str) -> None:
        """Raise if anything already claims part of [gpa, gpa + size) in the guest."""
        end = gpa + size
        for m in self.vm.maps:
            if _overlaps(gpa, end, m.vaddr, m.end_vaddr):
                raise ValueError(
                    f"{what} at [{gpa:#x}, {end:#x}) overlaps '{m.mr.name}' "
                    f"at [{m.vaddr:#x}, {m.end_vaddr:#x})"
                )
        for d in self.config.guest_dt_devices:
            if _overlaps(gpa, end, d.gpa_base, d.gpa_end):
                raise ValueError(
                    f"{what} at [{gpa:#x}, {end:#x}) overlaps the virtIO device "
                    f"at [{d.gpa_base:#x}, {d.gpa_end:#x})"
                )

    def _device_mr(self, paddr: int, size: int, name: Optional[str]) -> MemoryRegion:
        """The uncached MR for [paddr, paddr + size), created if it doesn't exist yet."""
        for mr in self.sdf.mrs:
            if mr.paddr is None or not _overlaps(
                paddr, paddr + size, mr.paddr, mr.paddr + mr.size
            ):
                continue
            if mr.paddr == paddr and mr.size >= size and not mr.cached:
                return mr
            raise ValueError(f"Device MMIO [{paddr:#x}, +{size:#x}) overlaps {mr}")
        return MemoryRegion(
            self.sdf,
            name or f"{self.vm.name}_mmio_{paddr:x}",
            size,
            paddr=paddr,
            cached=False,
        )

    def _map_into_vmm(self, mr: MemoryRegion, vaddr: Optional[int]) -> Map:
        if vaddr is None:
            return self.vmm.create_automap(mr, "rw")
        vmm_map = Map(mr, vaddr, "rw")
        self.vmm.add_map(vmm_map)
        return vmm_map

    def _default_memory_name(self, kind: GuestMemoryKind) -> str:
        name = f"guest_{self.vm.name}_{kind.name.lower()}"
        n = sum(1 for r in self.config.ram_regions if r.kind == kind)
        return name if n == 0 else f"{name}_{n}"

    def _default_guest_irq(self, irq: IRQ) -> int:
        if isinstance(irq, IrqIoapic):
            return irq.pin
        return irq.number

    def _require_x86(self, what: str) -> None:
        if not self.sdf.arch.is_x86():
            raise ValueError(f"{what} is x86 only")

    def _require_guest_dtb(self, what: str) -> DtbReader:
        if self._guest_dtb_reader is None:
            raise ValueError(f"{what} need a guest DTB")
        return self._guest_dtb_reader


def _check_guest_description(
    arch: Arch,
    guest_dtb: Optional[DeviceTreeBlob],
    one_to_one_ram: bool,
) -> None:
    if not arch.is_x86():
        if guest_dtb is None:
            raise ValueError("ARM and RISC-V guests need a guest DTB")
        return
    if guest_dtb is not None:
        raise ValueError("x86 guests are described by ACPI, not a DTB")
    if one_to_one_ram:
        raise ValueError(
            "one_to_one_ram is not supported on x86, use the IOMMU instead"
        )


def _check_vcpus(arch: Arch, vm: VirtualMachine) -> None:
    """libvmm uses vCPU IDs as indices on ARM, and always drives vCPU 0 on x86."""
    ids = sorted(v.id for v in vm.vcpus)
    if not ids:
        raise ValueError(f"VM '{vm.name}' has no vCPUs")
    if len(ids) > VMM_MAX_VCPUS:
        raise ValueError(f"libvmm supports at most {VMM_MAX_VCPUS} vCPUs per VM")
    if arch.is_x86() and ids != [0]:
        raise ValueError(
            f"An x86 VMM drives exactly one vCPU, with ID 0. '{vm.name}' has {ids}"
        )
    if ids != list(range(len(ids))):
        raise ValueError(
            f"VM '{vm.name}' needs vCPU IDs 0 to {len(ids) - 1}, not {ids}"
        )


def _find_dtb_gpa(
    arch: Arch, region: GuestMemoryRegion, dtb_size: int, initrd: Optional[Range]
) -> Optional[int]:
    """See LibvmmVirtMachine._place_initrd_and_dtb()."""
    size = arch.roundup_to_page(dtb_size)
    if initrd is None:
        candidates = [arch.rounddown_to_page(region.end - size)]
    else:
        start, initrd_size = initrd
        candidates = [
            arch.roundup_to_page(start + initrd_size),
            arch.rounddown_to_page(start) - size,
        ]
    return next(
        (gpa for gpa in candidates if region.gpa <= gpa and gpa + size <= region.end),
        None,
    )


def _select(indices: Optional[Sequence[int]], count: int, what: str) -> Sequence[int]:
    """`indices` into a list of `count` items, or all of them if None."""
    if indices is None:
        return range(count)
    bad = [i for i in indices if not 0 <= i < count]
    if bad:
        raise ValueError(
            f"{what} indices {bad} are out of range, it has {count} entries"
        )
    return indices


def _check_room(items: list, limit: int, what: str) -> None:
    if len(items) >= limit:
        raise ValueError(f"libvmm supports at most {limit} {what}")


def _overlaps(a_start: int, a_end: int, b_start: int, b_end: int) -> bool:
    return a_start < b_end and b_start < a_end
