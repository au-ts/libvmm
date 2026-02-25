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

// @billn there seems to be a big problem with this code. If the host CPU load is high, the fault
// and IRQ injection latency will also be high. Which will cause the timer value to skew and linux
// will complain that TSC isn't stable. I don't know if this is a problem with the code or just how it is...

// https://wiki.osdev.org/APIC

/* Uncomment this to enable debug logging */
// #define DEBUG_APIC

extern bool fault_cond;

#if defined(DEBUG_APIC)
#define LOG_APIC(...) do{ printf("%s|APIC: ", microkit_name); printf(__VA_ARGS__); }while(0)
#else
#define LOG_APIC(...) do{}while(0)
#endif

#define REG_IOAPIC_IOREGSEL_MMIO_OFF 0x0
#define REG_IOAPIC_IOWIN_MMIO_OFF 0x10
#define REG_IOAPIC_IOAPICID_REG_OFF 0x0
#define REG_IOAPIC_IOAPICVER_REG_OFF 0x1
#define REG_IOAPIC_IOAPICARB_REG_OFF 0x2
#define REG_IOAPIC_IOREDTBL_FIRST_OFF 0x10
#define REG_IOAPIC_IOREDTBL_LAST_OFF 0x40

// https://pdos.csail.mit.edu/6.828/2016/readings/ia32/ioapic.pdf
extern uintptr_t vapic_vaddr;
extern struct ioapic_regs ioapic_regs;
extern uint64_t tsc_hz;

// @billn revisit for multiple vcpus
uint64_t native_scaled_tsc_when_timer_starts;

static uint64_t ticks_to_ns(uint64_t hz, uint64_t ticks)
{
    __uint128_t tmp = (__uint128_t)ticks * (uint64_t)NS_IN_S;
    tmp += hz / 2;
    return (uint64_t)(tmp / hz);
}

static int get_next_pending_irq_vector(void)
{
    // scan IRRs for *a* pending interrupt
    // do it "right-to-left" as the higer vector is higher prio
    for (int i = LAPIC_NUM_ISR_IRR_TMR_32B - 1; i >= 0; i--) {
        for (int j = 31; j >= 0; j--) {
            int irr_reg_off = REG_LAPIC_IRR_0 + (i * 0x10);
            if (vapic_read_reg(irr_reg_off) & BIT(j)) {
                uint8_t candidate_vector = i * 32 + j;
                return candidate_vector;
            }
        }
    }
    return -1;
}

static void debug_print_lapic_pending_irqs(void)
{
    for (int i = LAPIC_NUM_ISR_IRR_TMR_32B - 1; i >= 0; i--) {
        for (int j = 31; j >= 0; j--) {
            uint32_t irr = vapic_read_reg(REG_LAPIC_IRR_0 + (i * 0x10));
            if (irr & BIT(j)) {
                LOG_VMM("irq vector %d is pending\n", i * 32 + j);
            }
        }
    }
}

uint32_t vapic_read_reg(int offset)
{
    return *((uint32_t *)(vapic_vaddr + offset));
}

