let
    pkgs = import <nixpkgs> {};
in
  pkgs.mkShell {
    buildInputs = with pkgs.buildPackages; [
        zig_0_11
        qemu
        gnumake
        dtc
        llvmPackages_16.clang
        llvmPackages_16.lld
        # expect is only needed for CI testing but we include it for
        # completeness
        expect
    ];
    hardeningDisable = [ "all" ];
}

