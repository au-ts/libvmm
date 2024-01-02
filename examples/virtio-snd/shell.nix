{ pkgs ? import <nixpkgs> {} }:
  pkgs.mkShell {
    nativeBuildInputs =    # nativeBuildInputs is usually what you want -- tools you need to run
    let
      crossInputs = with pkgs.pkgsCross.aarch64-multiplatform; [
        buildPackages.gcc
        alsa-lib
      ];
      nativeInputs = with pkgs; [
        dtc
        qemu
        patchelf
        buildPackages.clang
        buildPackages.lld
        gzip
        fakeroot
        cpio
        # used to generate compile_commands.json
        bear
      ];
    in crossInputs ++ nativeInputs;
}
