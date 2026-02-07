/*
 * Copyright 2025, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <microkit.h>
#include <libvmm/util/util.h>
#include <libvmm/arch/x86_64/apic.h>
#include <libvmm/arch/x86_64/fault.h>
#include <libvmm/arch/x86_64/vcpu.h>
#include <libvmm/arch/x86_64/vmcs.h>
#include <libvmm/arch/x86_64/virq.h>
#include <libvmm/arch/x86_64/util.h>
#include <libvmm/guest.h>
#include <sel4/arch/vmenter.h>
#include <sddf/util/util.h>
#include <sddf/timer/client.h>

// @billn the TPR register must be properly implemeted, as I've seen a concrete example of a guest using it
// to temporarily masl the APIC timer interrupts: https://github.com/tianocore/edk2/blob/dddb45306affe61328d126ab2cac8de3da909ebc/MdeModulePkg/Bus/Pci/PciBusDxe/PciEnumeratorSupport.c#L890

// @billn there seems to be a big problem with this code. If the host CPU load is high, the fault
// and IRQ injection latency will also be high. Which will cause the timer value to skew and linux
// will complain that TSC isn't stable. I don't know if this is a problem with the code or just how it is...

// https://wiki.osdev.org/APIC

/* Uncomment this to enable debug logging */
// #define DEBUG_APIC

#if defined(DEBUG_APIC)
#define LOG_APIC(...) do{ printf("%s|APIC: ", microkit_name); printf(__VA_ARGS__); }while(0)
#else
#define LOG_APIC(...) do{}while(0)
#endif

// Table 11-1. Local APIC Register Address Map
#define REG_LAPIC_ID 0x20
#define REG_LAPIC_REV 0x30
#define REG_LAPIC_TPR 0x80
#define REG_LAPIC_APR 0x90
#define REG_LAPIC_PPR 0xa0
#define REG_LAPIC_EOI 0xb0
#define REG_LAPIC_LDR 0xd0
#define REG_LAPIC_DFR 0xe0
#define REG_LAPIC_SVR 0xf0
// @billn make this less hardcodey
#define REG_LAPIC_ISR_0 0x100
#define REG_LAPIC_ISR_1 0x110
#define REG_LAPIC_ISR_2 0x120
#define REG_LAPIC_ISR_3 0x130
#define REG_LAPIC_ISR_4 0x140
#define REG_LAPIC_ISR_5 0x150
#define REG_LAPIC_ISR_6 0x160
#define REG_LAPIC_ISR_7 0x170
#define REG_LAPIC_TMR_0 0x180
#define REG_LAPIC_TMR_1 0x190
#define REG_LAPIC_TMR_2 0x1A0
#define REG_LAPIC_TMR_3 0x1B0
#define REG_LAPIC_TMR_4 0x1C0
#define REG_LAPIC_TMR_5 0x1D0
#define REG_LAPIC_TMR_6 0x1E0
#define REG_LAPIC_TMR_7 0x1F0
#define REG_LAPIC_IRR_0 0x200
#define REG_LAPIC_IRR_1 0x210
#define REG_LAPIC_IRR_2 0x220
#define REG_LAPIC_IRR_3 0x230
#define REG_LAPIC_IRR_4 0x240
#define REG_LAPIC_IRR_5 0x250
#define REG_LAPIC_IRR_6 0x260
#define REG_LAPIC_IRR_7 0x270
#define REG_LAPIC_ESR 0x280
#define REG_LAPIC_CMCI 0x2f0
#define REG_LAPIC_ICR_LOW 0x300
#define REG_LAPIC_ICR_HIGH 0x310
#define REG_LAPIC_TIMER 0x320
#define REG_LAPIC_THERMAL 0x330
#define REG_LAPIC_PERF_MON_CNTER 0x340
#define REG_LAPIC_LVT_LINT0 0x350
#define REG_LAPIC_LVT_LINT1 0x360
#define REG_LAPIC_LVT_ERR 0x370
#define REG_LAPIC_INIT_CNT 0x380
#define REG_LAPIC_CURR_CNT 0x390
#define REG_LAPIC_DCR 0x3e0

