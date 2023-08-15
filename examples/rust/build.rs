fn main() {
    match std::env::var("SEL4CP_BOARD_DIR") {
        Ok(sel4cp_board_dir) => println!("cargo:rustc-link-search={sel4cp_board_dir}/lib"),
        Err(e) => println!("Could not get environment variable 'BOARD_DIR': {e}"),
    }

    match std::env::var("VMM_BUILD_DIR") {
        Ok(build_dir) => println!("cargo:rustc-link-search={build_dir}"),
        Err(e) => println!("Could not get environment variable 'VMM_BUILD_DIR': {e}"),
    }
}
