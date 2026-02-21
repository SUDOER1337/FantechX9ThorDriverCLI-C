# Fantech X9 Thor Driver CLI - C Implementation

A C implementation of Fantech X9 Thor RGB gaming mouse driver, **inspired by** [GuessWhatBBQ's FantechX9ThorDriver](https://github.com/GuessWhatBBQ/FantechX9ThorDriver). This gives minimalist performance of C with CLI interface.

## Project Structure (after complied)

```
fantech-driver-c/
├── src/
│   ├── main.c              # Application entry point
│   ├── usb_driver.c        # USB communication layer
│   ├── usb_driver.h        # USB driver header
│   ├── config.c            # Configuration management
│   ├── config.h            # Configuration header
│   ├── cli.c               # Command-line interface
│   ├── cli.h               # CLI header
│   └── protocol.h          # USB protocol definitions
├── Makefile                # Build configuration
├── README.md               # This file
└── bin/                    # Compiled binary (created by make)
```

## About This Implementation

This C implementation was created from scratch by analyzing the Python version's USB protocol with SWE-1.5, testing with GDB, and command-line interface. The development process involved:

1. **Protocol Analysis**: Reverse-engineering the USB communication patterns
2. **Configuration Compatibility**: Ensuring 100% INI file format compatibility
3. **CLI Parity**: Implementing all command-line options and behaviors
4. **Performance Optimization**: Leveraging C's low-level capabilities

## Requirements

### System Dependencies
- **libusb-1.0**: USB device communication library
- **gcc**: C compiler (or compatible)
- **make**: Build system

### Installation on Ubuntu/Debian
```bash
sudo apt update
sudo apt install build-essential libusb-1.0-0-dev
```

### Installation on Fedora/CentOS
```bash
sudo dnf install gcc make libusb1-devel
```

### Installation on Arch Linux
```bash
sudo pacman -S base-devel libusb
```

## Building

### Quick Build
```bash
make
```

### Debug Build
```bash
make debug
```

### Release Build
```bash
make release
```

### Install System-wide
```bash
sudo make install
```

## Usage

### Basic Commands

```bash
# Find and check device
fantech-driver find

# Set DPI for profile 2 to 1600
fantech-driver set-dpi 1600 1

# Set RGB color for profile 1 to red
fantech-driver set-color 1 255 0 0

# Set lighting mode to static
fantech-driver set-rgb Static 6

# Apply configuration from file
fantech-driver preset --conf ~/.config/fantech-x9-thor/gaming.conf
```

### Configuration Management

```bash
# Show configuration information
fantech-driver config info

# Migrate local config to user directory
fantech-driver config migrate

# List available presets
fantech-driver config presets
```

## Configuration Files

The driver automatically detects configuration files in this priority order:

1. **Custom path** (specified with `--conf`)
2. **Local config**: `driver.conf` (current directory)
3. **User config**: `~/.config/fantech-x9-thor/config.conf` (recommended)

### Example Configuration

```ini
[Active_Profile]
profile = 1

[Profile_DPIs]
profile_1 = 200
profile_2 = 600
profile_3 = 1200
profile_4 = 1600
profile_5 = 2400
profile_6 = 4000

[Profile_Colors]
profile_1 = rgb(255,73,0)
profile_2 = rgb(255,73,0)
profile_3 = rgb(255,53,0)
profile_4 = rgb(255,73,0)
profile_5 = rgb(255,73,0)
profile_6 = rgb(255,73,0)

[Color_Scheme]
type = Static
duration = 6

[Cyclic_Colors]
yellow = 0
blue = 0
violet = 0
green = 0
red = 0
cyan = 0
white = 0
```

## USB Setup

Since the driver needs direct USB access, appropriate permissions are required.

### UDEV Rules (Recommended)

Create a udev rule for automatic permissions:

```bash
echo "SUBSYSTEMS==\"usb\", ATTRS{idVendor}==\"18f8\", ATTRS{idProduct}==\"1086\", GROUP=\"users\", MODE=\"0660\"" | sudo tee /etc/udev/rules.d/50-fantechdriver.rules
```

Then reload udev rules:

```bash
sudo udevadm control --reload-rules
sudo udevadm trigger
```

### Alternative: Run as Root (Not Recommended)

```bash
sudo fantech-driver find
```


## Development

### Build Targets

```bash
make help           # Show all available targets
make test-compile   # Test compilation without linking
make clean          # Remove build artifacts
make check-deps     # Check for required dependencies
```

### Debugging

For debugging with gdb:

```bash
make debug
gdb ./bin/fantech-driver
```
## Troubleshooting

### Device Not Found
- Check USB connection
- Verify udev rules are installed
- Try running with `sudo` for testing

### Permission Denied
- Install udev rules (see USB Setup section)
- Add user to appropriate groups
- Check device permissions with `lsusb`

### Build Errors
- Ensure libusb-1.0 development headers are installed
- Check gcc and make are available
- Run `make check-deps` to verify dependencies

## License

This project is released under the MIT License.

### MIT License

Copyright (c) 2025 Fantech X9 Thor Driver C Implementation

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

### Attribution

This implementation is **inspired by** the original Python Fantech X9 Thor Driver. The original Python version served as the reference for USB protocol analysis, configuration format, and feature set. This C implementation was written from scratch to provide performance and deployment benefits while maintaining 100% functional compatibility.

## Contributing

Contributions are welcome! Please ensure:

1. Code follows the existing style
2. Memory management is handled properly
3. Error handling is comprehensive

## Roadmap

- [ ] Complete preset system implementation
- [ ] Add TUI interface
- [ ] Enhanced error reporting
- [ ] Systemd service integration
- [ ] Auto-detection of device changes