#define REG_IOAPIC_IOREGSEL_MMIO_OFF 0x0
#define REG_IOAPIC_IOWIN_MMIO_OFF 0x10
#define REG_IOAPIC_IOAPICID_REG_OFF 0x0
#define REG_IOAPIC_IOAPICVER_REG_OFF 0x1
#define REG_IOAPIC_IOAPICARB_REG_OFF 0x2
#define REG_IOAPIC_IOREDTBL_FIRST_OFF 0x10
#define REG_IOAPIC_IOREDTBL_LAST_OFF 0x40

extern struct lapic_regs lapic_regs;
// https://pdos.csail.mit.edu/6.828/2016/readings/ia32/ioapic.pdf
extern struct ioapic_regs ioapic_regs;
extern uint64_t tsc_hz;

static uint64_t ticks_to_ns(uint64_t hz, uint64_t ticks)
{
    __uint128_t tmp = (__uint128_t)ticks * (uint64_t)NS_IN_S;
    tmp += hz / 2;
    return (uint64_t)(tmp / hz);
}

static int n_irq_in_service(void)
{
    int n = 0;
    for (int i = LAPIC_NUM_ISR_IRR_TMR_32B - 1; i >= 0; i--) {
        for (int j = 31; j >= 0; j--) {
            if (lapic_regs.isr[i] & BIT(j)) {
                n += 1;
            }
        }
    }

    // @billn sus, multiple IRQs can be in service, but this seems to work for now
    assert(n <= 1);
    return n;
}

static int get_next_queued_irq_vector(void)
{
    // scan IRRs for *a* pending interrupt
    // do it "right-to-left" as the higer vector is higher prio (generally)
    for (int i = LAPIC_NUM_ISR_IRR_TMR_32B - 1; i >= 0; i--) {
        for (int j = 31; j >= 0; j--) {
            if (lapic_regs.irr[i] & BIT(j)) {
                return i * 32 + j;
            }
        }
    }
    return -1;
}

static void debug_print_lapic_pending_irqs(void)
{
    for (int i = LAPIC_NUM_ISR_IRR_TMR_32B - 1; i >= 0; i--) {
        for (int j = 31; j >= 0; j--) {
            if (lapic_regs.irr[i] & BIT(j)) {
                LOG_VMM("irq vector %d is pending\n", i * 32 + j);
            }
        }
    }
}

bool vcpu_can_take_irq(size_t vcpu_id)
{
    // Not executing anything that blocks IRQs
    if (microkit_vcpu_x86_read_vmcs(vcpu_id, VMX_GUEST_INTERRUPTABILITY) != 0) {
        return false;
    }
    // IRQ on in cpu register
    if (!(microkit_vcpu_x86_read_vmcs(vcpu_id, VMX_GUEST_RFLAGS) & BIT(9))) {
        return false;
    }
    return true;
}

int lapic_dcr_to_divider(void)
{
    // Figure 11-10. Divide Configuration Register
    switch (lapic_regs.dcr) {
    case 0:
        return 2;
    case 1:
        return 4;
    case 2:
        return 8;
    case 3:
        return 16;
    case 8:
        return 32;
    case 9:
        return 64;
    case 10:
        return 128;
    case 11:
        return 1;
    default:
        LOG_VMM_ERR("unknown LAPIC DCR register encoding: 0x%x\n", lapic_regs.dcr);
        assert(false);
    }

    return -1;
}

uint8_t ioapic_pin_to_vector(int ioapic, int pin)
{
    assert(ioapic == 0);
    return ioapic_regs.ioredtbl[pin] & 0xff;
}

enum lapic_timer_mode {
    LAPIC_TIMER_ONESHOT,
    LAPIC_TIMER_PERIODIC,
    LAPIC_TIMER_TSC_DEADLINE,
};

enum lapic_timer_mode lapic_parse_timer_reg(void)
{
    // Figure 11-8. Local Vector Table (LVT)
    switch (((lapic_regs.timer >> 17) & 0x3)) {
    case 0:
        return LAPIC_TIMER_ONESHOT;
    case 1:
        return LAPIC_TIMER_PERIODIC;
    case 2:
        // not advertised in cpuid, unreachable!
        assert(false);
        return LAPIC_TIMER_TSC_DEADLINE;
    default:
        LOG_VMM_ERR("unknown LAPIC timer mode register encoding: 0x%x\n", lapic_regs.timer);
        assert(false);
    }

    return -1;
}

uint64_t tsc_now_scaled(void)
{
    return rdtsc() / lapic_dcr_to_divider();
}

