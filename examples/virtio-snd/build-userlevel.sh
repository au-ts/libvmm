#!/usr/bin/env nix-shell
#!nix-shell -i bash --pure
#!nix-shell -p patchelf pkgsCross.aarch64-multiplatform.alsa-lib pkgsCross.aarch64-multiplatform.buildPackages.gcc

if [ "$#" -ne 2 ]; then
    echo "usage: ./build-userlevel <gcc args> <elf file>"
fi

aarch64-unknown-linux-gnu-gcc $1 &&
patchelf --set-interpreter /lib64/ld-linux-aarch64.so.1 "$2"
