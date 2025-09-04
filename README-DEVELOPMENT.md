# ZMK Custom Keyboard Development Setup

This repository includes optional custom drivers and a sample shield for prototyping:

- Sharp Memory LCD display integration via existing Zephyr driver
- TI DRV2605 haptic driver (I2C)
- BlackBerry optical trackpad driver (SPI)

Use `zmk-docker.sh` to run west commands in a consistent container.

Quick start:

```
./zmk-docker.sh west init -l app/
./zmk-docker.sh west update
./zmk-docker.sh west build -s app -b nice_nano_v2 -- -DSHIELD=my_keyboard
```

Unit tests (native_posix):

```
west build -s app/tests/drivers_test -b native_posix -t run
```

