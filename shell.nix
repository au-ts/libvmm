let
    rust_overlay = import (builtins.fetchTarball "https://github.com/oxalica/rust-overlay/archive/master.tar.gz");
    zig = import (builtins.fetchTarball "https://github.com/mitchellh/zig-overlay/archive/master.tar.gz") {};
    pkgs = import <nixpkgs> { overlays = [ rust_overlay ]; };
    rust = pkgs.rust-bin.fromRustupToolchainFile ./examples/rust/rust-toolchain.toml;
    llvm = pkgs.llvmPackages_11;
in
  pkgs.mkShell {
    buildInputs = with pkgs.buildPackages; [
        zig.master
        rust
        qemu
        gnumake
        dtc
        llvm.clang
        llvm.lld
        llvm.libllvm
        llvm.libclang
        # expect is only needed for CI testing but we include it for
        # completeness
        expect
    ];
    hardeningDisable = [ "all" ];
    # Need to specify this when using Rust with bindgen
    LIBCLANG_PATH = "${llvm.libclang.lib}/lib";
}

