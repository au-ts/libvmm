fn main() {
    println!("cargo:rustc-link-search=/home/ivanv/ts/sel4cp/release/sel4cp-sdk-1.2.6/board/qemu_arm_virt/debug/lib/");
    println!("cargo:rustc-link-search=/home/ivanv/ts/sel4cp_vmm/examples/rust/build");
}

