#include <microkit.h>
#include <libvmm/util/util.h>
#include <libvmm/arch/x86_64/apic.h>
#include <libvmm/arch/x86_64/fault.h>
#include <libvmm/arch/x86_64/vmcs.h>
#include <libvmm/guest.h>
#include <sel4/arch/vmenter.h>
#include <sddf/util/util.h>
#include <sddf/timer/client.h>

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
#define REG_LAPIC_EOI 0xb0
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
#define REG_LAPIC_IRR_0 0x200
#define REG_LAPIC_IRR_1 0x210
#define REG_LAPIC_IRR_2 0x220
#define REG_LAPIC_IRR_3 0x230
#define REG_LAPIC_IRR_4 0x240
#define REG_LAPIC_IRR_5 0x250
#define REG_LAPIC_IRR_6 0x260
#define REG_LAPIC_IRR_7 0x270
#define REG_LAPIC_ESR 0x280
#define REG_LAPIC_ICR 0x300
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

struct lapic_regs lapic_regs;
// https://pdos.csail.mit.edu/6.828/2016/readings/ia32/ioapic.pdf
struct ioapic_regs ioapic_regs;
uint64_t lapic_timer_hz;

static uint64_t tsc_ticks_to_ns(uint64_t tsc_hz, uint64_t tsc_ticks) {
    double seconds = (double) tsc_ticks / (double) tsc_hz;
    return (uint64_t) (seconds * (double) NS_IN_S);
}

