# Guide of Device Passthrough on seL4CP VMM

This is a guide for whom wants to do device passthrough (on seL4CP VMM) and doesn't want to spend time figuring out all these issues that I have previously been struggling with. The content of this guide will eventually be merged into the seL4CP VMM manual, but currently I am too lazy to do that.

The target platform of this document is Odroid-C4 and QEMU, though you might find it useful for other platform.

## Interpreting Device Trees

Let us take ethernet devices as an example.

This is an ethernet device node in the DTS file:
```c
ethernet@ff3f0000 {
    compatible = "amlogic,meson-g12a-dwmac\0snps,dwmac-3.70a\0snps,dwmac";
    reg = <0x00 0xff3f0000 0x00 0x10000 0x00 0xff634540 0x00 0x08>;
    interrupts = <0x00 0x08 0x04>;
    interrupt-names = "macirq";
    clocks = <0x02 0x26 0x02 0x02 0x02 0x0d 0x02 0x02>;
    clock-names = "stmmaceth\0clkin0\0clkin1\0timing-adjustment";
    rx-fifo-depth = <0x1000>;
    tx-fifo-depth = <0x800>;
    /* make sure this is not "disabled" */
    status = "okay";
    power-domains = <0x03 0x06>;
    pinctrl-0 = <0x07 0x08>;
    pinctrl-names = "default";
    phy-mode = "rgmii";
    phy-handle = <0x09>;
    amlogic,tx-delay-ns = <0x02>;

    mdio {
        #address-cells = <0x01>;
        #size-cells = <0x00>;
        compatible = "snps,dwmac-mdio";
        phandle = <0x13>;
    };
};
```

As you can see in the `reg` property, the memory region for this device should be:
```xml
<memory_region name="eth" size="0x10000" phys_addr="0xff3f0000" />
```
Note that we ignore the second memory region since it is part of `bus@ff600000`, and we must passthrough the bus as well.

From the `interrupts` property, we can also figure out the interrupt number and the nature of this interrupt. This `interrupts` property has three cells:

- the first cell defines the index of the interrupt within the controller. The controller for this interrupt is `interrupt-controller@ffc01000` (configured by the `interrupt-parent` property within the root node of the device tree), which is a GIC. For GIC interrupts, 0 indicates SPI interrupts, 1 indicates PPI interrupts. This impacts the offsets added to translate the interrupt number (16 for SPI, 32 for non-SPI).

- the second cell is the interrupt number.

- the 3rd cell is the flags, encoded as follows:
	* bits[3:0] trigger type and level flags.
		- 1 = low-to-high edge triggered
		- 2 = high-to-low edge triggered (invalid for SPIs)
		- 4 = active high level-sensitive
		- 8 = active low level-sensitive (invalid for SPIs).
    * bits[15:8] PPI interrupt cpu mask.

This tells us that the interrupt number the seL4 kernel sees is 40, and the trigger mode if this interrupt is level triggering (the default trigger mode for IRQ configuration on seL4CP):

```xml
<irq irq="40" id="5" />
```

From the `phy-handle` property we find that the physical layer that this ethernet device communicates with has a `phandle` number 0x09, which lead us to the node of the physical layer:

```c
ethernet-phy@0 {
    reg = <0x00>;
    max-speed = <0x3e8>;
    reset-assert-us = <0x2710>;
    reset-deassert-us = <0x13880>;
    reset-gpios = <0x14 0x0f 0x07>;
    interrupt-parent = <0x15>;
    interrupts = <0x1a 0x08>;
    phandle = <0x09>;
};
```

This is also part of `bus@ff600000` and we don't need to add any memory region for it. The `interrupt-parent` property in this device node is 0x15, indicating that the interrupt controller is `interrupt-controller@f080`:
```c
interrupt-controller@f080 {
    compatible = "amlogic,meson-sm1-gpio-intc\0amlogic,meson-gpio-intc";
    reg = <0x00 0xf080 0x00 0x10>;
    interrupt-controller;
    #interrupt-cells = <0x02>;
    amlogic,channel-interrupts = <0x40 0x41 0x42 0x43 0x44 0x45 0x46 0x47>;
    phandle = <0x15>;
};
```

Let us see what the documentation of this device says:
> Meson SoCs contains an Amlogic Meson GPIO interrupt controller (GPIOIC) which is able to watch the SoC pads and generate an interrupt on edge or level. The controller is essentially a 256 pads to 8 or 12 GIC interrupt multiplexer, with a filter block to select edge or level and polarity. It does not expose all 256 mux inputs because the documentation shows that the upper part is not mapped to any pad. The actual number of interrupts exposed depends on the SoC.

Okay, long story short, this means that each number in the `amlogic,channel-interrupts` property is a GIC interrupt number. There are 8 numbers, so each GIC interrupt is responsible for 32 GPIOIC interrupts.

The `interrupts` property in `ethernet-phy@0` has two cells. The first cell is the interrupt number and the second is the flags. We can see that 0x1a is less than 32, so the corresponding GIC interrupt number for this GPIOIC interrupt number is `0x40`. The trigger mode is `0x08`, which means level sensitive, voilà:
```xml
<!-- don't forget to add the offset: 0x40 + 32 -->
<irq irq="96" id="6" />
```

Now we can properly configure the memory regions and IRQs for our passthrough devices. In some weird situations, the DTS file might not be correct. You can always check what the Linux guest actually sees by dumping the DTS in the Linux guest:
```shell
$ dtc -I fs -O dts /proc/device-tree
```

and the interrupt info:
```shell
$ cat /proc/interrupts
```

## Interrupt routing in the VMM
TBD(@jade)

## QEMU Device Passthrough
TBD(@jade)

## Current issue with DMA
TBD(@jade)

## Device Tree Overlays
TBD(@jade)

## Reference
TBD(@jade)