bool lapic_fault_handle(seL4_VCPUContext *vctx, uint64_t offset, seL4_Word qualification,
                        decoded_instruction_ret_t decoded_ins)
{
    // TODO: support other alignments?
    assert(mem_access_width_to_bytes(decoded_ins) == 4);
    assert(offset % 4 == 0);

    if (offset != REG_LAPIC_EOI) {
        // Prevent floods of End of Interrupt prints
        if (ept_fault_is_read(qualification)) {
            LOG_APIC("handling LAPIC read at offset 0x%lx\n", offset);
        } else if (ept_fault_is_write(qualification)) {
            uint64_t data;
            assert(mem_write_get_data(decoded_ins, qualification, vctx, &data));
            LOG_APIC("handling LAPIC write at offset 0x%lx, value 0x%lx\n", offset, data);
        }
    }

    if (ept_fault_is_read(qualification)) {
        uint64_t data;
        switch (offset) {
        case REG_LAPIC_ID:
            data = lapic_regs.id;
            break;
        case REG_LAPIC_REV:
            data = lapic_regs.revision;
            break;
        case REG_LAPIC_TPR:
            data = lapic_regs.tpr;
            break;
        case REG_LAPIC_APR:
            data = lapic_regs.apr;
            break;
        case REG_LAPIC_PPR:
            data = lapic_regs.ppr;
            break;
        case REG_LAPIC_DFR:
            data = lapic_regs.dfr;
            break;
        case REG_LAPIC_LDR:
            data = lapic_regs.ldr;
            break;
        case REG_LAPIC_SVR:
            data = lapic_regs.svr;
            break;
        case REG_LAPIC_IRR_0:
            data = lapic_regs.irr[0];
            break;
        case REG_LAPIC_IRR_1:
            data = lapic_regs.irr[1];
            break;
        case REG_LAPIC_IRR_2:
            data = lapic_regs.irr[2];
            break;
        case REG_LAPIC_IRR_3:
            data = lapic_regs.irr[3];
            break;
        case REG_LAPIC_IRR_4:
            data = lapic_regs.irr[4];
            break;
        case REG_LAPIC_IRR_5:
            data = lapic_regs.irr[5];
            break;
        case REG_LAPIC_IRR_6:
            data = lapic_regs.irr[6];
            break;
        case REG_LAPIC_IRR_7:
            data = lapic_regs.irr[7];
            break;
        case REG_LAPIC_ISR_0:
            data = lapic_regs.isr[0];
            break;
        case REG_LAPIC_ISR_1:
            data = lapic_regs.isr[1];
            break;
        case REG_LAPIC_ISR_2:
            data = lapic_regs.isr[2];
            break;
        case REG_LAPIC_ISR_3:
            data = lapic_regs.isr[3];
            break;
        case REG_LAPIC_ISR_4:
            data = lapic_regs.isr[4];
            break;
        case REG_LAPIC_ISR_5:
            data = lapic_regs.isr[5];
            break;
        case REG_LAPIC_ISR_6:
            data = lapic_regs.isr[6];
            break;
        case REG_LAPIC_ISR_7:
            data = lapic_regs.isr[7];
            break;
        case REG_LAPIC_TMR_0:
            data = lapic_regs.tmr[0];
            break;
        case REG_LAPIC_TMR_1:
            data = lapic_regs.tmr[1];
            break;
        case REG_LAPIC_TMR_2:
            data = lapic_regs.tmr[2];
            break;
        case REG_LAPIC_TMR_3:
            data = lapic_regs.tmr[3];
            break;
        case REG_LAPIC_TMR_4:
            data = lapic_regs.tmr[4];
            break;
        case REG_LAPIC_TMR_5:
            data = lapic_regs.tmr[5];
            break;
        case REG_LAPIC_TMR_6:
            data = lapic_regs.tmr[6];
            break;
        case REG_LAPIC_TMR_7:
            data = lapic_regs.tmr[7];
            break;
        case REG_LAPIC_TIMER:
            data = lapic_regs.timer;
            break;
        case REG_LAPIC_LVT_ERR:
        case REG_LAPIC_CMCI:
        case REG_LAPIC_THERMAL:
            data = 0;
            break;
        case REG_LAPIC_INIT_CNT:
            data = lapic_regs.init_count;
            break;
        case REG_LAPIC_CURR_CNT:
            if (lapic_regs.init_count == 0) {
                data = 0;
            } else {
                uint64_t tsc_tick_now_scaled = tsc_now_scaled();
                uint64_t elapsed_scaled_tsc_tick = tsc_tick_now_scaled - lapic_regs.native_scaled_tsc_when_timer_starts;

                uint64_t remaining = 0;
                if (elapsed_scaled_tsc_tick < lapic_regs.init_count) {
                    remaining = lapic_regs.init_count - elapsed_scaled_tsc_tick;
                }
                data = remaining;
                LOG_APIC("current count read 0x%lx\n", remaining);
            }

            break;
        case REG_LAPIC_ESR:
        case REG_LAPIC_PERF_MON_CNTER:
            data = 0;
            break;
        case REG_LAPIC_ICR_LOW:
            data = lapic_regs.icr_low;
            break;
        case REG_LAPIC_ICR_HIGH:
            data = lapic_regs.icr_high;
            break;
        case REG_LAPIC_LVT_LINT0:
            data = lapic_regs.lint0;
            break;
        case REG_LAPIC_LVT_LINT1:
            data = lapic_regs.lint1;
            break;
        case REG_LAPIC_DCR:
            data = lapic_regs.dcr;
            break;
        default:
            LOG_VMM_ERR("Reading unknown LAPIC register offset 0x%x\n", offset);
            return false;
        }
        assert(mem_read_set_data(decoded_ins, qualification, vctx, data));
    } else {
        uint64_t data;
        assert(mem_write_get_data(decoded_ins, qualification, vctx, &data));
        switch (offset) {
        case REG_LAPIC_TPR:
            // LOG_VMM("TPR write via LAPIC 0x%x\n", data);
            lapic_regs.tpr = data;
            break;
        case REG_LAPIC_EOI:
            // @billn really sus, need to check what is the last in service interrupt and only clear that.
            // LOG_VMM("lapic EOI\n");

            // @billn sus: improve this to allow multiple IRQs in service and also check the TPR
            // @billn sus: there can only be 1 interrupt in service at any given time.
            if (n_irq_in_service() == 1) {
                lapic_regs.isr[0] = 0;
                lapic_regs.isr[1] = 0;
                lapic_regs.isr[2] = 0;
                lapic_regs.isr[3] = 0;
                lapic_regs.isr[4] = 0;
                lapic_regs.isr[5] = 0;
                lapic_regs.isr[6] = 0;
                lapic_regs.isr[7] = 0;

                // if it is a passed through I/O APIC IRQ, run the ack function
                for (int i = 0; i < IOAPIC_NUM_PINS; i++) {
                    if (ioapic_regs.virq_passthrough_map[i].valid) {
                        uint8_t candidate_vector = ioapic_pin_to_vector(0, i);
                        if (candidate_vector == lapic_regs.last_injected_vector) {
                            if (ioapic_regs.virq_passthrough_map[i].ack_fn) {
                                ioapic_regs.virq_passthrough_map[i].ack_fn(
                                    0, i, ioapic_regs.virq_passthrough_map[i].ack_data);
                            }
                            break;
                        }
                    }
                }

                int next_irq_vector = get_next_queued_irq_vector();
                if (next_irq_vector != -1) {
                    inject_lapic_irq(GUEST_BOOT_VCPU_ID, next_irq_vector);
                }
            }

            break;
        case REG_LAPIC_LDR:
            lapic_regs.ldr = data;
            break;
        case REG_LAPIC_DFR:
            lapic_regs.dfr = data;
            break;
        case REG_LAPIC_SVR:
            lapic_regs.svr = data;
            break;
        case REG_LAPIC_LVT_LINT0:
            lapic_regs.lint0 = data;
            break;
        case REG_LAPIC_LVT_LINT1:
            lapic_regs.lint1 = data;
            break;
        case REG_LAPIC_ICR_LOW:
            // Figure 11-12. Interrupt Command Register (ICR)
            // 11-20 Vol. 3A: "The act of writing to the low doubleword of the ICR causes the IPI to be sent."
            lapic_regs.icr_low = data;

            uint64_t icr = lapic_regs.icr_low | (((uint64_t)lapic_regs.icr_high) << 32);

            // @billn sus, handle other types of IPIs
            uint8_t delivery_mode = ((icr >> 8) & 0x7);
            uint8_t destination = (icr >> 56) & 0xff;
            if (delivery_mode == 0) {
                // fixed mode
                assert(destination == 0);
                uint8_t vector = icr & 0xff;
                if (!inject_lapic_irq(GUEST_BOOT_VCPU_ID, vector)) {
                    LOG_VMM_ERR("failed to send IPI\n");
                    return false;
                }
            } else {
                LOG_VMM_ERR("LAPIC received requuest to send IPI of unknown delivery mode 0x%x, destination 0x%x\n",
                            delivery_mode, destination);
            }

            LOG_VMM("icr write 0x%lx\n", icr);
            break;
        case REG_LAPIC_ICR_HIGH:
            lapic_regs.icr_high = data;
            break;
        case REG_LAPIC_TIMER:
            lapic_regs.timer = data;
            // @billn make sure the guest isn't using teh TSC deadline mode, which isn't implemented right now
            assert(((lapic_regs.timer >> 17) & 0x3) != 2);

            // LOG_APIC("*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_\n");
            // vcpu_print_regs(0);
            LOG_APIC("LAPIC timer config write 0x%lx\n", lapic_regs.timer);

            break;
        case REG_LAPIC_ESR:
        case REG_LAPIC_THERMAL:
        case REG_LAPIC_LVT_ERR:
        case REG_LAPIC_PERF_MON_CNTER:
            // lapic_regs.esr = 0;
            break;
        case REG_LAPIC_INIT_CNT:
            // Figure 11-8. Local Vector Table (LVT)

            LOG_APIC("*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_*_\n");
            // vcpu_print_regs(0);

            lapic_regs.init_count = data;
            if (data > 0) {
                LOG_APIC("LAPIC timer started, mode 0x%x, irq masked %d\n", (lapic_regs.timer >> 17) % 0x3,
                         !!(lapic_regs.timer & BIT(16)));

                uint64_t delay_ns = ticks_to_ns(tsc_hz, lapic_regs.init_count * lapic_dcr_to_divider());
                LOG_APIC("setting timeout for 0x%lx ns, from init count 0x%lx\n", delay_ns, lapic_regs.init_count);
                sddf_timer_set_timeout(TIMER_DRV_CH_FOR_LAPIC, delay_ns);

                lapic_regs.native_scaled_tsc_when_timer_starts = tsc_now_scaled();
            }
            break;
        case REG_LAPIC_DCR:
            lapic_regs.dcr = data;
            // LOG_VMM("REG_LAPIC_DCR = 0x%x\n", lapic_regs.dcr);
            break;
        default:
            LOG_VMM_ERR("Writing unknown LAPIC register offset 0x%x, value 0x%lx\n", offset, data);
            return false;
        }
    }

    return true;
}

