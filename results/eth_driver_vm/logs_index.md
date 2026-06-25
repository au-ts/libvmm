

### ethernet_vm_log_bad_checksum - 16/6
Shows the failing outgoing tcmp checksum. This was resolved by removing ifdef so that it would be calculated in software. Need to check if this can be done in hardware.

### ethernet_vm_log_ping_working_client1_crash
After fixing the checksum issue ping now works, but we have a crash on the PD caused by UBsan

### ethernet_vm_log_no_interrupts_no_eth
The spurious interrupts in debug mode are linked to the ethernet interface being up. Once this was deactivated they stopped

### ethernet_vm_log_faulting_PD_benchmark
Another UBsan issue. Just removed the compile flags as the code was correct.
