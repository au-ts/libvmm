<!--
     Copyright 2026, UNSW
     SPDX-License-Identifier: CC-BY-SA-4.0
-->

# Intel APICv kernel patch

To build the Microkit SDK with a kernel patch that enable Intel APICv operation,
follow the standard steps for building the SDK
[here](https://github.com/seL4/microkit/blob/main/DEVELOPER.md).

At the "Building the SDK" step, you will need to clone seL4 v16:
```
git clone https://github.com/seL4/seL4.git
cd seL4
git checkout 16.0.0
```

Apply the APICv patch:
```
git apply apicv.patch
```

Enable APICv operation in Microkit's `build_sdk.py`:
```diff
DEFAULT_KERNEL_OPTIONS_X86_64: KERNEL_OPTIONS = {
    "KernelPlatform": "pc99",
    "KernelX86MicroArch": "generic",
    "KernelIOMMU": True,
+    "KernelIntelApicv": True,
}
```

Finally build Microkit with the custom kernel.