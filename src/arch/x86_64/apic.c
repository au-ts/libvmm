/*
 * Copyright 2025, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <microkit.h>
#include <libvmm/util/util.h>
#include <libvmm/arch/x86_64/apic.h>
#include <libvmm/arch/x86_64/guest_time.h>
#include <libvmm/arch/x86_64/fault.h>
#include <libvmm/arch/x86_64/vcpu.h>
#include <libvmm/arch/x86_64/vmcs.h>
#include <libvmm/arch/x86_64/virq.h>
#include <libvmm/arch/x86_64/util.h>
#include <libvmm/guest.h>
#include <sel4/arch/vmenter.h>
#include <sddf/util/util.h>
#include <sddf/timer/client.h>

/* Implementation of a virtual Local APIC and I/O APIC. */

#if APIC_VIRT_LEVEL < APIC_VIRT_LEVEL_APICV
#define LAPIC_REG_BLOCK_SIZE 0x1000
char lapic_regs[LAPIC_REG_BLOCK_SIZE];
#endif

// Uncomment this to enable debug logging
// #define DEBUG_APIC

// @billn there seems to be a big problem with this code. If the host CPU load is high, the fault
// and IRQ injection latency will also be high. Which will cause the timer value to skew and linux
// will complain that TSC isn't stable. I don't know if this is a problem with the code or just how it is...

// @billn the APIC timer doesn't need to be tied to the TSC I think, I did it like this originally because
// I didn't know anything about x86 architecture. But for simplicity it could be tied to the sDDF timer
// instead. The only case where the TSC comes into play is the TSC deadline timer mode, which we haven't implemented

/* Documents referenced:
 * 1. Intel® 64 and IA-32 Architectures Software Developer’s Manual
 *    Combined Volumes: 1, 2A, 2B, 2C, 2D, 3A, 3B, 3C, 3D, and 4
 *    Order Number: 325462-080US June 2023
 * 2. Intel 82093AA I/O ADVANCED PROGRAMMABLE INTERRUPT CONTROLLER (IOAPIC)
 *    Order Number: 290566-001 May 1996
 */

/* [1] "12.5.2 Valid Interrupt Vectors" */
#define MIN_VECTOR 32

#define LAPIC_NUM_ISR_IRR_TMR_32B 8
#define MAX_VECTOR (LAPIC_NUM_ISR_IRR_TMR_32B * 32)
#define IRR_ISR_MULTIPLIER 0x10

#if defined(DEBUG_APIC)
#define LOG_APIC(...) do{ printf("%s|APIC: ", microkit_name); printf(__VA_ARGS__); }while(0)
#else
#define LOG_APIC(...) do{}while(0)
#endif

extern uintptr_t vapic_vaddr;
extern struct ioapic_regs ioapic_regs;
extern guest_t guest;

/* Bookkeeping for the local APIC timer */
// @billn revisit for multiple vcpus
uint64_t native_scaled_apic_ticks_when_timer_starts;
bool timeout_handle_valid;
guest_timeout_handle_t timeout_handle;

uint32_t lapic_read_reg(int offset)
{
    assert(offset < 0x1000);

#if APIC_VIRT_LEVEL < APIC_VIRT_LEVEL_APICV
    uintptr_t lapic_regs_ptr = (uintptr_t)(&lapic_regs);
#else
    // @billn fix hardcoded vaddr
    uintptr_t lapic_regs_ptr = 0x101000ull;
#endif

    return *((volatile uint32_t *)(lapic_regs_ptr + offset));
}

void lapic_write_reg(int offset, uint32_t value)
{
    assert(offset < 0x1000);

#if APIC_VIRT_LEVEL < APIC_VIRT_LEVEL_APICV
    uintptr_t lapic_regs_ptr = (uintptr_t)(&lapic_regs);
#else
    // @billn fix hardcoded vaddr
    uintptr_t lapic_regs_ptr = 0x101000ull;
#endif

    volatile uint32_t *reg = (uint32_t *)(lapic_regs_ptr + offset);
    *reg = value;
}