bool ioapic_fault_handle(seL4_VCPUContext *vctx, uint64_t offset, seL4_Word qualification,
                         decoded_instruction_ret_t decoded_ins)
{
    if (ept_fault_is_read(qualification)) {
        LOG_APIC("handling I/O APIC read at MMIO offset 0x%lx\n", offset);
    } else if (ept_fault_is_write(qualification)) {
        LOG_APIC("handling I/O APIC write at MMIO offset 0x%lx\n", offset);
    }

    if (ept_fault_is_read(qualification)) {
        if (offset == REG_IOAPIC_IOWIN_MMIO_OFF) {
            uint64_t data;

            if (ioapic_regs.selected_reg == REG_IOAPIC_IOAPICID_REG_OFF) {
                data = ioapic_regs.ioapicid;
            } else if (ioapic_regs.selected_reg == REG_IOAPIC_IOAPICVER_REG_OFF) {
                data = ioapic_regs.ioapicver;
            } else if (ioapic_regs.selected_reg == REG_IOAPIC_IOAPICARB_REG_OFF) {
                data = ioapic_regs.ioapicarb;
            } else if (ioapic_regs.selected_reg >= REG_IOAPIC_IOREDTBL_FIRST_OFF
                       && ioapic_regs.selected_reg <= REG_IOAPIC_IOREDTBL_LAST_OFF) {
                int redirection_reg_idx = ((ioapic_regs.selected_reg - REG_IOAPIC_IOREDTBL_FIRST_OFF) & ~((uint64_t)1))
                                        / 2;
                int is_high = ioapic_regs.selected_reg & 0x1;

                // LOG_VMM("reading indirect register 0x%x, is high %d\n", redirection_reg_idx, is_high);

                if (is_high) {
                    data = ioapic_regs.ioredtbl[redirection_reg_idx] >> 32;
                } else {
                    data = ioapic_regs.ioredtbl[redirection_reg_idx] & 0xffffffff;
                }

            } else {
                LOG_VMM_ERR("Reading unknown I/O APIC register offset 0x%x\n", ioapic_regs.selected_reg);
                return false;
            }

            assert(mem_read_set_data(decoded_ins, qualification, vctx, data));
        } else {
            LOG_VMM_ERR("Reading unknown I/O APIC MMIO register 0x%x\n", offset);
            return false;
        }
    } else {
        uint64_t data;
        assert(mem_write_get_data(decoded_ins, qualification, vctx, &data));

        if (offset == REG_IOAPIC_IOREGSEL_MMIO_OFF) {
            ioapic_regs.selected_reg = data & 0xff;
            // LOG_VMM("selecting I/O APIC register 0x%x for write\n", ioapic_regs.selected_reg);

        } else if (offset == REG_IOAPIC_IOWIN_MMIO_OFF) {
            if (ioapic_regs.selected_reg == REG_IOAPIC_IOAPICID_REG_OFF) {
                LOG_APIC("Written to I/O APIC ID register: 0x%lx\n", data);
            } else if (ioapic_regs.selected_reg == REG_IOAPIC_IOAPICVER_REG_OFF) {
                LOG_APIC("Written to I/O APIC VER register: 0x%lx\n", data);
            } else if (ioapic_regs.selected_reg >= REG_IOAPIC_IOREDTBL_FIRST_OFF
                       && ioapic_regs.selected_reg <= REG_IOAPIC_IOREDTBL_LAST_OFF) {
                int redirection_reg_idx = ((ioapic_regs.selected_reg - REG_IOAPIC_IOREDTBL_FIRST_OFF) & ~((uint64_t)1))
                                        / 2;
                int is_high = ioapic_regs.selected_reg & 0x1;

                uint64_t old_reg = ioapic_regs.ioredtbl[redirection_reg_idx];

                if (is_high) {
                    uint64_t new_high = data << 32;
                    uint64_t low = ioapic_regs.ioredtbl[redirection_reg_idx] & 0xffffffff;
                    ioapic_regs.ioredtbl[redirection_reg_idx] = low | new_high;
                } else {
                    uint64_t high = (ioapic_regs.ioredtbl[redirection_reg_idx] >> 32) << 32;
                    uint64_t new_low = data & 0xffffffff;
                    ioapic_regs.ioredtbl[redirection_reg_idx] = new_low | high;
                }

                uint64_t new_reg = ioapic_regs.ioredtbl[redirection_reg_idx];

                // If an I/O APIC IRQ pin goes from masked to unmasked and there are passed through
                // IRQ on that pin, ack it so that if HW triggered an IRQ before the guest unmask the line
                // in the virtual I/O APIC, the real IRQ doesn't get stuck in waiting for ACK.
                if ((old_reg & BIT(16)) && !(new_reg & BIT(16))) {
                    for (int i = 0; i < IOAPIC_NUM_PINS; i++) {
                        if (i == redirection_reg_idx) {
                            if (ioapic_regs.virq_passthrough_map[i].ack_fn) {
                                ioapic_regs.virq_passthrough_map[i].ack_fn(
                                    0, i, ioapic_regs.virq_passthrough_map[i].ack_data);
                            }
                            break;
                        }
                    }
                }

            } else {
                LOG_VMM_ERR("Writing unknown I/O APIC register offset 0x%x\n", ioapic_regs.selected_reg);
                return false;
            }
        } else {
            LOG_VMM_ERR("Writing unknown I/O APIC MMIO register 0x%x\n", offset);
            return false;
        }
    }

    return true;
}

