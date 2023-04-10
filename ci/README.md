# CI for the VMM

This basic CI attempts to build and run systems using the VMM in order to give
the minimal assurance that at least the code is not completely broken. It is
mainly useful in the scenario where you change something and want to make sure
you have not inadvertently broken something unrelated.

## Running the CI locally

Make sure you have the dependencies listed in the top-level README, you will
also need the `expect` program installed (`sudo apt install expect`).

You can then run the CI like so:
```sh
    $ ./examples.sh /path/to/sdk
```
