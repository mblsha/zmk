# ZMK Custom Keyboard Development Setup

This repository contains a complete ZMK development setup with custom drivers for a keyboard featuring:

- **Sharp Memory LCD display** (LS013B7DH03)
- **LRA haptic feedback** (DRV2605)
- **BlackBerry trackpad** for pointer input
- **Comprehensive CI/CD pipeline**
- **Unit tests** for custom drivers

## Hardware Features

### Display
- Sharp Memory LCD (128x128 pixels)
- SPI interface with EXTCOMIN signal
- Ultra-low power consumption
- Excellent outdoor visibility

### Haptic Feedback
- TI DRV2605 haptic motor driver
- Support for both LRA and ERM actuators
- Multiple waveform libraries
- Auto-calibration support
- I2C interface

### Pointing Device
- BlackBerry trackpad (optical)
- SPI interface with motion interrupt
- Configurable scaling and inversion
- Low power operation

## Quick Start

### Prerequisites

- Docker (for consistent build environment)
- Git
- Python 3.8+ (for pre-commit hooks)

### Setup

1. **Clone and initialize workspace:**
   ```bash
   git clone <your-repo-url>
   cd zmk
   ./zmk-docker.sh west init -l app/
   ./zmk-docker.sh west update
   ```

2. **Build firmware:**
   ```bash
   ./zmk-docker.sh west build -s app -b nice_nano_v2 -- -DSHIELD=my_keyboard
   ```

3. **Flash firmware:**
   - Copy `build/zephyr/zmk.uf2` to your nice!nano in bootloader mode

### Development Workflow

1. **Install pre-commit hooks:**
   ```bash
   pip install pre-commit
   pre-commit install -c .pre-commit-config-custom.yaml
   ```

2. **Run tests:**
   ```bash
   ./zmk-docker.sh west test -p -v app/tests/drivers_test
   ```

3. **Build and test in CI:**
   - GitHub Actions automatically builds firmware and runs tests on push/PR

## Project Structure

```
app/
├── boards/shields/my_keyboard/     # Custom keyboard definition
│   ├── Kconfig.defconfig          # Default configuration
│   ├── Kconfig.shield             # Shield enablement
│   ├── my_keyboard.conf           # Build configuration
│   ├── my_keyboard.keymap         # Key layout
│   ├── my_keyboard.overlay        # Hardware definition
│   └── my_keyboard.zmk.yml        # Metadata
├── drivers/                       # Custom drivers
│   ├── input/                     # Input devices
│   │   ├── blackberry_trackpad.c  # BlackBerry trackpad driver
│   │   ├── CMakeLists.txt
│   │   └── Kconfig
│   └── misc/                      # Miscellaneous devices
│       ├── drv2605.c              # Haptic driver
│       ├── CMakeLists.txt
│       └── Kconfig
├── dts/bindings/                  # Device tree bindings
│   ├── input/
│   │   └── blackberry,trackpad.yaml
│   └── misc/
│       └── ti,drv2605.yaml
└── tests/drivers_test/            # Unit tests
    ├── src/
    │   ├── main.c
    │   ├── test_drv2605.c
    │   └── test_blackberry_trackpad.c
    ├── boards/native_posix.overlay
    ├── CMakeLists.txt
    └── prj.conf
```

## Hardware Connections

### nice!nano v2 Pin Assignments

| Component | Connection | Pin |
|-----------|------------|-----|
| Display CS | SPI CS0 | P0.08 |
| Display EXTCOMIN | GPIO | P0.10 |
| Trackpad CS | SPI CS1 | P0.09 |
| Trackpad IRQ | GPIO | P0.07 |
| Haptic SDA | I2C SDA | P0.17 |
| Haptic SCL | I2C SCL | P0.20 |
| Haptic EN | GPIO | P0.06 |
| Row 0 | GPIO | P0.21 |
| Row 1 | GPIO | P0.20 |
| Col 0 | GPIO | P0.02 |
| Col 1 | GPIO | P0.03 |

## Custom Drivers

