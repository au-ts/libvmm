<!--
     Copyright 2026, UNSW
     SPDX-License-Identifier: CC-BY-SA-4.0
-->

# CI for the VMM

## Testing code

Each example system as part of CI is compiled and run either in a simulator
(QEMU) or on real hardware (remotely in the Trustworthy Systems lab).

On each commit and pull request, the build and simulation tests are checked. The
hardware tests do not happen unless an explicit 'hardware test' label is added
to a pull request. This is done to not overwhelm our supply of hardware.

> [!NOTE]
> Whilst the CI scripts are able to run machines on real hardware with the
> Machine Queue scripts in our laboratory, at the moment we have yet to set up
> Github Actions to have access to our boards for libVMM, so no CI hardware
> tests happen at the moment

### Setup

Install the `ts_ci` library from `systems-ci`.

```sh
$ pip install git+https://github.com/au-ts/systems-ci#subdirectory=ts_ci
```

### Builds

You can reproduce what the CI builds with:

```sh
./ci/build.py /path/to/microkit/sdk
```

You will need to provide the path to your Microkit SDK.

There are various options to speed up development, such as only compiling a
particular example or for a particular board.

For example, if you were working on virtio example system, you might want to
instead run:

```sh
./ci/build.py /path/to/sdk --examples virtio
```

### Runtime

There are two kinds of runtime tests, simulation via QEMU and hardware tests on
actual boards.

Simulation tests can be run on your development machine but hardware tests
obviously require actual hardware.

After you've run the [build script](#builds) you can run everything with images
with:
```sh
./ci/run.py
```

There are various options, such as running tests regarding a specific example:
```sh
./ci/run.py --example <EXAMPLE>
```

All the options can be found with:
```sh
./ci/run.py --help
```

Alternatively, one can invoke a test script directly.

```sh
./ci/tests/buildroot_login.py --help
```

#### Running with QEMU

If you do not have access to hardware, you can run all the simulation tests with
QEMU:
```sh
./ci/run.py --only-qemu
```

## Style

The CI runs a style check on any changed files and new files added in each
GitHub pull request.

### C code

We use `clang-format`, following the style guide in `.clang-format` for C code.

#### Installation

On macOS: `brew install clang-format`.

On apt: `sudo apt install clang-format`.

On Nix: `nix develop` in the root of the repository.

#### Using

You can reproduce the style check by doing the following, which prints the diff
of your committed changes against the main branch.

```sh
git clang-format --diff main
```

You can also auto-format changed files by omitting the `--diff` argument.

```sh
git clang-format main
```

Omitting the branch name runs the formatter on any staged files.
