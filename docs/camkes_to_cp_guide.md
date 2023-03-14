<!--
     Copyright 2023, UNSW (ABN 57 195 873 179)

     SPDX-License-Identifier: CC-BY-SA-4.0
-->

# CAmkES to seL4CP Transition Guide (Very Early Draft)

## Intro
<!-- @jade: write more stuff -->
This is a guide that aims to provide leads for those who are migrating applications from CAmkES to seL4CP. The guide takes two seL4CP applications as examples, and compares them with their corresponding example on CAmkES:
    - sDDF <!-- @jade: expand these -->
    - VirtIO Demo

A basic understanding of CAmkES and sel4cp might be needed before reading this documentation.

## Comparison

1. CAmkES uses auto-generated code to introduce or parameterise variables for the system (e.g. struct vswitch_mapping, VM_COMPOSITION_DEF, recv1_shmem_size), this is not the case in sel4cp VMM. We don't yet have a proper way to configure this on sel4cp VMM, so many of the variables and structures are hard-coded, this includes:

    - vswitch layouts (connections, MAC address, sharedringbuffer etc.)
    - VM dtb, currently all VMM shares the same dtb file, which makes it harder to do device passthrough and interrupt handling. To bypass this, you might need to hack the Makefile
    - the ability to switch VirtIO backend on and off.
    - other variable names.
    <!-- @jade: expand the bullet points -->

    We have not yet addressed this issue as we need more examples to determine what sel4cp actually needs.

2. CAmkES uses different kinds of connectors, such as seL4VirtQueues, to enable communication between components, which also add auto-generated codes into the systems. Some connectors, e.g. seL4VMDTBPassthrough, are unfortunately used as ways to hack the system. In sel4cp it uses channels to enable two protection domains to interact using either protected procedures or notifications.
<!-- @jade: expand this -->

3. inter-component transporting mechanisms on CAmkES is a virtqueue-like ring buffer library, which requires a corresponding connector(s). sel4cp uses shared ring buffers as well as channel
to achieve a similar functionality but with a way simpler implementation.
<!-- @jade: gives examples (sddf?) -->

4. TBD

# Reference
1. sel4cp manual: https://github.com/BreakawayConsulting/sel4cp/blob/main/docs/manual.md
2. CAmkES manual: https://github.com/seL4/camkes-tool/pull/82 (the best I can find)