void vapic_write_reg(int offset, uint32_t value)
{
    volatile uint32_t *reg = (uint32_t *)(vapic_vaddr + offset);
    *reg = value;
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
    switch (vapic_read_reg(REG_LAPIC_DCR)) {
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
        LOG_VMM_ERR("unknown LAPIC DCR register encoding: 0x%x\n", vapic_read_reg(REG_LAPIC_DCR));
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
    uint32_t timer_reg = vapic_read_reg(REG_LAPIC_TIMER);
    switch (((timer_reg >> 17) & 0x3)) {
    case 0:
        return LAPIC_TIMER_ONESHOT;
    case 1:
        return LAPIC_TIMER_PERIODIC;
    case 2:
        // not advertised in cpuid, unreachable!
        assert(false);
        return LAPIC_TIMER_TSC_DEADLINE;
    default:
        LOG_VMM_ERR("unknown LAPIC timer mode register encoding: 0x%x\n", timer_reg);
        assert(false);
    }

    return -1;
}

uint64_t tsc_now_scaled(void)
{
    return rdtsc() / lapic_dcr_to_divider();
}

bool lapic_read_fault_handle(uint64_t offset)
{
    uint32_t result;

    switch (offset) {
    case REG_LAPIC_CURR_CNT: {
        if (vapic_read_reg(REG_LAPIC_INIT_CNT) == 0) {
            result = 0;
        } else {
            uint64_t tsc_tick_now_scaled = tsc_now_scaled();
            uint64_t elapsed_scaled_tsc_tick = tsc_tick_now_scaled - native_scaled_tsc_when_timer_starts;

            uint64_t remaining = 0;
            if (elapsed_scaled_tsc_tick < vapic_read_reg(REG_LAPIC_INIT_CNT)) {
                remaining = vapic_read_reg(REG_LAPIC_INIT_CNT) - elapsed_scaled_tsc_tick;
            }
            result = remaining;
            LOG_APIC("current count read 0x%lx\n", remaining);
        }
        break;
    }
    default:
        LOG_VMM_ERR("Reading unknown LAPIC register offset 0x%x\n", offset);
        return false;
    }

    vapic_write_reg(offset, result);
    return true;
}

bool lapic_write_fault_handle(uint64_t offset)
{
    switch (offset) {

    case REG_LAPIC_SVR:
    case REG_LAPIC_LVT_LINT0:
    case REG_LAPIC_LVT_LINT1:
    case REG_LAPIC_LVT_ERR:
    case REG_LAPIC_ESR:
    case REG_LAPIC_TIMER:
    case REG_LAPIC_DCR:
    case REG_LAPIC_DFR:
    case REG_LAPIC_LDR:
    case REG_LAPIC_THERMAL:
    case REG_LAPIC_PERF_MON_CNTER:
        break;

    case REG_LAPIC_INIT_CNT: {
        // Figure 11-8. Local Vector Table (LVT)
        uint32_t init_count = vapic_read_reg(REG_LAPIC_INIT_CNT);
        if (init_count > 0) {
            uint32_t timer_reg = vapic_read_reg(REG_LAPIC_TIMER);
            (void)timer_reg;
            LOG_APIC("LAPIC timer started, mode 0x%x, irq masked %d\n", (timer_reg >> 17) % 0x3,
                     !!(timer_reg & BIT(16)));

            uint64_t delay_ns = ticks_to_ns(tsc_hz, init_count * lapic_dcr_to_divider());
            LOG_APIC("setting timeout for 0x%lx ns, from init count 0x%lx\n", delay_ns, init_count);
            sddf_timer_set_timeout(TIMER_DRV_CH_FOR_LAPIC, delay_ns);

            native_scaled_tsc_when_timer_starts = tsc_now_scaled();
        }
        break;
    }

    case REG_LAPIC_ICR_LOW: {
        // Figure 11-12. Interrupt Command Register (ICR)
        // 11-20 Vol. 3A: "The act of writing to the low doubleword of the ICR causes the IPI to be sent."
        uint64_t icr = vapic_read_reg(REG_LAPIC_ICR_LOW) | (((uint64_t)vapic_read_reg(REG_LAPIC_ICR_HIGH)) << 32);

        // @billn sus, handle other types of IPIs
        uint8_t delivery_mode = ((icr >> 8) & 0x7);
        uint8_t destination = (icr >> 56) & 0xff;
        if (delivery_mode == 0 || delivery_mode == 5) {
            // fixed mode
            assert(destination == 0);
            uint8_t vector = icr & 0xff;
            if (!inject_lapic_irq(GUEST_BOOT_VCPU_ID, vector)) {
                LOG_VMM_ERR("failed to send IPI\n");
                return false;
            } else {
                LOG_VMM("sent ipi\n");
            }
        } else {
            LOG_VMM_ERR("LAPIC received requuest to send IPI of unknown delivery mode 0x%x, destination 0x%x\n",
                        delivery_mode, destination);
        }

        LOG_VMM("icr write 0x%lx, current TPL is 0x%x\n", icr, vapic_read_reg(REG_LAPIC_TPR));
        break;
    }

    default:
        LOG_VMM_ERR("Writing unknown LAPIC register offset 0x%x\n", offset);
        return false;
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

                LOG_APIC("ioapic pin %d reprogram 0x%lx, masked %d\n", redirection_reg_idx, new_reg,
                         !!(ioapic_regs.ioredtbl[redirection_reg_idx] & BIT(16)));

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

bool inject_lapic_irq(size_t vcpu_id, uint8_t vector)
{
    assert(vcpu_id == 0);

    if (!(vapic_read_reg(REG_LAPIC_SVR) & BIT(8))) {
        // APIC software disable
        return false;
    }

    int irr_n = vector / 32;
    int irr_idx = vector % 32;

    int irr_reg_off = REG_LAPIC_IRR_0 + (irr_n * 0x10);
    assert(irr_reg_off <= REG_LAPIC_IRR_7);

    // Mark as pending for injection, let hardware handle the rest
    vapic_write_reg(irr_reg_off, vapic_read_reg(irr_reg_off) | BIT(irr_idx));

    // page 26-8 Vol. 3C "Guest interrupt status"
    int highest_vector_pending = get_next_pending_irq_vector();
    assert(highest_vector_pending != -1);
    uint16_t guest_irq_status = microkit_vcpu_x86_read_vmcs(0, VMX_GUEST_INTERRUPT_STATUS);
    guest_irq_status &= 0xff00;
    guest_irq_status |= (uint8_t)highest_vector_pending;
    microkit_vcpu_x86_write_vmcs(0, VMX_GUEST_INTERRUPT_STATUS, guest_irq_status);

    return true;
}

bool handle_lapic_timer_nftn(size_t vcpu_id)
{
    if (!(vapic_read_reg(REG_LAPIC_SVR) & BIT(8))) {
        // APIC software disable
        return true;
    }

    // restart timeout if periodic
    uint32_t init_count = vapic_read_reg(REG_LAPIC_INIT_CNT);
    if (lapic_parse_timer_reg() == LAPIC_TIMER_PERIODIC && init_count > 0) {
        native_scaled_tsc_when_timer_starts = tsc_now_scaled();
        uint64_t delay_ns = ticks_to_ns(tsc_hz, init_count * lapic_dcr_to_divider());
        LOG_APIC("restarting periodic timeout for 0x%lx ns\n", delay_ns);
        sddf_timer_set_timeout(TIMER_DRV_CH_FOR_LAPIC, delay_ns);
    }

    // but only inject IRQ if it is not masked
    uint32_t timer_reg = vapic_read_reg(REG_LAPIC_TIMER);
    if (!(timer_reg & BIT(16))) {
        uint8_t vector = timer_reg & 0xff;
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

    // @billn sus
    uint8_t delivery_mode = (ioapic_regs.ioredtbl[pin] >> 8) & 0x7;

    // @billn sus revisit delivery mode 1 for multiple vcpu
    if (delivery_mode != 0 && delivery_mode != 1) {
        LOG_VMM_ERR("unknown I/O APIC delivery mode for injection on pin %d, mode 0x%x\n", pin, delivery_mode);
        assert(false);
    }
    // uint8_t level_trigger = (ioapic_regs.ioredtbl[pin] >> 15) & 0x1;
    // assert(!level_trigger);

    uint8_t vector = ioapic_pin_to_vector(ioapic, pin);

    // For any passed through interrupts:
    // When the guest EOIs the interrupt, we must trigger a vmexit to run the ack func
    if (ioapic_regs.virq_passthrough_map[pin].valid) {

        int eoi_bitmap_n = vector / 64;
        int n_bitmap_i = vector % 64;
        switch (eoi_bitmap_n) {
        case 0: {
            uint64_t bitmap = microkit_vcpu_x86_read_vmcs(GUEST_BOOT_VCPU_ID, VMX_CONTROL_EOI_EXIT_BITMAP_0);
            microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_CONTROL_EOI_EXIT_BITMAP_0, bitmap | BIT(n_bitmap_i));
            break;
        }
        case 1: {
            uint64_t bitmap = microkit_vcpu_x86_read_vmcs(GUEST_BOOT_VCPU_ID, VMX_CONTROL_EOI_EXIT_BITMAP_1);
            microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_CONTROL_EOI_EXIT_BITMAP_1, bitmap | BIT(n_bitmap_i));
            break;
        }
        case 2: {
            uint64_t bitmap = microkit_vcpu_x86_read_vmcs(GUEST_BOOT_VCPU_ID, VMX_CONTROL_EOI_EXIT_BITMAP_2);
            microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_CONTROL_EOI_EXIT_BITMAP_2, bitmap | BIT(n_bitmap_i));
            break;
        }
        case 3: {
            uint64_t bitmap = microkit_vcpu_x86_read_vmcs(GUEST_BOOT_VCPU_ID, VMX_CONTROL_EOI_EXIT_BITMAP_3);
            microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_CONTROL_EOI_EXIT_BITMAP_3, bitmap | BIT(n_bitmap_i));
            break;
        }
        default:
            LOG_VMM_ERR("bug: eoi_bitmap_n > 3\n");
            return false;
        }
    }

    // @billn need to read cpu id from redirection register
    return inject_lapic_irq(0, vector);
}

bool ioapic_ack_passthrough_irq(uint8_t vector)
{
    for (int i = 0; i < IOAPIC_NUM_PINS; i++) {
        if (ioapic_regs.virq_passthrough_map[i].valid) {
            uint8_t candidate_vector = ioapic_pin_to_vector(0, i);
            if (candidate_vector == vector) {
                if (ioapic_regs.virq_passthrough_map[i].ack_fn) {
                    ioapic_regs.virq_passthrough_map[i].ack_fn(0, i, ioapic_regs.virq_passthrough_map[i].ack_data);
                }
                return true;
            }
        }
    }
    return false;
}