static int get_next_pending_irq_vector(void)
{
    /* Scans IRRs for *a* pending interrupt,
     * do it "right-to-left" as the higer vector is higher priority.
     */
    for (int i = LAPIC_NUM_ISR_IRR_TMR_32B - 1; i >= 0; i--) {
        for (int j = 31; j >= 0; j--) {
            int irr_reg_off = REG_LAPIC_IRR_0 + (i * IRR_ISR_MULTIPLIER);
            if (lapic_read_reg(irr_reg_off) & BIT(j)) {
                uint8_t candidate_vector = i * 32 + j;
                return candidate_vector;
            }
        }
    }
    return -1;
}

static void mark_irq_pending(uint8_t vector)
{
    int irr_n = vector / 32;
    int irr_idx = vector % 32;

    int irr_reg_off = REG_LAPIC_IRR_0 + (irr_n * IRR_ISR_MULTIPLIER);
    assert(irr_reg_off <= REG_LAPIC_IRR_7);

    lapic_write_reg(irr_reg_off, lapic_read_reg(irr_reg_off) | BIT(irr_idx));
}

static int lapic_dcr_to_divider(void)
{
    /* [1] "Figure 12-10. Divide Configuration Register" */
    switch (lapic_read_reg(REG_LAPIC_DCR)) {
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
        LOG_VMM_ERR("unknown LAPIC DCR register encoding: 0x%x\n", lapic_read_reg(REG_LAPIC_DCR));
        assert(false);
    }

    return -1;
}

static uint8_t ioapic_pin_to_vector(int ioapic, int pin)
{
    assert(ioapic == 0);
    return ioapic_regs.ioredtbl[pin] & 0xff;
}

#if APIC_VIRT_LEVEL < APIC_VIRT_LEVEL_APICV
static void mark_irq_in_service(uint8_t vector)
{
    int irr_n = vector / 32;
    int irr_idx = vector % 32;

    int irr_reg_off = REG_LAPIC_IRR_0 + (irr_n * IRR_ISR_MULTIPLIER);
    assert(irr_reg_off <= REG_LAPIC_IRR_7);

    /* Should have been pending! */
    assert(lapic_read_reg(irr_reg_off) | BIT(irr_idx));

    /* Clear pending bit */
    lapic_write_reg(irr_reg_off, lapic_read_reg(irr_reg_off) & ~BIT(irr_idx));

    /* Set in service bit */
    int isr_reg_off = REG_LAPIC_ISR_0 + (irr_n * IRR_ISR_MULTIPLIER);
    lapic_write_reg(isr_reg_off, lapic_read_reg(isr_reg_off) | BIT(irr_idx));
}

static bool is_irq_in_service(uint8_t vector)
{
    int isr_n = vector / 32;
    int isr_idx = vector % 32;

    int isr_reg_off = REG_LAPIC_ISR_0 + (isr_n * IRR_ISR_MULTIPLIER);
    assert(isr_reg_off <= REG_LAPIC_ISR_7);

    return !!(lapic_read_reg(isr_reg_off) & BIT(isr_idx));
}

static bool get_highest_vector_in_service(uint8_t *ret)
{
    for (int i = LAPIC_NUM_ISR_IRR_TMR_32B - 1; i >= 0; i--) {
        for (int j = 31; j >= 0; j--) {
            int isr_reg_off = REG_LAPIC_ISR_0 + (i * IRR_ISR_MULTIPLIER);
            if (lapic_read_reg(isr_reg_off) & BIT(j)) {
                *ret = i * 32 + j;
                return true;
            }
        }
    }
    return false;
}

static int get_next_eligible_irq_vector(void)
{
    // uint8_t ppr = get_ppr();

    // scan IRRs for *a* pending interrupt
    // do it "right-to-left" as the higer vector is higher prio (generally)
    for (int i = LAPIC_NUM_ISR_IRR_TMR_32B - 1; i >= 0; i--) {
        for (int j = 31; j >= 0; j--) {
            int irr_reg_off = REG_LAPIC_IRR_0 + (i * IRR_ISR_MULTIPLIER);
            if (lapic_read_reg(irr_reg_off) & BIT(j)) {
                uint8_t candidate_vector = i * 32 + j;
                // @billn highly sus, revisit PPR calculation
                if (candidate_vector >> 4 > lapic_read_reg(REG_LAPIC_TPR) >> 4) {
                    return candidate_vector;
                } else {
                    // highest pending vector is lower than highest eligible priority, so do nothing
                    return -1;
                }
            }
        }
    }
    return -1;
}

