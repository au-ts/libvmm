# CI for the VMM

This basic CI attempts to build and run systems using the VMM in order to give
the minimal assurance that at least the code is not completely broken. It is
mainly useful in the scenario where you change something and want to make sure
you have not inadvertently broken something unrelated.

## Running the CI locally

I have tried my best to make the CI avaiable to run locally. This is done with
two scripts:

1. `acquire_sdk.sh`
2. `build_examples.sh`

The first script is not necessary and is primarily for getting the GitHub CI
to download a pre-built seL4CP SDK. You can manually downlod a pre-built SDK
or build it yourself and then pass the path to the second script like the
following:
```sh
    $ ./build_examples.sh /path/to/sdk
```
