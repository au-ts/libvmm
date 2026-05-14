// Copyright 2024, UNSW
// SPDX-License-Identifier: BSD-2-Clause

#![no_std]
#![no_main]
#![feature(never_type)]

use core::{include_bytes};
use core::ffi::c_void;

use sel4_microkit::{protection_domain, MessageInfo, Channel, Child, Handler, debug_println};

const GUEST_RAM_VMM_VADDR: usize = 0x40000000;
/* For ARM, these constants depends on what's defined in your DTB. */
const GUEST_RAM_START_GPA: usize = 0x40000000;
const GUEST_DTB_GPA: usize = 0x4f000000;
const GUEST_INIT_RAM_DISK_GPA: usize = 0x4d700000;
const GUEST_RAM_SIZE: usize = 0x10000000;
const GUEST_BOOT_VCPU_ID: usize = 0;

/// On the QEMU virt AArch64 platform the UART we are using has an IRQ number of 33.
const UART_IRQ: usize = 33;
/// The VMM is expecting the IRQ to be delivered with the channel ID of 1.
const UART_CH: Channel = Channel::new(1);

// This is arguably the hardest part of writing the VMM in Rust, it is the act of
// connecting the Rust code to libvmm which is written in C. Thankfully, it is not
// too bad for us to manually create the bindings, given that there are only a few
// VMM calls in order to setup a basic Linux guest.
//
// However, in the future it might not be realistic to do this, especially with other
// services of the VMM such as virtIO. Ultimately if you are writing a VMM in Rust
// then you may want to look into bindgen for automatically creating these bindings,
// or even creating your own "Rust-like" bindings for libvmm to make it nicer to use
// (or closer to feeling like a Rust crate), or both!
#[link(name = "vmm", kind = "static")]
#[link(name = "sddf_util_debug", kind = "static")]
#[link(name = "microkit", kind = "static")]
extern "C" {
    fn guest_init(init_args: arch_guest_init) -> bool;
    fn linux_setup_images(ram_start: usize,
                          kernel: usize, kernel_size: usize,
                          dtb_src: usize, dtb_dest: usize, dtb_size: usize,
                          initrd_src: usize, initrd_dest: usize, initrd_size: usize) -> usize;
    fn virq_register_passthrough(vcpu_id: usize, irq: i32, irq_ch: u32) -> bool;
    fn virq_handle_passthrough(irq_ch: u32) -> bool;
    fn guest_start(kernel_pc: usize, dtb: usize, initrd: usize) -> bool;
    fn fault_handle(vcpu_id: usize, msginfo: MessageInfo) -> bool;
}

pub const GUEST_MAX_RAM_REGIONS: usize = 1;

#[repr(C)]
pub struct guest_ram_region {
    pub gpa_start: usize,
    pub size: usize,
    pub vmm_vaddr: *mut c_void,
}

#[repr(C)]
pub struct arch_guest_init {
    pub num_vcpus: usize,
    pub num_guest_ram_regions: usize,
    pub guest_ram_regions: [guest_ram_region; GUEST_MAX_RAM_REGIONS],
}

#[protection_domain]
fn init() -> VmmHandler {
    debug_println!("VMM|INFO: starting Rust VMM");
    // Thankfully Rust comes with a simple macro that allows us to package
    // binaries locally and placing them in the final binary that we load
    // onto our platform, in this case the QEMU virt AArch64 board.
    let linux = include_bytes!(env!("LINUX_PATH"));
    let dtb = include_bytes!(concat!(env!("BUILD_DIR"), "/linux.dtb"));
    let initrd = include_bytes!(env!("INITRD_PATH"));
    // libvmm does not understand slices like Rust, so we have to
    // turn this slices of u8 into raw addresses.
    let linux_addr = linux.as_ptr() as usize;
    let dtb_addr = dtb.as_ptr() as usize;
    let initrd_addr = initrd.as_ptr() as usize;

    unsafe {
        let success = guest_init(arch_guest_init {
            num_vcpus: 1,
            num_guest_ram_regions: 1,
            guest_ram_regions: [guest_ram_region {
                gpa_start: GUEST_RAM_START_GPA,
                size: GUEST_RAM_SIZE,
                vmm_vaddr: GUEST_RAM_VMM_VADDR as *mut _,
            }],
        });
        assert!(success);

        let guest_pc = linux_setup_images(GUEST_RAM_START_GPA,
                                            linux_addr, linux.len(),
                                            dtb_addr, GUEST_DTB_GPA, dtb.len(),
                                            initrd_addr, GUEST_INIT_RAM_DISK_GPA, initrd.len()
                                         );
        assert!(guest_pc != 0);

        let success = virq_register_passthrough(GUEST_BOOT_VCPU_ID, UART_IRQ as i32, UART_CH.index() as u32);
        assert!(success);
        guest_start(guest_pc, GUEST_DTB_GPA, GUEST_INIT_RAM_DISK_GPA);
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
                    let success = virq_handle_passthrough(UART_CH.index() as u32);
                    if !success {
                        debug_println!("VMM|ERROR: could not inject UART IRQ");
                    }
                }
            }
            _ => unreachable!()
        }
        Ok(())
    }

    fn fault(&mut self, id: Child, msg_info: MessageInfo) -> Result<Option<MessageInfo>, Self::Error> {
        unsafe {
            if fault_handle(id.index(), msg_info) {
                Ok(Some(MessageInfo::new(0, 0)))
            } else {
                unreachable!()
            }
        }
    }
}