static uint8_t get_ppr(void)
{
    uint8_t ppr;

    /* [1] "Figure 12-19. Processor-Priority Register (PPR)" */
    uint8_t highest_vector_in_service = 0;
    get_highest_vector_in_service(&highest_vector_in_service);
    uint8_t highest_priority_in_service = highest_vector_in_service >> 4;
    uint8_t task_priority_register = lapic_read_reg(REG_LAPIC_TPR);
    uint8_t task_priority_class = task_priority_register >> 4;
    uint8_t task_priority_subclass = task_priority_register & 0xf;
    uint8_t proc_priority_class = MAX(task_priority_class, highest_priority_in_service);
    uint8_t proc_priority_subclass = 0;

    if (task_priority_class >= highest_priority_in_service) {
        proc_priority_subclass = task_priority_subclass;
    }
    ppr = proc_priority_subclass | (proc_priority_class << 4);
    return ppr;
}

static bool vcpu_can_take_irq(size_t vcpu_id)
{
    // Not executing anything that blocks IRQs
    if (vcpu_exit_get_interruptability() != 0) {
        return false;
    }
    // IRQ on in cpu register
    if (!(vcpu_exit_get_rflags() & BIT(9))) {
        return false;
    }
    return true;
}

void lapic_write_tpr(uint8_t tpr)
{
    uint8_t old_tpr = lapic_read_reg(REG_LAPIC_TPR);
    lapic_write_reg(REG_LAPIC_TPR, tpr);
    if (tpr != old_tpr && vcpu_can_take_irq(0)) {
        int vector = get_next_eligible_irq_vector();
        if (vector != -1) {
            assert(inject_lapic_irq(0, vector));
        }
    }
}

/* Inject a highest priority interrupt that is eligible to be delivered.
 * Assumes that the vCPU's architectural state allow injection! */
void lapic_maintenance(void)
{
    int vector = get_next_eligible_irq_vector();
    if (vector == -1) {
        /* no pending IRQ to inject */
        vcpu_exit_update_ppvc(VMCS_PPVC_DEFAULT);
        return;
    }

    if (is_irq_in_service(vector)) {
        /* selected interrupt already in service, remains queued until guest issues EOI
         * so we don't loose it. */
        // @billn sus, might break level trigger
        vcpu_exit_update_ppvc(VMCS_PPVC_DEFAULT);
        return;
    }

    if (vcpu_exit_get_irq() & BIT(31)) {
        /* Another interrupt is awaiting injection, don't do anything for now
         * to prevent interrupt overwriting. */
        return;
    }

    /* Move IRQ from pending to in-service */
    mark_irq_in_service(vector);

    /* Tell the hardware to inject an irq on next vmenter.
     * Table 26-17. Format of the VM-Entry Interruption-Information Field */
    uint32_t vm_entry_interruption = (uint32_t)vector;
    vm_entry_interruption |= BIT(31); // valid bit

    vcpu_exit_update_ppvc(VMCS_PPVC_DEFAULT);
    vcpu_exit_inject_irq(vm_entry_interruption);
}

