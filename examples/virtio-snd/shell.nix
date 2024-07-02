{ pkgs ? import (fetchTarball "https://github.com/NixOS/nixpkgs/archive/057f9aecfb71c4437d2b27d3323df7f93c010b7e.tar.gz") {} }:
  pkgs.mkShell {
    nativeBuildInputs =
    let
      crossInputs = with pkgs.pkgsCross.aarch64-multiplatform; [
        alsa-lib
      ];
      nativeInputs = with pkgs; [
        llvm
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
