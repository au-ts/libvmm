#![no_std]
#![no_main]
#![feature(never_type)]

use core::{include_bytes};
use core::ffi::{c_void};

use sel4cp::message::{MessageInfo};
use sel4cp::{protection_domain, Channel, Child, Handler, debug_println};

const guest_ram_vaddr: usize = 0x40000000;
const GUEST_DTB_VADDR: usize = 0x4f000000;
const GUEST_INIT_RAM_DISK_VADDR: usize = 0x4d700000;
const GUEST_VCPU_ID: usize = 0;

/// On the QEMU ARM virt platform the UART we are using has an IRQ number of 33.
const UART_IRQ: usize = 33;
/// The VMM is expecting the IRQ to be delivered with the channel ID of 1.
const UART_CH: Channel = Channel::new(1);

// This is arguably the hardest part of writing the VMM in Rust, it is the act of
// connecting the Rust code to the VMM library written in C. Thankfully, it is not
// too bad for us to manually create the bindings, given that there are only a few
// VMM calls in order to setup a basic Linux guest.
//
// However, in the future it might not be realistic to do this, especially with other
// services of the VMM such as virtIO. Ultimately if you are writing a VMM in Rust
// then you may want to look into bindgen for automatically creating these bindings,
// or even creating your own "Rust-like" bindings for the VMM library to make it
// nicer to use (or closer to feeling like a Rust crate), or both!
#[link(name = "vmm", kind = "static")]
#[link(name = "sel4cp", kind = "static")]
extern "C" {
    fn linux_setup_images(ram_start: usize,
                          kernel: usize, kernel_size: usize,
                          dtb_src: usize, dtb_dest: usize, dtb_size: usize,
                          initrd_src: usize, initrd_dest: usize, initrd_size: usize) -> usize;
    fn virq_controller_init(boot_vpcu_id: usize) -> bool;
    fn virq_register(vcpu_id: usize, irq: i32, ack_fn: extern fn(usize, i32, *const c_void), ack_data: *const c_void) -> bool;
    fn virq_inject(vcpu_id: usize, irq: i32) -> bool;
    fn guest_start(boot_vpcu_id: usize, kernel_pc: usize, dtb: usize, initrd: usize) -> bool;
    fn fault_handle(vcpu_id: usize, msginfo: MessageInfo) -> bool;
}

extern "C" fn uart_irq_ack(_: usize, _: i32, _: *const c_void) {
    UART_CH.irq_ack();
    // debug_println("VMM|ERROR: could not ack UART IRQ channel!");
}

#[protection_domain]
fn init() -> VmmHandler {
    debug_println!("VMM|INFO: starting Rust VMM");
    // Thankfully Rust comes with a simple macro that allows us to package
    // binaries locally and placing them in the final binary that we load
    // onto our platform, in this case the QEMU ARM virt board.
    let linux = include_bytes!("../images/linux");
    let dtb = include_bytes!("../build/linux.dtb");
    let initrd = include_bytes!("../images/rootfs.cpio.gz");
    // The VMM library does not understand slices like Rust, so we have to
    // turn this slices of u8 into raw addresses.
    let linux_addr = linux.as_ptr() as usize;
    let dtb_addr = dtb.as_ptr() as usize;
    let initrd_addr = initrd.as_ptr() as usize;

    unsafe {
        let guest_pc = linux_setup_images(guest_ram_vaddr,
                                            linux_addr, linux.len(),
                                            dtb_addr, GUEST_DTB_VADDR, dtb.len(),
                                            initrd_addr, GUEST_INIT_RAM_DISK_VADDR, initrd.len()
                                         );
        // @ivanv, deal with unused vars
        _ = virq_controller_init(GUEST_VCPU_ID);
        _ = virq_register(GUEST_VCPU_ID, UART_IRQ as i32, uart_irq_ack, core::ptr::null());
        UART_CH.irq_ack();
        guest_start(GUEST_VCPU_ID, guest_pc, GUEST_DTB_VADDR, GUEST_INIT_RAM_DISK_VADDR);
    }

    VmmHandler {}
}

struct VmmHandler {}

impl Handler for VmmHandler {
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
            if fault_handle(id.unwrap(), msg_info) {
                id.reply(MessageInfo::new(0, 0));
                Ok(())
            } else {
                unreachable!()
            }
        }
    }
}