void lapic_write_eoi(void)
{
    uint8_t done_vector;
    if (!get_highest_vector_in_service(&done_vector)) {
        return;
    }

    /* Clear it from being in service */
    int isr_n = done_vector / 32;
    int isr_idx = done_vector % 32;
    int isr_reg_off = REG_LAPIC_ISR_0 + (isr_n * IRR_ISR_MULTIPLIER);
    lapic_write_reg(isr_reg_off, lapic_read_reg(isr_reg_off) & ~BIT(isr_idx));

    /* if it is a passed through I/O APIC IRQ, run the ack function */
    for (int i = 0; i < IOAPIC_NUM_PINS; i++) {
        if (ioapic_regs.virq_passthrough_map[i].valid) {
            uint8_t candidate_vector = ioapic_pin_to_vector(0, i);
            if (candidate_vector == done_vector) {
                if (ioapic_regs.virq_passthrough_map[i].ack_fn) {
                    ioapic_regs.virq_passthrough_map[i].ack_fn(0, i, ioapic_regs.virq_passthrough_map[i].ack_data);
                }
                break;
            }
        }
    }

    int next_irq_vector = get_next_eligible_irq_vector();
    if (next_irq_vector != -1) {
        inject_lapic_irq(GUEST_BOOT_VCPU_ID, next_irq_vector);
    }
}

bool lapic_fault_handle(seL4_VCPUContext *vctx, uint64_t offset, seL4_Word qualification,
                        decoded_instruction_ret_t decoded_ins)
{
    assert(mem_access_width_to_bytes(decoded_ins) == 4);
    assert(offset % 4 == 0);

    bool success;

    if (ept_fault_is_read(qualification)) {
        uint32_t data;
        success = lapic_read_fault_handle(offset, &data);
        if (success) {
            assert(mem_read_set_data(decoded_ins, qualification, vctx, data));
        }
    } else {
        uint64_t data;
        assert(mem_write_get_data(decoded_ins, qualification, vctx, &data));
        success = lapic_write_fault_handle(offset, data);
    }

    return success;
}
#endif /* APIC_VIRT_LEVEL < APIC_VIRT_LEVEL_APICV */

enum lapic_timer_mode {
    LAPIC_TIMER_ONESHOT,
    LAPIC_TIMER_PERIODIC,
    LAPIC_TIMER_TSC_DEADLINE,
};