// When lapic_maintenance() is called, the vCPU *must* be ready to take the interrupt.
void lapic_maintenance(void)
{
    int vector = get_next_queued_irq_vector();
    if (vector == -1) {
        // no pending IRQ to inject
        return;
    }

    int irr_n = vector / 32;
    int irr_idx = vector % 32;

    if (lapic_regs.isr[irr_n] & BIT(irr_idx)) {
        // interrupt already in service, remains queued until guest issues EOI.
        return;
    }

    // Move IRQ from pending to in-service
    lapic_regs.irr[irr_n] &= ~BIT(irr_idx);
    lapic_regs.isr[irr_n] |= BIT(irr_idx);

    // Make absolutely sure that the vCPU can take the interrupt
    assert(microkit_vcpu_x86_read_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_INTERRUPTABILITY) == 0);
    assert(microkit_vcpu_x86_read_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_RFLAGS) & BIT(9));

    // Tell the hardware to inject an irq on next vmenter.
    // Table 25-17. Format of the VM-Entry Interruption-Information Field
    uint32_t vm_entry_interruption = (uint32_t)vector;
    vm_entry_interruption |= BIT(31); // valid bit

    microkit_mr_set(SEL4_VMENTER_CALL_CONTROL_ENTRY_MR, vm_entry_interruption);
    // Clear interrupt window exiting bit if set.
    microkit_mr_set(SEL4_VMENTER_CALL_CONTROL_PPC_MR, VMCS_PCC_DEFAULT);

    lapic_regs.last_injected_vector = vector;
}

