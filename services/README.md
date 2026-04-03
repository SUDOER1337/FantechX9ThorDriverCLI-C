# Fantech X9 Thor Driver - Service Integration

This directory contains service integration files for running the Fantech X9 Thor driver as a system service.

## Supported Init Systems

- **SystemD** - Modern Linux distributions (Ubuntu 16.04+, CentOS 7+, Debian 8+, Arch Linux)
- **OpenRC** - Gentoo, Alpine Linux, and some other distributions

## Installation

### Quick Install

```bash
# System-wide installation (requires root)
sudo ./install-services.sh system

# Per-user installation
./install-services.sh user
```

### Manual Installation

#### SystemD

1. **System-wide service:**
   ```bash
   sudo cp services/systemd/fantech-driver.service /etc/systemd/system/
   sudo systemctl daemon-reload
   sudo systemctl enable fantech-driver.service
   sudo systemctl start fantech-driver.service
   ```

2. **Per-user service:**
   ```bash
   cp services/systemd/fantech-driver@.service /etc/systemd/user/
   systemctl --user daemon-reload
   systemctl --user enable fantech-driver@$USER.service
   systemctl --user start fantech-driver@$USER.service
   ```

#### OpenRC

1. **System-wide service:**
   ```bash
   sudo cp services/openrc/fantech-driver.sh /etc/init.d/fantech-driver
   sudo chmod +x /etc/init.d/fantech-driver
   sudo rc-update add fantech-driver default
   sudo rc-service fantech-driver start
   ```

2. **Per-user service:**
   ```bash
   sudo cp services/openrc/fantech-driver.sh /usr/local/bin/fantech-driver-service
   sudo chmod +x /usr/local/bin/fantech-driver-service
   # Run manually: fantech-driver-service start
   ```

## Configuration

### System-wide Configuration

Configuration file: `/etc/fantech-x9-thor/config.conf`

```bash
# Create system-wide config directory
sudo mkdir -p /etc/fantech-x9-thor

# Copy your config
sudo cp my_config.conf /etc/fantech-x9-thor/config.conf
```

### Per-user Configuration

Configuration file: `~/.config/fantech-x9-thor/config.conf`

```bash
# Create user config directory
mkdir -p ~/.config/fantech-x9-thor

# Copy your config
cp my_config.conf ~/.config/fantech-x9-thor/config.conf
```

## Service Management

### SystemD Commands

```bash
# System-wide service
sudo systemctl start fantech-driver
sudo systemctl stop fantech-driver
sudo systemctl restart fantech-driver
sudo systemctl status fantech-driver
sudo systemctl enable fantech-driver    # Start on boot
sudo systemctl disable fantech-driver   # Don't start on boot

# Per-user service
systemctl --user start fantech-driver@$USER
systemctl --user stop fantech-driver@$USER
systemctl --user restart fantech-driver@$USER
systemctl --user status fantech-driver@$USER
systemctl --user enable fantech-driver@$USER
systemctl --user disable fantech-driver@$USER
```

### OpenRC Commands

```bash
# System-wide service
sudo rc-service fantech-driver start
sudo rc-service fantech-driver stop
sudo rc-service fantech-driver restart
sudo rc-service fantech-driver status
sudo rc-update add fantech-driver default    # Start on boot
sudo rc-update del fantech-driver default   # Don't start on boot
```

## Logging

### SystemD Logs

```bash
# System-wide service logs
sudo journalctl -u fantech-driver -f

# Per-user service logs
journalctl --user -u fantech-driver@$USER -f
```

### OpenRC Logs

```bash
# Log files are located at:
/var/log/fantech-driver-$USER.log
/var/log/fantech-driver-$USER.err

# View logs
sudo tail -f /var/log/fantech-driver-$USER.log
```

## Features

### Auto-apply Configuration

The service automatically applies the specified configuration when the device is connected:

- **System-wide**: `/etc/fantech-x9-thor/config.conf`
- **Per-user**: `~/.config/fantech-x9-thor/config.conf`

### Device Monitoring

The service monitors USB events and automatically:
- Detects when the Fantech X9 Thor mouse is connected
- Applies the saved configuration
- Handles device disconnections gracefully

### Security

- **SystemD**: Uses `NoNewPrivileges=true`, `ProtectSystem=strict`, and limited device access
- **OpenRC**: Runs as specified user with limited permissions

## Troubleshooting

### Device Not Found

1. Check udev rules:
   ```bash
   sudo udevadm control --reload-rules
   sudo udevadm trigger
   ```

2. Verify device permissions:
   ```bash
   lsusb | grep 18f8:1086
   ```

3. Check service status:
   ```bash
   # SystemD
   sudo systemctl status fantech-driver
   
   # OpenRC
   sudo rc-service fantech-driver status
   ```

### Configuration Not Applied

1. Check configuration file permissions:
   ```bash
   # System-wide
   sudo ls -la /etc/fantech-x9-thor/config.conf
   
   # Per-user
   ls -la ~/.config/fantech-x9-thor/config.conf
   ```

2. Test configuration manually:
   ```bash
   fantech-driver preset --conf /path/to/config.conf
   ```

### Service Won't Start

1. Check logs for errors:
   ```bash
   # SystemD
   sudo journalctl -u fantech-driver -n 50
   
   # OpenRC
   sudo cat /var/log/fantech-driver-$USER.err
   ```

2. Verify binary installation:
   ```bash
   which fantech-driver
   ```

## Uninstallation

### SystemD

```bash
# System-wide
sudo systemctl stop fantech-driver
sudo systemctl disable fantech-driver
sudo rm /etc/systemd/system/fantech-driver.service
sudo systemctl daemon-reload

# Per-user
systemctl --user stop fantech-driver@$USER
systemctl --user disable fantech-driver@$USER
sudo rm /etc/systemd/user/fantech-driver@.service
systemctl --user daemon-reload
```

### OpenRC

```bash
sudo rc-service fantech-driver stop
sudo rc-update del fantech-driver default
sudo rm /etc/init.d/fantech-driver
```

### Remove Files

```bash
# System-wide
sudo rm -rf /etc/fantech-x9-thor
sudo rm /etc/udev/rules.d/50-fantech-driver.rules
sudo rm /etc/logrotate.d/fantech-driver

# Per-user (optional)
rm -rf ~/.config/fantech-x9-thor
```

## Advanced Configuration

### Custom Presets

You can use any preset file with the service by modifying the service file:

```bash
# Edit the service file
sudo systemctl edit fantech-driver

# Change the ExecStart line to use a different preset:
ExecStart=/usr/local/bin/fantech-driver daemon --auto /etc/fantech-x9-thor/presets/gaming.conf
```

### Multiple Devices

For multiple devices, create separate service files with different configurations:

```bash
# Copy and modify the service
sudo cp /etc/systemd/system/fantech-driver.service /etc/systemd/system/fantech-driver-gaming.service
sudo systemctl edit fantech-driver-gaming.service
```

## Support

For issues with the service integration:

1. Check the troubleshooting section above
2. Verify your init system is supported
3. Ensure the driver binary is properly installed
4. Check that your configuration file is valid

The service integration is designed to be robust and handle most common scenarios automatically.
