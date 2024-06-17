let
  pkgs = import <nixpkgs> {};
in
  pkgs.mkShell {
    nativeBuildInputs =
    let
      crossInputs = with pkgs.pkgsCross.aarch64-multiplatform; [
        alsa-lib
      ];
      nativeInputs = with pkgs; [
        dtc
        qemu
        patchelf
        clang
        lld
        gzip
        fakeroot
        cpio
        zig
      ];
    in crossInputs ++ nativeInputs;
}