static enum lapic_timer_mode lapic_parse_timer_reg(void)
{
    // [1] "Figure 12-8. Local Vector Table (LVT)"
    uint32_t timer_reg = lapic_read_reg(REG_LAPIC_TIMER);
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

static uint64_t lapic_time_now_scaled(void)
{
    return guest_time_tsc_now() / lapic_dcr_to_divider();
}

static uint32_t lapic_read_curr_count_reg(void)
{
    uint32_t result = 0;

    /* [1] "12.5.4 APIC Timer" */
    if (lapic_read_reg(REG_LAPIC_INIT_CNT) != 0) {
        uint64_t apic_tick_now_scaled = lapic_time_now_scaled();
        uint64_t elapsed_scaled_apic_tick = apic_tick_now_scaled - native_scaled_apic_ticks_when_timer_starts;

        uint64_t remaining = 0;
        if (elapsed_scaled_apic_tick < lapic_read_reg(REG_LAPIC_INIT_CNT)) {
            remaining = lapic_read_reg(REG_LAPIC_INIT_CNT) - elapsed_scaled_apic_tick;
        }
        result = remaining;
        LOG_APIC("current count read 0x%lx\n", remaining);
    }

    return result;
}

static void handle_lapic_timer_nftn(size_t vcpu_id)
{
    assert(timeout_handle_valid);
    guest_time_cancel_timeout(timeout_handle);

    if (!(lapic_read_reg(REG_LAPIC_SVR) & BIT(8))) {
        /* [1] "Figure 12-23. Spurious-Interrupt Vector Register (SVR)"
         * APIC Software Disable bit */
        return;
    }

    /* Restart timeout if periodic */
    uint32_t init_count = lapic_read_reg(REG_LAPIC_INIT_CNT);
    if (lapic_parse_timer_reg() == LAPIC_TIMER_PERIODIC && init_count > 0) {
        native_scaled_apic_ticks_when_timer_starts = lapic_time_now_scaled();
        uint64_t delay_ticks = init_count * lapic_dcr_to_divider();
        LOG_APIC("restarting periodic timeout for 0x%lx ticks\n", delay_ticks);

        timeout_handle = guest_time_request_timeout(delay_ticks, &handle_lapic_timer_nftn, 0);
        assert(timeout_handle != TIMEOUT_HANDLE_INVALID);
        timeout_handle_valid = true;
    }

    /* But only inject IRQ if it is not masked */
    uint32_t timer_reg = lapic_read_reg(REG_LAPIC_TIMER);
    if (!(timer_reg & BIT(16))) {
        uint8_t vector = timer_reg & 0xff;
        assert(inject_lapic_irq(vcpu_id, vector));
    }
}

static void lapic_write_init_count_reg(uint32_t data)
{
    /* [1] "12.5.4 APIC Timer" */
    lapic_write_reg(REG_LAPIC_INIT_CNT, data);
    if (data) {
        uint32_t timer_reg = lapic_read_reg(REG_LAPIC_TIMER);
        (void)timer_reg;
        LOG_APIC("LAPIC timer started, mode 0x%x, irq masked %d\n", (timer_reg >> 17) % 0x3, !!(timer_reg & BIT(16)));

        uint64_t delay_ticks = data * lapic_dcr_to_divider();
        LOG_APIC("setting timeout for 0x%lx ticks\n", data);

        if (timeout_handle_valid) {
            /* Guest kernel has changed the timeout */
            guest_time_cancel_timeout(timeout_handle);
            timeout_handle_valid = false;
        }

        timeout_handle = guest_time_request_timeout(delay_ticks, &handle_lapic_timer_nftn, 0);
        assert(timeout_handle != TIMEOUT_HANDLE_INVALID);
        timeout_handle_valid = true;

        native_scaled_apic_ticks_when_timer_starts = lapic_time_now_scaled();
    } else {
        if (timeout_handle_valid) {
            /* Guest kernel has cancelled the timeout */
            guest_time_cancel_timeout(timeout_handle);
            timeout_handle_valid = false;
        }
    }
}

static void lapic_write_icr_low(uint32_t data)
{
    lapic_write_reg(REG_LAPIC_ICR_LOW, data);

    /* [1] "12.6.1 Interrupt Command Register (ICR)"
        * "The act of writing to the low doubleword of the ICR causes the IPI to be sent."
        */
    uint64_t icr = (uint64_t)data | (((uint64_t)lapic_read_reg(REG_LAPIC_ICR_HIGH)) << 32);

    // @billn sus, handle other types of IPIs
    uint8_t delivery_mode = ((icr >> 8) & 0x7);
    uint8_t destination = (icr >> 56) & 0xff;
    if (delivery_mode == 0 || delivery_mode == 5) {
        // fixed mode
        if (destination != 0) {
            LOG_VMM_ERR("trying to send IPI to unknown APIC ID %d\n", destination);
        }
        uint8_t vector = icr & 0xff;
        inject_lapic_irq(GUEST_BOOT_VCPU_ID, vector);
    } else {
        LOG_VMM_ERR("LAPIC received requuest to send IPI of unknown delivery mode 0x%x, destination 0x%x\n",
                    delivery_mode, destination);
    }
}

bool lapic_read_fault_handle(uint64_t offset, uint32_t *result)
{
    switch (offset) {
#if APIC_VIRT_LEVEL < APIC_VIRT_LEVEL_APICV
    case REG_LAPIC_ID:
    case REG_LAPIC_REV:
    case REG_LAPIC_APR:
    case REG_LAPIC_DFR:
    case REG_LAPIC_LDR:
    case REG_LAPIC_SVR:
    case REG_LAPIC_IRR_0:
    case REG_LAPIC_IRR_1:
    case REG_LAPIC_IRR_2:
    case REG_LAPIC_IRR_3:
    case REG_LAPIC_IRR_4:
    case REG_LAPIC_IRR_5:
    case REG_LAPIC_IRR_6:
    case REG_LAPIC_IRR_7:
    case REG_LAPIC_ISR_0:
    case REG_LAPIC_ISR_1:
    case REG_LAPIC_ISR_2:
    case REG_LAPIC_ISR_3:
    case REG_LAPIC_ISR_4:
    case REG_LAPIC_ISR_5:
    case REG_LAPIC_ISR_6:
    case REG_LAPIC_ISR_7:
    case REG_LAPIC_TMR_0:
    case REG_LAPIC_TMR_1:
    case REG_LAPIC_TMR_2:
    case REG_LAPIC_TMR_3:
    case REG_LAPIC_TMR_4:
    case REG_LAPIC_TMR_5:
    case REG_LAPIC_TMR_6:
    case REG_LAPIC_TMR_7:
    case REG_LAPIC_TIMER:
    case REG_LAPIC_LVT_ERR:
    case REG_LAPIC_THERMAL:
    case REG_LAPIC_INIT_CNT:
    case REG_LAPIC_ESR:
    case REG_LAPIC_PERF_MON_CNTER:
    case REG_LAPIC_ICR_LOW:
    case REG_LAPIC_ICR_HIGH:
    case REG_LAPIC_LVT_LINT0:
    case REG_LAPIC_LVT_LINT1:
    case REG_LAPIC_DCR:
    case REG_LAPIC_TPR:
        *result = lapic_read_reg(offset);
        break;
    case REG_LAPIC_PPR:
        *result = get_ppr();
        break;
#endif /* APIC_VIRT_LEVEL < APIC_VIRT_LEVEL_APICV */
    case REG_LAPIC_CURR_CNT: {
        *result = lapic_read_curr_count_reg();
        break;
    }
    case REG_LAPIC_CMCI: {
        *result = lapic_read_reg(offset);
        break;
    }
    default:
        LOG_VMM_ERR("Reading unknown LAPIC register offset 0x%lx\n", offset);
        return false;
    }

    return true;
}

bool lapic_write_fault_handle(uint64_t offset, uint32_t data)
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
    case REG_LAPIC_CMCI:
#if APIC_VIRT_LEVEL < APIC_VIRT_LEVEL_APICV
    case REG_LAPIC_ICR_HIGH:
#endif
        lapic_write_reg(offset, data);
        break;

