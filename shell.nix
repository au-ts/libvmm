let
    pkgs = import <nixpkgs> {};
in
  pkgs.mkShell {
    buildInputs = with pkgs.buildPackages; [
        qemu
        gnumake
		dtc
		expect
        llvmPackages_16.clang
        llvmPackages_16.lld
    ];
}

