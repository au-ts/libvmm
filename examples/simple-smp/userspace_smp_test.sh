#
# Copyright 2025, UNSW
#
# SPDX-License-Identifier: BSD-2-Clause
#

#!/bin/bash

# This is a minimal sanity SMP test script for aarch64 Linux guests running on libvmm

echo "Simple SMP test for aarch64 Linux on libvmm"

echo -n "TEST 1: guest kernel can see more than 1 CPU..."
LOGICAL_CPU_LOW=$(cat /sys/devices/system/cpu/online | cut -d'-' -f1)
LOGICAL_CPU_HIGH=$(cat /sys/devices/system/cpu/online | cut -d'-' -f2)
NUM_CPUS=$(($LOGICAL_CPU_HIGH - $LOGICAL_CPU_LOW + 1))
if [ $NUM_CPUS -gt 1 ];
then
    echo "PASSED! Found $NUM_CPUS CPUs."
else
    echo "FAIL! Only found 1 CPU."
    killall yes
    exit 1
fi

# Now keep all CPUs busy to make sure the architectural timers' IRQ tick up
# on all CPUs
for i in $(seq 1 $NUM_CPUS)
do
    yes >/dev/null &
done

echo "TEST 2: arch timer IRQ ticks on all CPUs..."
# Filter for arch timer interrupts
# This is what it will looks like: ` 11: 2080 1291 1319 1542 GICv3 27 Level arch_timer`
PRE_WAIT_IRQS=$(cat /proc/interrupts | grep arch_timer | sed -e 's/ \+/ /g')

# Sleep for just a bit for the IRQs to tick up
sleep 0.1

# Then check again and compare, making sure they are greater.
POST_WAIT_IRQS=$(cat /proc/interrupts | grep arch_timer | sed -e 's/ \+/ /g')

echo "Pre-wait arch timers IRQ:  $PRE_WAIT_IRQS"
echo "Post-wait arch timers IRQ: $POST_WAIT_IRQS"

for i in $(seq 1 $NUM_CPUS)
do
    COLUMN=$(($i + 2))
    PRE_WAIT_IRQ=$(echo "$PRE_WAIT_IRQS" | cut -d' ' -f$COLUMN)
    POST_WAIT_IRQ=$(echo "$POST_WAIT_IRQS" | cut -d' ' -f$COLUMN)

    echo -n "checking CPU #$i..."

    if [ $POST_WAIT_IRQ -le $PRE_WAIT_IRQ ];
    then
        echo "FAIL! post $POST_WAIT_IRQ > pre $PRE_WAIT_IRQ is not true"
        killall yes
        exit 1
    else
        echo "OK: post $POST_WAIT_IRQ > pre $PRE_WAIT_IRQ"
    fi
done

echo "SMP TEST PASSED!"
killall yes
exit 0
