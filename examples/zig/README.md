# Zig VMM example

This example aims to show using [Zig](https://ziglang.org/) to interact with
the VMM library. Although Zig is most-often thought of as a programming
language, the project actually provides three things:
1. The Zig programming language
2. The Zig build system
3. A portable drop-in C/C++ cross-compiler

This example makes use of all three. The VMM code itself is written in the
Zig programming langauge, it calls into `libvmm` which has been compiled using
the Zig C compiler. Building the example is done via the Zig build system.

## Building the example

You will first need Zig version 0.11.x (e.g 0.11.0 or 0.11.1) which can be
downloaded from [here](https://ziglang.org/download/).

```sh
zig build -Dsdk=/path/to/sdk
```

To view other options available when building the example (such as optimisation
level or the target Microkit configuration), run the following command:
```sh
zig build -Dsdk=/path/to/sdk -h
```

## Running the example

To also run the example via QEMU, run the following command:
```sh
zig build -Dsdk=/path/to/sdk qemu
```
