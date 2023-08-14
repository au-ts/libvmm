#![no_std]
#![no_main]
#![feature(never_type)]

use core::{include_bytes};
use core::ffi::{c_void};

use sel4cp::message::{MessageInfo, NoMessageValue, StatusMessageLabel};
use sel4cp::{memory_region_symbol, protection_domain, Channel, Child, Handler, debug_println};

const guest_ram_vaddr: usize = 0x40000000;
const GUEST_DTB_VADDR: usize = 0x4f000000;
const GUEST_INIT_RAM_DISK_VADDR: usize = 0x4d700000;
const GUEST_VCPU_ID: usize = 0;

const UART_IRQ: usize = 33;

const UART_CH: Channel = Channel::new(1);

#[link(name = "vmm", kind = "static")]
#[link(name = "sel4cp", kind = "static")]
extern "C" {
    fn linux_setup_images(ram_start: usize,
                          kernel: usize, kernel_size: usize,
                          dtb_src: usize, dtb_dest: usize, dtb_size: usize,
                          initrd_src: usize, initrd_dest: usize, initrd_size: usize) -> usize;
    fn virq_controller_init(boot_vpcu_id: usize) -> bool;
    fn virq_register(vcpu_id: usize, irq: i32, ack_fn: fn(usize, i32, *const c_void), ack_data: *const c_void) -> bool;
    fn virq_inject(vcpu_id: usize, irq: i32) -> bool;
    fn guest_start(boot_vpcu_id: usize, kernel_pc: usize, dtb: usize, initrd: usize) -> bool;
    fn fault_to_string(label: usize) -> str;
    fn fault_handle(vcpu_id: usize, msginfo: MessageInfo) -> bool;
}

fn uart_irq_ack(_: usize, irq: i32, _: *const c_void) {
    UART_CH.irq_ack();
}

#[protection_domain]
fn init() -> ThisHandler {
    let linux = include_bytes!("../images/linux");
    let dtb = include_bytes!("../build/linux.dtb");
    let initrd = include_bytes!("../images/rootfs.cpio.gz");

    debug_println!("VMM|INFO: starting Rust VMM");

    let linux_addr = linux.as_ptr() as usize;
    let dtb_addr = dtb.as_ptr() as usize;
    let initrd_addr = initrd.as_ptr() as usize;

    unsafe {
        let guest_pc = linux_setup_images(guest_ram_vaddr,
                                            linux_addr, linux.len(),
                                            dtb_addr, GUEST_DTB_VADDR, dtb.len(),
                                            initrd_addr, GUEST_INIT_RAM_DISK_VADDR, initrd.len()
                                         );
        let success = virq_controller_init(GUEST_VCPU_ID);
        _ = virq_register(GUEST_VCPU_ID, UART_IRQ as i32, uart_irq_ack, core::ptr::null());
        UART_CH.irq_ack();
        guest_start(GUEST_VCPU_ID, guest_pc, GUEST_DTB_VADDR, GUEST_INIT_RAM_DISK_VADDR);
    }

    ThisHandler {}
}

struct ThisHandler {}

impl Handler for ThisHandler {
    type Error = !;

    fn notified(&mut self, channel: Channel) -> Result<(), Self::Error> {
        match channel {
            UART_CH => {
                unsafe {
                    let success = virq_inject(GUEST_VCPU_ID, UART_IRQ as i32);
                    if !success {
                        debug_println!("VMM|ERROR: could not inject UART IRQ");
                    }
                }
            }
            _ => unreachable!()
        }
        Ok(())
    }

    fn fault(&mut self, id: Child, msg_info: MessageInfo) -> Result<(), Self::Error> {
        unsafe {
            let success = fault_handle(id.unwrap(), msg_info);
            if success {
                id.reply(MessageInfo::new(0, 0));
                Ok(())
            } else {
                // debug_println!("Failed to handle %s fault\n", fault_to_string(label));
                Ok(())
            }
        }
    }
}
