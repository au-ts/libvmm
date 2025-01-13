# Notes of creating this example

- Disabling `clock-controller` to bypass the system freezing issue
- Building rootfs with `hvc0` device for log prompt
- Adding `serial_virt_rx` for debugging
- ~~using buildroot config for simple example and replacing `ttymxc0` with `hvc0`~~ (missing ncurses library???)
- Intercepting requests to clock controller: This solves the issue. I guess the driver needs to check if certain clock configurations are satisfied.
- Integrating UIO modules into the kernel


