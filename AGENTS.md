# Build and Test Guide (Agents)

This repo contains optional custom drivers (BlackBerry trackpad + TI DRV2605) and a sample shield. This guide shows how to build and test them quickly in Docker, and how to enable the features when needed.

## Prerequisites
- Docker installed and usable from your shell
- Internet access (for `west update`)

## Initialize Workspace (one‑time)
```bash
./zmk-docker.sh west init -l app/
./zmk-docker.sh west update
```

## Build Firmware (sample shield)
Builds ZMK for `nice_nano_v2` using the included `my_keyboard` shield. This shield includes a simple 2x2 matrix and a Sharp Memory LCD node.
```bash
./zmk-docker.sh west build -s app -b nice_nano_v2 -- -DSHIELD=my_keyboard
# Artifacts: build/zephyr/zmk.uf2, zmk.hex
```

Tip: If Docker logs show a git “safe.directory” warning, it’s safe to ignore. If needed, run inside the container: `git config --global --add safe.directory /work`.

## Enable Custom Drivers (real hardware)
The drivers are compiled only when their Kconfig symbols are set, and they require DT nodes in your overlay.

- Enable via Kconfig at build time (optional for compile, required for tests):
```bash
-DCONFIG_DRV2605=y -DCONFIG_BLACKBERRY_TRACKPAD=y
```
- Add DT nodes for your hardware (example snippets):
```dts
/* DRV2605 on I2C */
&i2c0 {
    status = "okay";
    drv2605@5a {
        compatible = "ti,drv2605";
        reg = <0x5a>;
        enable-gpios = <&gpio0 15 GPIO_ACTIVE_HIGH>; // optional
        library = <6>;           // example: LRA library
        actuator-type = <1>;     // 0=ERM, 1=LRA
        rated-voltage = <2000>;  // mV
        overdrive-voltage = <2500>;
        auto-calibration;
        status = "okay";
    };
};

/* BlackBerry trackpad on SPI */
&spi1 {
    status = "okay";
    cs-gpios = <&gpio0 9 GPIO_ACTIVE_LOW>; // pick a CS pin
    bb_tp@0 {
        compatible = "blackberry,trackpad";
        reg = <0>;
        spi-max-frequency = <4000000>;
        irq-gpios = <&gpio0 2 GPIO_ACTIVE_HIGH>;
        shutdown-gpios = <&gpio0 3 GPIO_ACTIVE_HIGH>; // optional
        invert-x; // example optional tuning
        status = "okay";
    };
};
```

## Unit Tests (native_posix)
A lightweight test suite validates that enabled drivers initialize and that DT nodes appear under native emulation.

Build and run:
```bash
west build -s app/tests/drivers_test -b native_posix -p -- \
  -DCONFIG_ASSERT=y -DCONFIG_DRV2605=y -DCONFIG_BLACKBERRY_TRACKPAD=y
west build -t run
```
Notes:
- The test overlay (`app/tests/drivers_test/boards/native_posix.overlay`) creates emulated I2C/SPI nodes only when corresponding configs are enabled.
- Tests are scaffolds (init checks and placeholders) designed for CI gating and local sanity.

## Dev Checks
Basic sanity scripts for device tree and driver headers:
```bash
# Check DT files
scripts/check-devicetree-syntax.sh app/boards/shields/my_keyboard/*.overlay app/dts/bindings/**/*.yaml

# Check driver source headers and patterns
scripts/check-driver-includes.sh app/drivers/input/blackberry_trackpad.c app/drivers/misc/drv2605.c
```

## Clean/Rebuild
```bash
rm -rf build
./zmk-docker.sh west build -s app -b nice_nano_v2 -- -DSHIELD=my_keyboard
```

## File Map (key additions)
- Drivers: `app/drivers/input/blackberry_trackpad.c`, `app/drivers/misc/drv2605.c`
- Bindings: `app/dts/bindings/input/blackberry,trackpad.yaml`, `app/dts/bindings/misc/ti,drv2605.yaml`
- Shield: `app/boards/shields/my_keyboard/*`
- Tests: `app/tests/drivers_test/*`
- Tools: `zmk-docker.sh`, `scripts/check-*.sh`