    case REG_LAPIC_INIT_CNT: {
        lapic_write_init_count_reg(data);
        break;
    }
    case REG_LAPIC_ICR_LOW: {
        lapic_write_icr_low(data);
        break;
    }
#if APIC_VIRT_LEVEL < APIC_VIRT_LEVEL_APICV
    case REG_LAPIC_TPR: {
        lapic_write_tpr(data);
        break;
    }
    case REG_LAPIC_EOI: {
        lapic_write_eoi();
        break;
    }
#endif
    default:
        LOG_VMM_ERR("Writing unknown LAPIC register offset 0x%lx\n", offset);
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
            LOG_VMM_ERR("Reading unknown I/O APIC MMIO register 0x%lx\n", offset);
            return false;
        }
    } else {
        uint64_t data;
        assert(mem_write_get_data(decoded_ins, qualification, vctx, &data));

        if (offset == REG_IOAPIC_IOREGSEL_MMIO_OFF) {
            ioapic_regs.selected_reg = data & 0xff;

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
            LOG_VMM_ERR("Writing unknown I/O APIC MMIO register 0x%lx\n", offset);
            return false;
        }
    }

    return true;
}

bool inject_lapic_irq(size_t vcpu_id, uint8_t vector)
{
    assert(vcpu_id == 0);

    if (vector < MIN_VECTOR) {
        /* Architecturally reserved vector. */
        return false;
    }

    if (!(lapic_read_reg(REG_LAPIC_SVR) & BIT(8))) {
        /* [1] "Figure 12-23. Spurious-Interrupt Vector Register (SVR)"
         * APIC Software Disable bit */
        return false;
    }

    /* If this IRQ vector is already pending then it will get coalesced. */
    mark_irq_pending(vector);

#if APIC_VIRT_LEVEL < APIC_VIRT_LEVEL_APICV
    if (vcpu_can_take_irq(0)) {
        lapic_maintenance();
    } else {
        vcpu_exit_update_ppvc(VMCS_PPVC_WAIT_IRQ_WINDOW);
    }
#else
    /* See "26.4.2 Guest Non-Register State" for "Guest interrupt status" layout */
    int highest_vector_pending = get_next_pending_irq_vector();
    assert(highest_vector_pending != -1);
    uint16_t guest_irq_status = microkit_vcpu_x86_read_vmcs(0, VMX_GUEST_INTERRUPT_STATUS);
    guest_irq_status &= 0xff00;
    guest_irq_status |= (uint8_t)highest_vector_pending;
    microkit_vcpu_x86_write_vmcs(0, VMX_GUEST_INTERRUPT_STATUS, guest_irq_status);
#endif

    return true;
}

