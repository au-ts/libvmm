# CI for the VMM

This basic CI attempts to build and run systems using the VMM in order to give
the minimal assurance that at least the code is not completely broken.

## Running the CI locally

If you are making changes to the VMM library itself, it is a good idea to
first test the changes locally before upstreaming the changes.

Make sure you have the dependencies listed in the top-level README, you will
also need the `expect` program installed.

On Ubuntu/Debian:
```sh
sudo apt install expect
```

On macOS:
```sh
brew install expect
```

If you are using the Nix shell then you will already have it installed.

You can then run the CI like so:
```sh
    $ ./ci/examples.sh /path/to/sdk
```

You should then get the `Passed all VMM tests` message.
