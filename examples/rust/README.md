# Rust VMM example

This example shows how to use the
[Rust programming language](https://www.rust-lang.org/) with the VMM libary.
This example is similar to the [simple example](../simple) in that it boots a
simple Linux guest.

The Rust VMM only supports the QEMU ARM virt platform as the goal of this
example is to simply show how to use the VMM library (despite being written in
C) and call it from a Rust program. This specific example uses manual FFI
bindings to call into the VMM library, but you could use
[bindgen](https://github.com/rust-lang/rust-bindgen) or make your own Rust-like
bindings. However for a minimal VMM as we have in this example, manual bindings
are sufficient.

It should also be noted that this example makes use of the
[rust-seL4](https://github.com/coliasgroup/rust-seL4/) project's support for
seL4CP, which is subject to change.

## Building the example

You will first need to have Rust installed, the instructions for this are [here](https://www.rust-lang.org/tools/install).

From here, you build the example with:
```sh
make SEL4CP_SDK=/path/to/sel4cp-sdk-1.2.6
```

## Running the example

You can build and run the example in a single command with:
```sh
make SEL4CP_SDK=/path/to/sel4cp-sdk-1.2.6 qemu
```
