{ pkgs ? import <nixpkgs> {} }:
  pkgs.mkShell {
    nativeBuildInputs =
    let
      crossInputs = with pkgs.pkgsCross.aarch64-multiplatform; [
        # buildPackages.gcc
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
