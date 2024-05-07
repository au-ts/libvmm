# Benchmarking!

This is a benchmarking example utilising the various SPH benchmarks done by Martins and Pinto in the paper titled:
'Shedding Light on Static Partitioning Hypervisors for Arm-based Mixed-Criticality Systems'

This benchmarking suite adds some funcationlity to the vmm to also benchmark the various syscalls the vmm needs to call.

These alterations shouldn't affect the performance of the vmm without the invocation of some compiler flags, but either way this shouldn't be merged into mainline.

There are a few tests detailed below, the benchmarks currently work best on the following 2 platforms

* odroidc4
* maaxboard

with partial support for the

* zcu102

although that one is annoying to use with machine queue at the time of writing this so it is recommended to avoid (automation becomes tedious).

# Tests

To benchmark the VMM there are various programs one can build and run available in this folder, they are as follows:

* IRQ Latency Benchmark
* NO-OP Syscall Benchmark
* MIBENCH Benchmark

They are detailed below

## IRQLAT (IRQ Latency Benchmark)

The following benchmark times the latency of a timer generated IRQ, also displaying various PMU variables along the way. It is a baremetal application.

## NO-OP (NO-OP Syscall Benchmark)

The following benchmark times the amount of cycles a NO-OP syscall (a syscall that immediately returns from the vmm) takes to complete, this requires ignore a NULL dereference,
and hence a special compiler flag is added

## MIBENCH (MIBENCH Benchmark)

This is a linux image with a CPU benchmark suite application on the ROOT FS, used for testing CPU benchmarks in a virtualised large operating system

# To Run the Benchmark

First you need to have some pre-requisites

## Pre-reqs

1. Make sure you have machine queue installed and working, and also added to the path with the symlink 'mq' (as described in the README for machine_queue found here: https://github.com/seL4/machine_queue/)

2. Make sure you have all the pre-requisites required for running libvmm, easiest way to do this is to follow the README instructions and get the "simple" example up and running

3. IMPORTANT: You must alter microkit to have a speed-up fix that drastically improves perfromance, to do this you must:

    i. Move the variables called `have_reply` and `reply_tag` out of the function `handler_loop` within the file `libmicrokit/src/main.c` to be global variables at the top of the file instead

    ii. Add them to the file `libmicrokit/src/include/microkit.h` as extern variables, similar to the already existing variables such as `signal_cap` and `signal_msg`

4. Build Microkit

5. Change the "MICROKIT_SDK_PATH" variable the at the top of `bench.sh` to be the path of where your built sdk is.

Now you should be able to run the benchmark

The easiest ways to run the benchmark is to use the supplied script file: `bench.sh`

This file can be run as so:

`./bench.sh <board> <test> (options)`

for example:

`./bench.sh odroidc4 irqlat` will compile and run the above mentioned IRQ latency benchmark for the odroidc4, then use machine queue to run the benchmark on any available machine

These are the available options:

`--benchmark` - also run the syscall benchmarking additions implemented into libvmm, to time the cycles taken for any traps into the hypervisor (NOTE: For this to work, the guest needs to dereference a bad address when it is done executing, so that the hypervisor knows that the guest is done (not elegant but it works))

`--no-recompile` - don't fully recompile everything, saves some time if you are only making changes to the vmm, but making changes to baremetal guests will not cause them to be automatically recompiled, hence the scripts always recompiles everything

`--no-size-patch` - see below

## SIZE PATCH

There is a large and concerning bug currently in the VMM that causes guests that are too small to not boot, immediately erroring out. This has been theorised to be a caching issue but nothing conclusive has been found so far. This means that baremetal guests, such as the ones used for benchmarking, need to be patched to become a large size so as to not break. This is done by using a very useful python script found in this folder called `wtf.py` and is automatically run after compilation, unless the `--no-size-patch` flag is specified.

## Output Files

Once the script finishes running (if run with the `--benchmark` flags, the machine queue will auto quit after the benchmarking is done, otherwise you will need to press CTRL+C to stop the machine queue), the output will be saved in a file called logfile_<board>_<test> (this is .gitignored). These contain all the output of the testing done, there are various python scripts included to parse these, and transform them into excel files.

Included is a requirements.txt that has all the needed libraries for running the scripts, although I'd recommend running them in a pyenv so as to not pollute any global installs, as so:

`python3.9 -m venv pyenv`
`./pyenv/bin/pip install -r requirements.txt`
`./pyenv/bin/python <parsing_python_script> <logfile>`

Some of the python files have some required arguments, they are detailed below:

no-op parser: `./pyenv/bin/python no-op_logfile_parser.py <logfile>`

irqlat parser: `./pyenv/bin/python irqlat_logfile_parser.py <logfile> <excel_output_name>`

benchmarking parser (for use with running bench.sh with --benchmark): `./pyenv/bin/python bench_results_parser.py <print or excel> <excel_output_name>`
 - This one can be run in either print or excel mode, where printing shows a summary of the data in the console, while excel saves an excel file with many sheets of the raw formatted data.