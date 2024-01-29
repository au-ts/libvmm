{ pkgs ? import <nixpkgs> {} }:
  pkgs.mkShell {
    nativeBuildInputs = with pkgs; [
      dtc
      qemu
      clang
      lld
      meson
      ninja
    ];
}
