# Copyright 2024, UNSW
#
# SPDX-License-Identifier: BSD-2-Clause

let
    pkgs = import <nixpkgs> {};
in
  pkgs.mkShell {
    buildInputs = with pkgs.buildPackages; [
        texlive.combined.scheme-full
        pandoc
        librsvg
    ];
}