### BlackBerry Trackpad Driver

**Features:**
- SPI communication with optical trackpad
- Motion interrupt handling
- Configurable coordinate scaling/inversion
- Input subsystem integration
- Power management support

**Configuration:**
```dts
bb_trackpad: bb_trackpad@1 {
    compatible = "blackberry,trackpad";
    reg = <1>;
    spi-max-frequency = <1000000>;
    irq-gpios = <&pro_micro 7 GPIO_ACTIVE_LOW>;
    scale-x = <2>;
    scale-y = <2>;
    invert-y;
    status = "okay";
};
```

### DRV2605 Haptic Driver

**Features:**
- I2C communication with DRV2605
- Multiple actuator types (LRA/ERM)
- Waveform library support
- Auto-calibration
- Power management
- Custom waveform sequences

**API:**
```c
// Play single waveform
drv2605_play_waveform(dev, DRV2605_WAVEFORM_CLICK);

// Play sequence
uint8_t sequence[] = {1, 2, 3, 0};
drv2605_play_sequence(dev, sequence, 4);

// Stop playback
drv2605_stop(dev);
```

## Testing

### Unit Tests

Run native tests with:
```bash
./zmk-docker.sh west build -s app/tests/drivers_test -b native_posix -t run
```

Tests include:
- Driver initialization
- Hardware communication
- Power management
- Error conditions

### Hardware-in-Loop Testing

For testing with actual hardware:

1. **Flash test firmware:**
   ```bash
   ./zmk-docker.sh west build -s app -b nice_nano_v2 -- -DSHIELD=my_keyboard -DCONFIG_ZMK_USB_LOGGING=y
   ```

2. **Monitor logs:**
   ```bash
   # Connect to serial console
   minicom -D /dev/ttyACM0 -b 115200
   ```

3. **Test interactions:**
   - Press keys to verify matrix scanning
   - Move trackpad to check motion events
   - Trigger haptic feedback

## Troubleshooting

### Build Issues

**West initialization fails:**
```bash
# Clean and retry
rm -rf .west modules zephyr
./zmk-docker.sh west init -l app/
./zmk-docker.sh west update
```

**Driver compilation errors:**
- Check device tree syntax with `scripts/check-devicetree-syntax.sh`
- Verify Kconfig dependencies are enabled
- Review driver logs for initialization failures

### Hardware Issues

**Display not working:**
- Verify SPI connections (MOSI, SCLK, CS)
- Check EXTCOMIN signal wiring
- Ensure power supply is adequate

**Trackpad not responding:**
- Check IRQ GPIO connection
- Verify SPI CS and communication
- Review motion interrupt configuration

**Haptic not working:**
- Check I2C connection (SDA, SCL)
- Verify enable GPIO if used
- Review DRV2605 logs for init status

## Contributing

1. **Follow coding standards:**
   - Use clang-format for C code
   - Follow Zephyr device tree conventions
   - Include SPDX license headers

2. **Write tests:**
   - Add unit tests for new features
   - Test error conditions
   - Mock hardware interactions for CI

3. **Update documentation:**
   - Update this README for new features
   - Add inline code comments
   - Document configuration options

## CI/CD Pipeline

GitHub Actions automatically:
- **Builds firmware** for the custom keyboard
- **Runs unit tests** on native_posix
- **Checks code quality** (formatting, syntax)
- **Archives artifacts** (UF2, HEX files)

### Customizing CI

Edit `.github/workflows/build-and-test.yml` to:
- Add new build targets
- Include additional tests
- Deploy to different environments

## Upstreaming

To prepare code for upstream contribution:

1. **Split changes into logical commits:**
   ```bash
   git rebase -i HEAD~n  # Clean up commit history
   ```

2. **Create separate PRs for:**
   - Device tree bindings
   - Driver implementation
   - Tests and documentation

3. **Follow ZMK contribution guidelines:**
   - Ensure compatibility with existing shields
   - Add proper documentation
   - Include comprehensive tests

## License

This project is licensed under the Apache 2.0 License - see individual file headers for details.