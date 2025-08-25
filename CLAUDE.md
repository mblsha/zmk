# ZMK Custom Development Guide

This document explains how to build, test, and work with this custom ZMK firmware featuring Sharp Memory LCD, BlackBerry trackpad, and DRV2605 haptic feedback.

## Quick Start

### Building Firmware

Use the Docker wrapper for consistent builds:

```bash
# Build for nice!nano v2 with basic functionality
./zmk-docker.sh west build -s app -b nice_nano_v2 -- -DSHIELD=my_keyboard

# Clean build
./zmk-docker.sh rm -rf build && ./zmk-docker.sh west build -s app -b nice_nano_v2 -- -DSHIELD=my_keyboard

# The resulting firmware will be in: build/zephyr/zmk.uf2
```

### Testing

Run comprehensive test suites for all custom functionality:

```bash
# Quick unit tests for drivers
./zmk-docker.sh west build -s app/tests/drivers_test -b native_posix -t run

# Run comprehensive test suite
cd app/tests
./test-runner.sh all

# Run specific test categories
./test-runner.sh drivers                    # Driver unit tests only
./test-runner.sh behavioral                 # Behavioral integration tests
./test-runner.sh --performance              # Performance benchmarks
./test-runner.sh --verbose --coverage all   # Full test suite with coverage

# Run specific test scenarios
./test-runner.sh specific haptic-feedback/basic
./test-runner.sh specific trackpad-input/scroll-gestures
```

**Test Categories:**
- **Unit Tests**: Driver-level tests with mocking (BlackBerry trackpad, DRV2605 haptic, display integration)
- **Behavioral Tests**: ZMK integration tests (haptic feedback patterns, trackpad input processing, display widgets)
- **Integration Tests**: Multi-component system validation
- **Performance Tests**: Latency, throughput, and resource usage benchmarks

### CI/CD

GitHub Actions provides comprehensive automated testing:

**Quick Test (on every PR/push):**
- Builds firmware for nice!nano v2 with custom drivers
- Runs driver unit tests on native_posix
- Basic code quality checks and formatting validation
- Uploads firmware artifacts and test results

**Comprehensive Test (daily + `[test-all]` commits):**
- Full test suite execution (unit + behavioral + integration)
- Performance benchmarking and regression detection
- Code coverage analysis with detailed reporting
- Security scanning and static analysis

**Additional Jobs:**
- **Code Quality**: Static analysis, formatting checks, device tree validation
- **Security Analysis**: Sensitive data detection, dependency scanning
- **Documentation**: API documentation generation, example validation
- **Hardware Validation**: Ready for hardware-in-the-loop testing (when available)

**Test Triggers:**
```bash
# Run comprehensive tests on specific commits
git commit -m "feat: improve trackpad sensitivity [test-all]"

# Daily automated comprehensive testing via cron schedule
# Performance and regression testing weekly
```

## Code Organization

### Project Structure

```
/home/mblsha/src/zmk/
├── zmk-docker.sh              # Docker build wrapper
├── app/                       # ZMK application code
│   ├── boards/shields/my_keyboard/  # Custom keyboard definition
│   │   ├── Kconfig.shield           # Shield configuration
│   │   ├── Kconfig.defconfig        # Default config values
│   │   ├── my_keyboard.overlay      # Device tree hardware definition
│   │   ├── my_keyboard.keymap       # Key layout and bindings
│   │   └── my_keyboard.conf         # Build configuration
│   ├── drivers/                     # Custom drivers
│   │   ├── input/                   # Input device drivers
│   │   │   └── blackberry_trackpad.c  # BlackBerry trackpad driver
│   │   └── misc/                    # Miscellaneous drivers
│   │       └── drv2605.c           # DRV2605 haptic driver
│   ├── dts/bindings/               # Device tree bindings
│   │   ├── input/blackberry,trackpad.yaml      # Trackpad binding
│   │   ├── misc/ti,drv2605.yaml                # Haptic binding
│   │   └── vendor-prefixes.txt                 # Vendor prefixes
│   └── tests/drivers_test/         # Unit tests
│       ├── src/test_blackberry_trackpad.c     # Trackpad tests
│       └── src/test_drv2605.c                 # Haptic tests
├── .github/workflows/          # CI/CD configuration
│   └── build-and-test.yml     # GitHub Actions workflow
└── .pre-commit-config.yaml    # Code quality checks
```

### Hardware Configuration

The keyboard is configured in `app/boards/shields/my_keyboard/`:

**Hardware Features:**
- **Board**: nice!nano v2 (nRF52840)
- **Matrix**: 2x2 key matrix (demo configuration)
- **Display**: Sharp Memory LCD LS013B7DH03 (128x128px) via SPI
- **Trackpad**: BlackBerry optical trackpad via SPI
- **Haptics**: DRV2605 LRA driver via I2C

**Pin Assignments:**
```
GPIO Pins:
- Rows: P0.21, P0.20 (with pull-down)
- Cols: P0.02, P0.03
- LCD CS: P0.08, EXTCOMIN: P0.10
- Trackpad CS: P0.09, IRQ: P0.07
- Haptic Enable: P0.06
- I2C: P0.17 (SDA), P0.20 (SCL)
- SPI: P0.17 (MOSI), P0.20 (SCK)
```

