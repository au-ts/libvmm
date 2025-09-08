#
# Copyright 2024, UNSW
# SPDX-License-Identifier: BSD-2-Clause
#
{
  description = "A flake for building libvmm and its examples";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-25.05";
    zig-overlay.url = "github:mitchellh/zig-overlay";
    zig-overlay.inputs.nixpkgs.follows = "nixpkgs";
    sdfgen.url = "github:au-ts/microkit_sdf_gen/0.26.0";
    sdfgen.inputs.nixpkgs.follows = "nixpkgs";
    rust-overlay = {
      url = "github:oxalica/rust-overlay";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };

  outputs = { nixpkgs, zig-overlay, rust-overlay, sdfgen, ... }:
    let
      microkit-version = "2.0.1";
      microkit-platforms = {
        aarch64-darwin = "macos-aarch64";
        x86_64-darwin = "macos-x86-64";
        x86_64-linux = "linux-x86-64";
        aarch64-linux = "linux-aarch64";
      };
      overlays = [ (import rust-overlay) ];

      forAllSystems = with nixpkgs.lib; genAttrs (builtins.attrNames microkit-platforms);
    in
    {
      devShells = forAllSystems
        (system: {
          default =
            let
              pkgs = import nixpkgs {
                inherit system overlays;
              };

              llvm = pkgs.llvmPackages_18;
              zig = zig-overlay.packages.${system}."0.15.1";
              rust = pkgs.rust-bin.fromRustupToolchainFile ./examples/rust/rust-toolchain.toml;

              pysdfgen = sdfgen.packages.${system}.pysdfgen.override { zig = zig; pythonPackages = pkgs.python312Packages; };

              python = pkgs.python312.withPackages (ps: [
                pysdfgen
              ]);
            in
            # mkShellNoCC, because we do not want the cc from stdenv to leak into this shell
            pkgs.mkShellNoCC rec {
              name = "libvmm-dev";

              microkit-platform = microkit-platforms.${system} or (throw "Unsupported system: ${system}");

              env.MICROKIT_SDK = pkgs.fetchzip {
                url = "https://github.com/seL4/microkit/releases/download/${microkit-version}/microkit-sdk-${microkit-version}-${microkit-platform}.tar.gz";
                hash = {
                  aarch64-darwin = "sha256-bFFyVBF2E3YDJ6CYbfCOID7KGREQXkIFDpTD4MzxfCE=";
                  x86_64-darwin = "sha256-tQWrI5LRp05tLy/HIxgN+0KFJrlmOQ+dpws4Fre+6E0=";
                  x86_64-linux = "sha256-YpgIAXWB8v4Njm/5Oo0jZpRt/t+e+rVTwFTJ8zr2Hn4=";
                  aarch64-linux = "sha256-GwWDRJalJOpAYCP/qggFOHDh2e2J1LspWUsyjopECYA=";
                }.${system} or (throw "Unsupported system: ${system}");
              };

              nativeBuildInputs = with pkgs; [
                rust
                git
                zig
                qemu
                gnumake
                dosfstools
                # for git-clang-format.
                llvm.libclang.python
                llvm.lld
                llvm.libllvm
                llvm.clang
                llvm.libclang
                dtc
                python
                util-linux
                # Needed for CI testing
                expect
                # for shasum
                perl
                curl
                which
                cpio
              ];

              # To avoid Nix adding compiler flags that are not available on a freestanding
              # environment.
              hardeningDisable = [ "all" ];
              # Necessary for Rust bindgen
              LIBCLANG_PATH = "${llvm.libclang.lib}/lib";
            };
        });
    };
}
