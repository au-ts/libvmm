// Copyright 2024, UNSW
// SPDX-License-Identifier: BSD-2-Clause

fn main() {
    // Our VMM needs to link with libmicrokit, this is how we tell Cargo where to look for it.
    match std::env::var("MICROKIT_BOARD_DIR") {
        Ok(microkit_board_dir) => println!("cargo:rustc-link-search={microkit_board_dir}/lib"),
        Err(e) => println!("Could not get environment variable 'MICROKIT_BOARD_DIR': {e}"),
    }

    match std::env::var("BUILD_DIR") {
        Ok(build_dir) => println!("cargo:rustc-link-search={build_dir}"),
        Err(e) => println!("Could not get environment variable 'BUILD_DIR': {e}"),
    }
}
