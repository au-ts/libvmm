let
    rust_overlay = import (builtins.fetchTarball "https://github.com/oxalica/rust-overlay/archive/master.tar.gz");
    pkgs = import <nixpkgs> { overlays = [ rust_overlay ]; };
    rust = pkgs.rust-bin.fromRustupToolchainFile ./examples/rust/rust-toolchain.toml;
in
  pkgs.mkShell {
    buildInputs = with pkgs.buildPackages; [
        zig_0_11
        rust
        qemu
        gnumake
        dtc
        llvmPackages_16.clang
        llvmPackages_16.lld
        llvmPackages_16.libllvm
        # expect is only needed for CI testing but we include it for
        # completeness
        expect
    ];
    hardeningDisable = [ "all" ];
}