### Custom Drivers

#### BlackBerry Trackpad (`app/drivers/input/blackberry_trackpad.c`)
- SPI-based optical pointing device
- Motion detection with X/Y coordinate reporting
- Configurable sensitivity and scaling
- Interrupt-driven for low power consumption
- Integration with ZMK input subsystem

**Key Features:**
- Motion threshold detection
- Coordinate scaling for different resolutions
- Power management support
- Zephyr device model compliance

#### DRV2605 Haptic Driver (`app/drivers/misc/drv2605.c`)
- I2C-based LRA (Linear Resonant Actuator) driver
- Auto-calibration support
- Waveform library with 123+ effects
- Sequence playback capability
- Power management integration

**Key Features:**
- Automatic LRA calibration
- Built-in waveform library
- Custom sequence support
- Real-time trigger (RTP) mode
- Comprehensive error handling

### Device Tree Bindings

Located in `app/dts/bindings/`:

- `input/blackberry,trackpad.yaml` - BlackBerry trackpad configuration
- `misc/ti,drv2605.yaml` - DRV2605 haptic driver configuration

Properties include SPI/I2C configuration, GPIO assignments, timing parameters, and device-specific settings.

### Testing Infrastructure

Unit tests in `app/tests/drivers_test/`:

- **Native POSIX target** for emulated testing
- **Mock devices** for driver testing without hardware
- **Automated testing** via GitHub Actions
- **Coverage reporting** for driver functionality

Test structure:
```c
// Example test case
void test_trackpad_motion_detection(void) {
    // Setup mock device
    // Simulate motion input
    // Verify coordinate output
    // Check power state
}
```

## Build System Integration

### Kconfig System
- `app/drivers/Kconfig` - Driver configuration options
- `app/boards/shields/my_keyboard/Kconfig.defconfig` - Shield defaults
- Conditional compilation for optional features

### CMake Integration
- `app/drivers/CMakeLists.txt` - Driver build rules
- `app/CMakeLists.txt` - Main application integration
- Automatic driver discovery and compilation

### West Workspace
- `west.yml` - Workspace configuration
- Module dependencies (Zephyr, HAL drivers, etc.)
- Version pinning for reproducible builds

## Development Workflow

### Adding New Features

1. **Hardware Definition**: Update device tree overlay
2. **Driver Implementation**: Add driver to appropriate subdirectory
3. **Binding Creation**: Define device tree binding YAML
4. **Integration**: Update CMakeLists.txt and Kconfig
5. **Testing**: Create unit tests in drivers_test
6. **Documentation**: Update this file

### Debugging

**Build Issues:**
```bash
# Verbose build output
./zmk-docker.sh west build -s app -b nice_nano_v2 -- -DSHIELD=my_keyboard -v

# Check device tree compilation
./zmk-docker.sh west build -s app -b nice_nano_v2 --cmake-only -- -DSHIELD=my_keyboard
```

**Runtime Issues:**
- Enable USB logging via `zmk-usb-logging` snippet
- Use RTT logging for wireless debugging
- Check power management states

### Configuration Options

Key Kconfig options in `my_keyboard.conf`:

```kconfig
# Display support
CONFIG_LVGL=y
CONFIG_ZMK_DISPLAY=y

# Custom drivers (currently disabled for stable build)
# CONFIG_BLACKBERRY_TRACKPAD=y
# CONFIG_DRV2605=y

# Power management
CONFIG_ZMK_SLEEP=y
CONFIG_ZMK_IDLE_SLEEP_TIMEOUT=900000

# Bluetooth
CONFIG_ZMK_BLE=y
CONFIG_BT_CTLR_TX_PWR_PLUS_8=y
```

## Hardware Variants

### Current Configuration (Stable)
- Basic 2x2 matrix with Sharp Memory LCD
- LVGL graphics for status display
- Size: ~534KB firmware

### Full Configuration (Development)
- All peripherals enabled (display + trackpad + haptics)
- Complete input processing pipeline
- Enhanced user interaction
- Size: ~600KB+ firmware (estimated)

## Troubleshooting

### Common Build Errors

**"offsets.h not found"**
- Caused by circular dependency in driver compilation
- Solution: Build offsets target first or disable custom drivers temporarily

**"Unknown vendor prefix"**
- Missing vendor prefix in `dts/bindings/vendor-prefixes.txt`
- Add required vendor (e.g., "blackberry BlackBerry Limited")

**Kconfig dependency loops**
- Circular dependencies in Kconfig files
- Review and remove duplicate or conflicting configurations

### Performance Optimization

- Use `-Os` optimization level for size
- Enable link-time optimization (LTO)
- Remove unused features via Kconfig
- Optimize device tree for minimal footprint

## Contributing

1. Follow ZMK coding standards
2. Add unit tests for new drivers
3. Update device tree bindings
4. Test on hardware before submitting
5. Update documentation

## License

This code follows ZMK's MIT license. Custom drivers include appropriate SPDX headers and attribution.