bool lapic_fault_handle(seL4_VCPUContext *vctx, uint64_t offset, seL4_Word qualification,
                        memory_instruction_data_t decoded_mem_ins)
{
    uint64_t *vctx_raw = (uint64_t *)vctx;

    // TODO: support other alignments?
    assert(offset % 4 == 0);
    if (ept_fault_is_read(qualification)) {
        LOG_APIC("handling LAPIC read at offset 0x%lx\n", offset);
    } else if (ept_fault_is_write(qualification)) {
        LOG_APIC("handling LAPIC write at offset 0x%lx, value 0x%x\n", offset, vctx_raw[decoded_mem_ins.target_reg]);
    }

    if (ept_fault_is_read(qualification)) {
        switch (offset) {
        case REG_LAPIC_ID:
            vctx_raw[decoded_mem_ins.target_reg] = lapic_regs.id;
            break;
        case REG_LAPIC_REV:
            vctx_raw[decoded_mem_ins.target_reg] = lapic_regs.revision;
            break;
        case REG_LAPIC_TPR:
            vctx_raw[decoded_mem_ins.target_reg] = lapic_regs.tpr;
            break;
        case REG_LAPIC_SVR:
            vctx_raw[decoded_mem_ins.target_reg] = lapic_regs.svr;
            break;
        case REG_LAPIC_IRR_0:
            vctx_raw[decoded_mem_ins.target_reg] = lapic_regs.irr[0];
            break;
        case REG_LAPIC_IRR_1:
            vctx_raw[decoded_mem_ins.target_reg] = lapic_regs.irr[1];
            break;
        case REG_LAPIC_IRR_2:
            vctx_raw[decoded_mem_ins.target_reg] = lapic_regs.irr[2];
            break;
        case REG_LAPIC_IRR_3:
            vctx_raw[decoded_mem_ins.target_reg] = lapic_regs.irr[3];
            break;
        case REG_LAPIC_IRR_4:
            vctx_raw[decoded_mem_ins.target_reg] = lapic_regs.irr[4];
            break;
        case REG_LAPIC_IRR_5:
            vctx_raw[decoded_mem_ins.target_reg] = lapic_regs.irr[5];
            break;
        case REG_LAPIC_IRR_6:
            vctx_raw[decoded_mem_ins.target_reg] = lapic_regs.irr[6];
            break;
        case REG_LAPIC_IRR_7:
            vctx_raw[decoded_mem_ins.target_reg] = lapic_regs.irr[7];
            break;
        case REG_LAPIC_ISR_0:
            vctx_raw[decoded_mem_ins.target_reg] = lapic_regs.isr[0];
            break;
        case REG_LAPIC_ISR_1:
            vctx_raw[decoded_mem_ins.target_reg] = lapic_regs.isr[1];
            break;
        case REG_LAPIC_ISR_2:
            vctx_raw[decoded_mem_ins.target_reg] = lapic_regs.isr[2];
            break;
        case REG_LAPIC_ISR_3:
            vctx_raw[decoded_mem_ins.target_reg] = lapic_regs.isr[3];
            break;
        case REG_LAPIC_ISR_4:
            vctx_raw[decoded_mem_ins.target_reg] = lapic_regs.isr[4];
            break;
        case REG_LAPIC_ISR_5:
            vctx_raw[decoded_mem_ins.target_reg] = lapic_regs.isr[5];
            break;
        case REG_LAPIC_ISR_6:
            vctx_raw[decoded_mem_ins.target_reg] = lapic_regs.isr[6];
            break;
        case REG_LAPIC_ISR_7:
            vctx_raw[decoded_mem_ins.target_reg] = lapic_regs.isr[7];
            break;
        case REG_LAPIC_TIMER:
            vctx_raw[decoded_mem_ins.target_reg] = lapic_regs.timer;
            break;
        case REG_LAPIC_LVT_ERR:
        case REG_LAPIC_THERMAL:
            break;
        case REG_LAPIC_INIT_CNT:
            vctx_raw[decoded_mem_ins.target_reg] = lapic_regs.init_count;
            break;
        case REG_LAPIC_CURR_CNT:
            uint64_t tsc_tick_now = rdtsc();
            uint64_t elapsed_tsc_tick = tsc_tick_now - lapic_regs.native_tsc_when_timer_starts;

            uint64_t remaining = 0;
            if (elapsed_tsc_tick < lapic_regs.init_count) {
                remaining = lapic_regs.init_count - elapsed_tsc_tick;
            }
            vctx_raw[decoded_mem_ins.target_reg] = remaining;
            break;
            
            break;
        case REG_LAPIC_ESR:
        case REG_LAPIC_PERF_MON_CNTER:
            vctx_raw[decoded_mem_ins.target_reg] = 0;
            break;
        case REG_LAPIC_ICR:
            vctx_raw[decoded_mem_ins.target_reg] = lapic_regs.icr;
            break;
        case REG_LAPIC_LVT_LINT0:
            vctx_raw[decoded_mem_ins.target_reg] = lapic_regs.lint0;
            break;
        case REG_LAPIC_LVT_LINT1:
            vctx_raw[decoded_mem_ins.target_reg] = lapic_regs.lint1;
            break;
        case REG_LAPIC_DCR:
            vctx_raw[decoded_mem_ins.target_reg] = lapic_regs.dcr;
            break;
        default:
            LOG_VMM_ERR("Reading unknown LAPIC register offset 0x%x\n", offset);
            return false;
        }
    } else {
        switch (offset) {
        case REG_LAPIC_TPR:
            lapic_regs.tpr = vctx_raw[decoded_mem_ins.target_reg];
            break;
        case REG_LAPIC_EOI:
            // @billn really sus, need to check what is the last in service interrupt and only clear that.
            // LOG_VMM("lapic EOI\n");
            
            lapic_regs.isr[0] = 0;
            lapic_regs.isr[1] = 0;
            lapic_regs.isr[2] = 0;
            lapic_regs.isr[3] = 0;
            lapic_regs.isr[4] = 0;
            lapic_regs.isr[5] = 0;
            lapic_regs.isr[6] = 0;
            lapic_regs.isr[7] = 0;

            break;
        case REG_LAPIC_SVR:
            lapic_regs.svr = vctx_raw[decoded_mem_ins.target_reg];
            break;
        case REG_LAPIC_LVT_LINT0:
            lapic_regs.lint0 = vctx_raw[decoded_mem_ins.target_reg];
            LOG_VMM("REG_LAPIC_LVT_LINT0 write 0x%lx\n", lapic_regs.lint0);
            break;
        case REG_LAPIC_LVT_LINT1:
            lapic_regs.lint1 = vctx_raw[decoded_mem_ins.target_reg];
            LOG_VMM("REG_LAPIC_LVT_LINT1 write 0x%lx\n", lapic_regs.lint1);
            break;
        case REG_LAPIC_ICR:
            lapic_regs.icr = vctx_raw[decoded_mem_ins.target_reg];
            break;
        case REG_LAPIC_TIMER:
            lapic_regs.timer = vctx_raw[decoded_mem_ins.target_reg];
            break;
        case REG_LAPIC_ESR:
        case REG_LAPIC_THERMAL:
        case REG_LAPIC_LVT_ERR:
        case REG_LAPIC_PERF_MON_CNTER:
            // lapic_regs.esr = 0;
            break;
        case REG_LAPIC_INIT_CNT:
            // @billn handle case when the guest turns off the timer by writing 0
            // Figure 11-8. Local Vector Table (LVT)
            if (vctx_raw[decoded_mem_ins.target_reg] > 0) {
                lapic_regs.init_count = vctx_raw[decoded_mem_ins.target_reg];
    
                if (lapic_regs.timer & BIT(16)) {
                    LOG_VMM("LAPIC timer started while irq MASKED\n");
                } else {
                    LOG_VMM("LAPIC timer started while irq UNMASKED, mode 0x%x\n", (lapic_regs.timer >> 17) % 0x3);
                    
                    // make sure the guest isn't using tsc-deadline mode
                    assert(((lapic_regs.timer >> 17) & 0x3) != 0x2);
    
                    LOG_VMM("setting timeout for %lu ns\n", tsc_ticks_to_ns(lapic_timer_hz, lapic_regs.init_count));
                    sddf_timer_set_timeout(TIMER_DRV_CH_FOR_LAPIC, tsc_ticks_to_ns(lapic_timer_hz, lapic_regs.init_count));
                }
                lapic_regs.native_tsc_when_timer_starts = rdtsc();

            }
            break;
        case REG_LAPIC_DCR:
            lapic_regs.dcr = vctx_raw[decoded_mem_ins.target_reg];
            break;
        default:
            LOG_VMM_ERR("Writing unknown LAPIC register offset 0x%x, value 0x%lx\n", offset,
                        vctx_raw[decoded_mem_ins.target_reg]);
            return false;
        }
    }

    return true;
}