bool inject_ioapic_irq(int ioapic, int pin)
{
    /* Only 1 chip right now, which is a direct map to the dual 8259. */
    assert(ioapic == 0);

    if (pin >= IOAPIC_LAST_INDIRECT_INDEX) {
        LOG_VMM_ERR("trying to inject IRQ to out of bound I/O APIC pin %d\n", pin);
        return false;
    }

    /* Check if the irq line is masked. */
    if (ioapic_regs.ioredtbl[pin] & BIT(16)) {
        return false;
    }

    // @billn sus revisit delivery mode 1 for multiple vcpu
    uint8_t delivery_mode = (ioapic_regs.ioredtbl[pin] >> 8) & 0x7;
    if (delivery_mode != 0 && delivery_mode != 1) {
        LOG_VMM_ERR("unknown I/O APIC delivery mode for injection on pin %d, mode 0x%x\n", pin, delivery_mode);
        assert(false);
    }

    // @billn sus only support edge triggered interrupts right now
    uint8_t level_trigger = (ioapic_regs.ioredtbl[pin] >> 15) & 0x1;
    assert(!level_trigger);

    uint8_t vector = ioapic_pin_to_vector(ioapic, pin);

#if APIC_VIRT_LEVEL == APIC_VIRT_LEVEL_APICV
    /* For any passed through interrupts:
     * When the guest EOIs the interrupt, we must trigger a vmexit to run the ack func
     */
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
            LOG_VMM_ERR("impossible: eoi_bitmap_n %d > 3\n", eoi_bitmap_n);
            return false;
        }
    }
#endif

    // @billn need to read cpu id from redirection register, revisit for multi vcpu
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

#if APIC_VIRT_LEVEL == APIC_VIRT_LEVEL_APICV
                    /* Now clear the vector's bit in EOI exit bitmap */
                    int eoi_bitmap_n = vector / 64;
                    int n_bitmap_i = vector % 64;
                    switch (eoi_bitmap_n) {
                    case 0: {
                        uint64_t bitmap = microkit_vcpu_x86_read_vmcs(GUEST_BOOT_VCPU_ID,
                                                                      VMX_CONTROL_EOI_EXIT_BITMAP_0);
                        bitmap &= ~BIT(n_bitmap_i);
                        microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_CONTROL_EOI_EXIT_BITMAP_0, bitmap);
                        break;
                    }
                    case 1: {
                        uint64_t bitmap = microkit_vcpu_x86_read_vmcs(GUEST_BOOT_VCPU_ID,
                                                                      VMX_CONTROL_EOI_EXIT_BITMAP_1);
                        bitmap &= ~BIT(n_bitmap_i);
                        microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_CONTROL_EOI_EXIT_BITMAP_1, bitmap);
                        break;
                    }
                    case 2: {
                        uint64_t bitmap = microkit_vcpu_x86_read_vmcs(GUEST_BOOT_VCPU_ID,
                                                                      VMX_CONTROL_EOI_EXIT_BITMAP_2);
                        bitmap &= ~BIT(n_bitmap_i);
                        microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_CONTROL_EOI_EXIT_BITMAP_2, bitmap);
                        break;
                    }
                    case 3: {
                        uint64_t bitmap = microkit_vcpu_x86_read_vmcs(GUEST_BOOT_VCPU_ID,
                                                                      VMX_CONTROL_EOI_EXIT_BITMAP_3);
                        bitmap &= ~BIT(n_bitmap_i);
                        microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_CONTROL_EOI_EXIT_BITMAP_3, bitmap);
                        break;
                    }
                    default:
                        LOG_VMM_ERR("impossible: eoi_bitmap_n %d > 3\n", eoi_bitmap_n);
                        return false;
                    }
#endif

                    return true;
                }
            }
        }
    }

    return false;
}