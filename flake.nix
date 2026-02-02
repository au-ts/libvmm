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
    sdfgen.url = "github:au-ts/microkit_sdf_gen/0.28.1";
    sdfgen.inputs.nixpkgs.follows = "nixpkgs";
    rust-overlay = {
      url = "github:oxalica/rust-overlay";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };

  outputs = { nixpkgs, zig-overlay, rust-overlay, sdfgen, ... }:
    let
      microkit-version = "2.1.0-dev.12+2d5a1a6";
      microkit-url = "https://trustworthy.systems/Downloads/microkit/";
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
      devShells = forAllSystems (
        system:
        let
          pkgs = import nixpkgs {
            inherit system overlays;
          };

          llvm = pkgs.llvmPackages_18;
          zig = zig-overlay.packages.${system}."0.15.1";
          rust = pkgs.rust-bin.fromRustupToolchainFile ./examples/rust/rust-toolchain.toml;

          clang-complete = (pkgs.symlinkJoin {
            name = "clang-complete";
            paths = llvm.clang-unwrapped.all;
            meta.mainProgram = "clang";

            # Clang searches up from the directory where it sits to find its built-in
            # headers. The `symlinkJoin` creates a symlink to the clang binary, and that
            # symlink is what ends up in your PATH from this shell. However, that symlink's
            # destination, the clang binary file, still resides in its own nix store
            # entry (`llvm.clang-unwrapped`), isolated from the header files (found in
            # `llvm.clang-unwrapped.lib` under `lib/clang/18/include`). So when search up its
            # parent directories, no built-in headers are found.
            #
            # By copying over the clang binary over the symlinks in the realisation of the
            # `symlinkJoin`, we can fix this; now the search mechanism looks up the parent
            # directories of the `clang` binary (which is a copy created by below command),
            # until it finds the aforementioned `lib/clang/18/include` (where the `lib` is
            # actually a symlink to `llvm.clang-unwrapped.lib + "/lib"`).
            postBuild = ''
              cp --remove-destination -- ${llvm.clang-unwrapped}/bin/* $out/bin/
            '';
          });

          pysdfgen = sdfgen.packages.${system}.pysdfgen.override { zig = zig; pythonPackages = pkgs.python312Packages; };

          python = pkgs.python312.withPackages (ps: [
            pysdfgen
          ]);
        in
        {
            docs = pkgs.mkShell rec {
              nativeBuildInputs = with pkgs; [
                texliveFull
                pandoc
                librsvg
              ];
            };
            # mkShellNoCC, because we do not want the cc from stdenv to leak into this shell
            default = pkgs.mkShellNoCC rec {
              name = "libvmm-dev";

              microkit-platform = microkit-platforms.${system} or (throw "Unsupported system: ${system}");

              env.MICROKIT_SDK = pkgs.fetchzip {
                url = "${microkit-url}/microkit-sdk-${microkit-version}-${microkit-platform}.tar.gz";
                hash =
                  {
                    aarch64-darwin = "sha256-MKtQyECOHpLQ/SQ6OTkZyRFY4ajFJsq9e0Zy/M8u9BY=";
                    x86_64-darwin = "sha256-rFL2S5UB14j8eSRyTWisYDeab5MClkxPUPUGmkdoWgQ=";
                    x86_64-linux = "sha256-C21EpS95KQ1Yw5KYumXqhSY4B9KOPiRY1Mt4k7n8shA=";
                    aarch64-linux = "sha256-S2oRumOiFO9NPkOOGA1Gj8MIPlzITblrMiehJccdwoM=";
                  }
                  .${system} or (throw "Unsupported system: ${system}");
              };

              nativeBuildInputs = with pkgs; [
                rust
                git
                zig
                qemu
                gnumake
                dosfstools
                gptfdisk
                # for git-clang-format.
                llvm.libclang.python
                llvm.lld
                llvm.libllvm
                clang-complete
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
        }
    );
  };
}
