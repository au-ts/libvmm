# Purpose
A context file so it is easier for someone to see where I am up to and get progress on my thesis.

# Overall research goal
This thesis aims to improve the virtualisation performance of Microkit based systems through two objectives:

- Characterise: To measure and decompose the sources of I/O overhead in both the driver and client VM path of a Microkit-based system
- Optimise: To propose and evaluate candidate optimisations and quantify their effect on end-to-end I/O latency and throughput

# Current progress
Recently I have 
- Implemeted a unoptimised single vCPU wfi. This lets the guest sleep and allows the idle benchmark to run
- Added vm benchmark support. 
    - The vmm takes measurements and forwards them to the benchmark pd
    - Also running a simple script to read /proc/stat inside the vm to check user vs guest

The results and figures can be fold in this folder

# Next steps
Implementing WFI did not provide performance benifits but showed the guest is saturating at ~25mbps. 

1. Go through the UIO driver carefully to check any gotchas
2. Run a client and see if there is this performance degredation

# Timeline
Will update after the meeting with the team.

# Testing
I have created 8 macrobenchmarking senarios which will help me to understand the performance of virtualisation. 

Ethernet driver

| Test                                 | Rationale                                                                                                                                                                               | Order |
| ------------------------------------ | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ----- |
| seL4 native Ethernet performance     | The native benchmark of seL4 SDDF on the maaxboard and odroidc4. Comparable to work done on LionsOS paper 2025(which uses the maaxboard)                                                | 1     |
| Driver VM                            | Replace the Ethernet driver with a VM. It is given access to the device via passthrough (will discuss why multiplexing is a bad idea)                                                   | 2     |
| Linux driver native                  | Native performance of the the same linux driver as 2                                                                                                                                    | 3     |                                                                                                                                | 3    |

Client VM

| Test                                 | Rationale                                                               | Case |
| ------------------------------------ | ----------------------------------------------------------------------- | ---- |
| Mibench on linux                     | Run the Mibench test suite on native linux to get baseline measurements | 4    |
| VirtIO + Client                      | Use VirtIO to run the client in a VM with the native SDDF stack         | 5    |
| Native on SDDF (may be hard to port) |                                                                         | 6    |

VM

| Test                                                | Rationale                                              | Case |
| --------------------------------------------------- | ------------------------------------------------------ | ---- |
| Mibench client on VM with device passthrough        | Here the device is running as both a client and driver | 7    |
| Mibench client with separate VM for ethernet driver | Combination of cases 2 and 5                           | 8    |

# Microbenchmarks
To support this I am aiming to use microbenchmarks to measure specific code paths or events. These will depend on the results / investigation of the macrobenchamrks
