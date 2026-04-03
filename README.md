# Fantech X9 Thor Driver CLI

A C implementation of Fantech X9 Thor RGB gaming mouse driver with command-line interface.

## Requirements

- libusb-1.0 development headers
- gcc and make

## Quick Installation

```bash
# Ubuntu/Debian
sudo apt update && sudo apt install build-essential libusb-1.0-0-dev

# Arch Linux
sudo pacman -S base-devel libusb

# Build
make

# Install system-wide (optional)
sudo make install
```

## Basic Usage

```bash
# Find and check device
fantech-driver find

# Set DPI for profile 1 to 1600
fantech-driver set-dpi 1600 0

# Set RGB color for profile 1 to red
fantech-driver set-color 1 255 0 0

# Set lighting mode to static
fantech-driver set-rgb Static 6

# Apply configuration from file
fantech-driver preset --conf config.conf

# Reset to firmware defaults
fantech-driver reset
```

## Configuration

The driver uses INI configuration files. See `ConfigExample.conf` for format reference.

Configuration priority:
1. Custom path (`--conf`)
2. Local `driver.conf`
3. User config `~/.config/fantech-x9-thor/config.conf`

## USB Setup

For automatic permissions, create udev rules:

```bash
echo "SUBSYSTEMS==\"usb\", ATTRS{idVendor}==\"18f8\", ATTRS{idProduct}==\"1086\", GROUP=\"users\", MODE=\"0660\"" | sudo tee /etc/udev/rules.d/50-fantechdriver.rules
sudo udevadm control --reload-rules
sudo udevadm trigger
```

Alternative: run with `sudo` (not recommended).

## License

MIT License
