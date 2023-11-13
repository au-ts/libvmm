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

At the moment, Zig still under heavy development and hence this example depends
on the 'master' version of Zig for now. This example has been built using
`0.12.0-dev.1533+b2ed2c4d4`, so anything equal to or above that version should work.

You can download Zig [here](https://ziglang.org/download/).

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

You should see the following output after a couple of seconds:
```
Welcome to Buildroot
buildroot login:
```

The login is `root`. There is no password necessary. After logging in, you will
see the console prompt where you can input commands:
```
Welcome to Buildroot
buildroot login: root
#
```