bool ioapic_fault_handle(seL4_VCPUContext *vctx, uint64_t offset, seL4_Word qualification,
                         memory_instruction_data_t decoded_mem_ins)
{
    if (ept_fault_is_read(qualification)) {
        LOG_APIC("handling I/O APIC read at MMIO offset 0x%lx\n", offset);
    } else if (ept_fault_is_write(qualification)) {
        LOG_APIC("handling I/O APIC write at MMIO offset 0x%lx\n", offset);
    }

    uint64_t *vctx_raw = (uint64_t *)vctx;

    if (ept_fault_is_read(qualification)) {
        if (offset == REG_IOAPIC_IOWIN_MMIO_OFF) {
            if (ioapic_regs.selected_reg == REG_IOAPIC_IOAPICID_REG_OFF) {
                vctx_raw[decoded_mem_ins.target_reg] = ioapic_regs.ioapicid;
            } else if (ioapic_regs.selected_reg == REG_IOAPIC_IOAPICVER_REG_OFF) {
                vctx_raw[decoded_mem_ins.target_reg] = ioapic_regs.ioapicver;
            } else if (ioapic_regs.selected_reg == REG_IOAPIC_IOAPICARB_REG_OFF) {
                vctx_raw[decoded_mem_ins.target_reg] = ioapic_regs.ioapicarb;
            } else if (ioapic_regs.selected_reg >= REG_IOAPIC_IOREDTBL_FIRST_OFF
                       && ioapic_regs.selected_reg <= REG_IOAPIC_IOREDTBL_LAST_OFF) {
                int redirection_reg_idx = ((ioapic_regs.selected_reg - REG_IOAPIC_IOREDTBL_FIRST_OFF) & ~((uint64_t)1)) / 2;
                int is_high = ioapic_regs.selected_reg & 0x1;

                // LOG_VMM("reading indirect register 0x%x, is high %d\n", redirection_reg_idx, is_high);

                if (is_high) {
                    vctx_raw[decoded_mem_ins.target_reg] = ioapic_regs.ioredtbl[redirection_reg_idx] >> 32;
                } else {
                    vctx_raw[decoded_mem_ins.target_reg] = ioapic_regs.ioredtbl[redirection_reg_idx] & 0xffffffff;
                }

            } else {
                LOG_VMM_ERR("Reading unknown I/O APIC register offset 0x%x\n", ioapic_regs.selected_reg);
                return false;
            }
        } else {
            LOG_VMM_ERR("Reading unknown I/O APIC MMIO register 0x%x\n", offset);
            return false;
        }
    } else {
        if (offset == REG_IOAPIC_IOREGSEL_MMIO_OFF) {
            ioapic_regs.selected_reg = vctx_raw[decoded_mem_ins.target_reg] & 0xff;
            // LOG_VMM("selecting I/O APIC register 0x%x\n", ioapic_regs.selected_reg);
        } else if (offset == REG_IOAPIC_IOWIN_MMIO_OFF) {
            if (ioapic_regs.selected_reg == REG_IOAPIC_IOAPICID_REG_OFF) {
                LOG_APIC("Written to I/O APIC ID register: 0x%lx\n", vctx_raw[decoded_mem_ins.target_reg]);
                ioapic_regs.ioapicid = vctx_raw[decoded_mem_ins.target_reg];
            } else if (ioapic_regs.selected_reg >= REG_IOAPIC_IOREDTBL_FIRST_OFF
                       && ioapic_regs.selected_reg <= REG_IOAPIC_IOREDTBL_LAST_OFF) {
                int redirection_reg_idx = ((ioapic_regs.selected_reg - REG_IOAPIC_IOREDTBL_FIRST_OFF) & ~((uint64_t)1)) / 2;
                int is_high = ioapic_regs.selected_reg & 0x1;

                // LOG_VMM("writing indirect register 0x%x\n", redirection_reg_idx);

                if (is_high) {
                    uint64_t new_high = vctx_raw[decoded_mem_ins.target_reg] << 32;
                    uint64_t low = ioapic_regs.ioredtbl[redirection_reg_idx] & 0xffffffff;
                    ioapic_regs.ioredtbl[redirection_reg_idx] = low | new_high;
                } else {
                    uint64_t high = (ioapic_regs.ioredtbl[redirection_reg_idx] >> 32) << 32;
                    uint64_t new_low = vctx_raw[decoded_mem_ins.target_reg] & 0xffffffff;
                    ioapic_regs.ioredtbl[redirection_reg_idx] = new_low | high;
                }

                
                // LOG_VMM("written value for redirection %d is %lx\n", redirection_reg_idx, ioapic_regs.ioredtbl[redirection_reg_idx]);
                
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

void lapic_maintenance(void) {
    // scan IRRs for pending interrupt
    // do it "right-to-left" as the higer vector is higher prio (generally)

    // for (int i = 7; i >= 0; i--) {

    // }

    uint8_t vector = lapic_regs.timer & 0xff; //@billn hack
    int irr_n = vector / 32;
    int irr_idx = vector % 32;

    // Should not be in service
    // assert(!(lapic_regs.isr[irr_n] & BIT(irr_idx)));

    // Move IRQ from pending to in-service
    lapic_regs.irr[irr_n] &= ~BIT(irr_idx);
    lapic_regs.isr[irr_n] |= BIT(irr_idx);

    assert(microkit_vcpu_x86_read_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_INTERRUPTABILITY) == 0);
    assert(microkit_vcpu_x86_read_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_RFLAGS) & BIT(9));

    // step 3, tell the hardware to inject an irq on next vmenter.
    // Table 25-17. Format of the VM-Entry Interruption-Information Field
    uint32_t vm_entry_interruption = (uint32_t) vector;
    vm_entry_interruption |= BIT(31); // valid bit

    // LOG_VMM("vm_entry_interruption %lx\n", vm_entry_interruption);

    microkit_mr_set(SEL4_VMENTER_CALL_CONTROL_ENTRY_MR, vm_entry_interruption);
    microkit_mr_set(SEL4_VMENTER_CALL_CONTROL_PPC_MR, VMCS_PCC_DEFAULT);

    // LOG_VMM("irq injected\n");
}

bool vcpu_can_take_irq(size_t vcpu_id) {
    if (microkit_vcpu_x86_read_vmcs(vcpu_id, VMX_GUEST_INTERRUPTABILITY) != 0) {
        return false;
    }
    if (!(microkit_vcpu_x86_read_vmcs(vcpu_id, VMX_GUEST_RFLAGS) & BIT(9))) {
        return false;
    }
    return true;
}

bool inject_lapic_irq(size_t vcpu_id, uint8_t vector) {
    assert(vcpu_id == 0);

    int irr_n = vector / 32;
    int irr_idx = vector % 32;
    if (lapic_regs.irr[irr_n] & BIT(irr_idx)) {
        // LOG_VMM("LAPIC irq inject vector %d already pending\n", vector);
        return false;
    }

    // Mark as pending for injection:
    // 1. immediately if the vCPU can take it,
    // 2. at some point in the future when the vCPU re-enable IRQs
    lapic_regs.irr[irr_n] |= BIT(irr_idx);

    if (vcpu_can_take_irq(vcpu_id)) {
        // LOG_VMM("irq ready\n");
        lapic_maintenance();
    } else {
        // LOG_VMM("wait for window\n");
        microkit_mr_set(SEL4_VMENTER_CALL_CONTROL_PPC_MR, VMCS_PCC_EXIT_IRQ_WINDOW);
    }
    return true;
}

bool handle_lapic_timer_nftn(size_t vcpu_id) {
    if (!(lapic_regs.timer & BIT(16))) {
        // timer not masked in local APIC

        uint8_t vector = lapic_regs.timer & 0xff;
        if (!inject_lapic_irq(vcpu_id, vector)) {
            LOG_VMM_ERR("failed to inject LAPIC timer IRQ vector 0x%x\n", vector);
            return false;
        }

        // restart timeout if periodic
        if (((lapic_regs.timer >> 17) & 0x3) == 1 && lapic_regs.init_count > 0) {
            lapic_regs.native_tsc_when_timer_starts = rdtsc();
            // LOG_VMM("restarting timeout for %lu ns\n", tsc_ticks_to_ns(lapic_timer_hz, lapic_regs.init_count));
            sddf_timer_set_timeout(TIMER_DRV_CH_FOR_LAPIC, tsc_ticks_to_ns(lapic_timer_hz, lapic_regs.init_count));
        } else {
            assert(false);
        }

        // LOG_VMM("lapic tick\n");
    }

    return true;
}

// bool inject_ioapic_irq(size_t vcpu_id, int pin) {
//     if (pin >= IOAPIC_LAST_INDIRECT_INDEX) {
//         LOG_VMM_ERR("trying to inject IRQ to out of bound I/O APIC pin %d\n", pin);
//         return false;
//     }

//     // step 1, read what vector the guest want the PIT irq0 to be delivered to.
//     // page 13 of the ioapic spec pdf
//     uint8_t cpu_vector = ioapic_regs.ioredtbl[pin] & 0xff;

//     // check if the irq is masked
//     if (ioapic_regs.ioredtbl[pin] & BIT(16)) {
//         // LOG_VMM("masked pin %d, vector %d\n", pin, cpu_vector);
//         return true;
//     }

//     // step 2, update the LAPIC state.
//     // set the vector's bit in the interrupt request register (IRR)
//     int irr_n = cpu_vector / 32;
//     int irr_idx = cpu_vector % 32;

//     if (cpu_vector > 255) {
//         LOG_VMM_ERR("guest tried to inject IRQ with vector > 255\n");
//         return false;
//     }

//     if (lapic_regs.irr[irr_n] & BIT(irr_idx)) {
//         // interrupt already awaiting service, drop.
//         LOG_VMM("ioapic irq inject pin %d, vector %d dropped\n", pin, cpu_vector);
//         return true;
//     }
//     // lapic_regs.irr[irr_n] |= BIT(irr_idx);

//     // @billn properly handle interrupt priority
//     // sus maxxing
//     // move request to servicing.
//     // lapic_regs.irr[irr_n] &= ~BIT(irr_idx);
//     lapic_regs.isr[irr_n] |= BIT(irr_idx);

//     // step 3, tell the hardware that we are injecting an irq on next vmenter.
//     // Table 25-17. Format of the VM-Entry Interruption-Information Field
//     uint32_t vm_entry_interruption = (uint32_t) cpu_vector;
//     vm_entry_interruption |= BIT(31); // valid bit
//     microkit_vcpu_x86_write_vmcs(vcpu_id, VMX_CONTROL_ENTRY_INTERRUPTION_INFO, vm_entry_interruption);

//     // LOG_VMM("injected I/O APIC IRQ vector %d, pin %d\n",cpu_vector,pin );

//     return true;
// }