bool inject_lapic_irq(size_t vcpu_id, uint8_t vector)
{
    assert(vcpu_id == 0);

    if (vector < 32) {
        LOG_VMM_ERR("IRQ Vector %d is archtecturally reserved! Will not inject.\n", vector);
        return false;
    }

    if (!(lapic_regs.svr & BIT(8))) {
        // APIC software disable
        return false;
    }

    int irr_n = vector / 32;
    int irr_idx = vector % 32;

    // Mark as pending for injection, there may be 3 scenarios that play out:
    // 1. IRQ already pending, in that case the 2 IRQs get collapsed into 1
    // 1. immediately if the vCPU can take it and LAPIC have no in service IRQ, @billn the second part is a bit sus
    // 2. at some point in the future when the vCPU re-enable IRQs
    lapic_regs.irr[irr_n] |= BIT(irr_idx);

    if (vcpu_can_take_irq(vcpu_id) && n_irq_in_service() == 0) {
        lapic_maintenance();
    } else {
        microkit_mr_set(SEL4_VMENTER_CALL_CONTROL_PPC_MR, VMCS_PCC_EXIT_IRQ_WINDOW);
    }
    return true;
}

bool handle_lapic_timer_nftn(size_t vcpu_id)
{
    if (!(lapic_regs.svr & BIT(8))) {
        // APIC software disable
        return true;
    }

    // restart timeout if periodic
    if (lapic_parse_timer_reg() == LAPIC_TIMER_PERIODIC && lapic_regs.init_count > 0) {
        lapic_regs.native_scaled_tsc_when_timer_starts = tsc_now_scaled();
        // sddf_timer_set_timeout(TIMER_DRV_CH_FOR_LAPIC, tsc_ticks_to_ns(lapic_timer_hz, lapic_regs.init_count));
        uint64_t delay_ns = ticks_to_ns(tsc_hz, lapic_regs.init_count * lapic_dcr_to_divider());
        LOG_APIC("restarting periodic timeout for 0x%lx ns\n", delay_ns);
        sddf_timer_set_timeout(TIMER_DRV_CH_FOR_LAPIC, delay_ns);
    }

    // but only inject IRQ if it is not masked
    if (!(lapic_regs.timer & BIT(16))) {
        uint8_t vector = lapic_regs.timer & 0xff;
        if (!inject_lapic_irq(vcpu_id, vector)) {
            // LOG_VMM_ERR("failed to inject LAPIC timer IRQ vector 0x%x\n", vector);
            return false;
        }
    }

    return true;
}

bool inject_ioapic_irq(int ioapic, int pin)
{
    // only 1 chip right now, which is a direct map to the dual 8259
    assert(ioapic == 0);

    if (pin >= IOAPIC_LAST_INDIRECT_INDEX) {
        LOG_VMM_ERR("trying to inject IRQ to out of bound I/O APIC pin %d\n", pin);
        return false;
    }

    // check if the irq is masked
    if (ioapic_regs.ioredtbl[pin] & BIT(16)) {
        return false;
    }

    // @billn need to read cpu id from redirection register
    return inject_lapic_irq(0, ioapic_pin_to_vector(ioapic, pin));
}
