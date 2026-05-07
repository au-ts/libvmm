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
    sdfgen.url = "github:au-ts/microkit_sdf_gen/0.29.0";
    sdfgen.inputs.nixpkgs.follows = "nixpkgs";
    rust-overlay = {
      url = "github:oxalica/rust-overlay";
      inputs.nixpkgs.follows = "nixpkgs";
    };
    systems-ci.url = "github:au-ts/systems-ci/main";
    systems-ci.flake = false;
  };

  outputs = { nixpkgs, zig-overlay, rust-overlay, sdfgen, systems-ci, ... }:
    let
      microkit-version = "2.2.0";
      microkit-url = "https://github.com/seL4/microkit/releases/download/2.2.0/";
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
      packages = forAllSystems (
        system:
        let
          pkgs = import nixpkgs {
            inherit system;
          };

          linux-613 = pkgs.fetchFromGitHub {
            owner = "torvalds";
            repo = "linux";
            rev = "v6.13";
            hash = "sha256-FD22KmTFrIhED5X3rcjPTot1UOq1ir1zouEpRWZkRC0=";
          };

          linux-518 = pkgs.fetchFromGitHub {
            owner = "torvalds";
            repo = "linux";
            rev = "v5.18";
            hash = "sha256-EB4nc9eHNAY2bHZVAnTckh/WpUkZN5EUj3rXZFR0E0M=";
          };

          linux-aarch64-example-simple = (pkgs.pkgsCross.aarch64-multiplatform.callPackage ./tools/linux/linux.nix {}) {
            version = "5.18";
            src = linux-518;
            arch = "arm64";
            configFile = ./examples/simple/images/linux_config;
          };

          linux-aarch64-example-virtio = (pkgs.pkgsCross.aarch64-multiplatform.callPackage ./tools/linux/linux.nix {}) {
            version = "6.13";
            src = linux-613;
            arch = "arm64";
            configFile = ./examples/virtio/client_vm/linux_config;
          };

          allImages = pkgs.runCommand "all-images" { nativeBuildInputs = with pkgs; [ gnutar gzip ]; }
            ''
              mkdir -p $out

              mkdir -p $out/linux-aarch64-example-simple/
              cp ${linux-aarch64-example-simple}/* $out/linux-aarch64-example-simple/

              mkdir -p $out/linux-aarch64-example-virtio/
              cp ${linux-aarch64-example-virtio}/* $out/linux-aarch64-example-virtio/

              cd $out
              for f in linux-*; do tar cf - $f | gzip -9 > `basename $f`.tar.gz; done
            ''
          ;
        in
        {
          images = allImages;
        }
      );
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

          ts_ci = pkgs.callPackage "${systems-ci}/ts_ci/package.nix" {
            python3Packages = pkgs.python312Packages;
          };

          python = pkgs.python312.withPackages (ps: [
            pysdfgen
            ts_ci
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
                    aarch64-darwin = "sha256-UZBEwS3vAQqJe6Xj+13smJRS0RYfoc0uCK7hB8ujbvA=";
                    x86_64-darwin = "sha256-aE2mYToK2ne9vzw6d3YQDzJvhpnI8IHOR9+VqZxwlfY=";
                    aarch64-linux = "sha256-U1hA7Vk/TlSWgV7KiEeG7AkA7t5IR/x89mSE0YHBRNA=";
                    x86_64-linux = "sha256-dxPu2Q01qjKhME6Z6kgG4ASDUe12ytZmh5tCtFva/L0=";
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
