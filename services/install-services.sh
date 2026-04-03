#!/bin/bash

# Fantech X9 Thor Driver Service Installation Script
# Supports both SystemD and OpenRC

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
SERVICE_NAME="fantech-driver"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if running as root
check_root() {
    if [[ $EUID -ne 0 ]]; then
        print_error "This script must be run as root for system-wide installation"
        print_status "For user-specific installation, use: sudo ./install-services.sh user"
        exit 1
    fi
}

# Detect init system
detect_init_system() {
    if [[ -f /usr/lib/systemd/systemd ]] || command -v systemctl >/dev/null 2>&1; then
        echo "systemd"
    elif [[ -f /sbin/openrc ]] || command -v openrc >/dev/null 2>&1; then
        echo "openrc"
    else
        echo "unknown"
    fi
}

# Install SystemD service
install_systemd() {
    local service_type=$1  # "user" or "system"
    
    print_status "Installing SystemD service ($service_type mode)..."
    
    # Create config directory if needed
    if [[ "$service_type" == "system" ]]; then
        mkdir -p /etc/fantech-x9-thor
        
        # Copy default config if it doesn't exist
        if [[ ! -f /etc/fantech-x9-thor/config.conf ]]; then
            if [[ -f "$PROJECT_ROOT/my_config.conf" ]]; then
                cp "$PROJECT_ROOT/my_config.conf" /etc/fantech-x9-thor/config.conf
                print_status "Copied default config to /etc/fantech-x9-thor/config.conf"
            else
                print_warning "No default config found, please create /etc/fantech-x9-thor/config.conf"
            fi
        fi
        
        # Install system service
        cp "$SCRIPT_DIR/systemd/fantech-driver.service" /etc/systemd/system/
        systemctl daemon-reload
        systemctl enable fantech-driver.service
        
        print_success "SystemD service installed and enabled"
        print_status "Start with: sudo systemctl start fantech-driver"
        print_status "Status with: sudo systemctl status fantech-driver"
        
    else  # user mode
        local target_user=${SUDO_USER:-$USER}
        
        # Install user service
        cp "$SCRIPT_DIR/systemd/fantech-driver@.service" /etc/systemd/user/
        
        # Enable user service
        sudo -u "$target_user" systemctl --user daemon-reload
        sudo -u "$target_user" systemctl --user enable fantech-driver@"$target_user".service
        
        print_success "User SystemD service installed and enabled for user: $target_user"
        print_status "Start with: systemctl --user start fantech-driver@$target_user"
        print_status "Status with: systemctl --user status fantech-driver@$target_user"
    fi
}

# Install OpenRC service
install_openrc() {
    local service_type=$1  # "user" or "system"
    
    print_status "Installing OpenRC service ($service_type mode)..."
    
    if [[ "$service_type" == "system" ]]; then
        # Create config directory
        mkdir -p /etc/fantech-x9-thor
        
        # Copy default config if it doesn't exist
        if [[ ! -f /etc/fantech-x9-thor/config.conf ]]; then
            if [[ -f "$PROJECT_ROOT/my_config.conf" ]]; then
                cp "$PROJECT_ROOT/my_config.conf" /etc/fantech-x9-thor/config.conf
                print_status "Copied default config to /etc/fantech-x9-thor/config.conf"
            else
                print_warning "No default config found, please create /etc/fantech-x9-thor/config.conf"
            fi
        fi
        
        # Install system service
        cp "$SCRIPT_DIR/openrc/fantech-driver.sh" /etc/init.d/fantech-driver
        chmod +x /etc/init.d/fantech-driver
        
        # Enable service
        rc-update add fantech-driver default
        
        print_success "OpenRC service installed and enabled"
        print_status "Start with: sudo rc-service fantech-driver start"
        print_status "Status with: sudo rc-service fantech-driver status"
        
    else  # user mode
        print_warning "User-mode OpenRC services are not fully supported"
        print_status "Consider using system-wide installation or manual startup"
        
        # Install user script for manual use
        mkdir -p /usr/local/bin
        cp "$SCRIPT_DIR/openrc/fantech-driver.sh" /usr/local/bin/fantech-driver-service
        chmod +x /usr/local/bin/fantech-driver-service
        
        print_status "User service script installed: /usr/local/bin/fantech-driver-service"
        print_status "Run manually: fantech-driver-service start"
    fi
}

# Install udev rules
install_udev_rules() {
    print_status "Installing udev rules..."
    
    cat > /etc/udev/rules.d/50-fantech-driver.rules << 'EOF'
# Fantech X9 Thor Mouse
SUBSYSTEMS=="usb", ATTRS{idVendor}=="18f8", ATTRS{idProduct}=="1086", GROUP="users", MODE="0660", TAG+="uaccess"
EOF
    
    udevadm control --reload-rules
    udevadm trigger
    
    print_success "Udev rules installed"
}

# Create log rotation
install_logrotate() {
    print_status "Installing log rotation..."
    
    cat > /etc/logrotate.d/fantech-driver << 'EOF'
/var/log/fantech-driver-*.log {
    daily
    rotate 7
    compress
    delaycompress
    missingok
    notifempty
    create 644 root root
}
EOF
    
    print_success "Log rotation configured"
}

# Main installation
main() {
    local install_type=${1:-"system"}
    local init_system
    
    print_status "Fantech X9 Thor Driver Service Installation"
    print_status "Install type: $install_type"
    
    # Detect init system
    init_system=$(detect_init_system)
    print_status "Detected init system: $init_system"
    
    if [[ "$init_system" == "unknown" ]]; then
        print_error "Unsupported init system"
        exit 1
    fi
    
    # Check root for system installation
    if [[ "$install_type" == "system" ]]; then
        check_root
    fi
    
    # Install the driver binary first
    if [[ ! -f /usr/local/bin/fantech-driver ]]; then
        print_status "Installing fantech-driver binary..."
        make -C "$PROJECT_ROOT" install
    fi
    
    # Install udev rules
    install_udev_rules
    
    # Install service based on init system
    case "$init_system" in
        "systemd")
            install_systemd "$install_type"
            ;;
        "openrc")
            install_openrc "$install_type"
            ;;
    esac
    
    # Install log rotation
    install_logrotate
    
    print_success "Installation completed!"
    
    if [[ "$install_type" == "system" ]]; then
        print_status "The service will start automatically on boot"
        print_status "To start it now, use the appropriate command for your init system"
    else
        print_status "User services will start when you log in"
    fi
}

# Show usage
usage() {
    echo "Fantech X9 Thor Driver Service Installation"
    echo ""
    echo "Usage: $0 [install_type]"
    echo ""
    echo "Install types:"
    echo "  system    - Install system-wide service (requires root)"
    echo "  user      - Install per-user service"
    echo ""
    echo "Examples:"
    echo "  sudo $0 system    # System-wide installation"
    echo "  $0 user            # Per-user installation"
}

# Check arguments
if [[ $# -gt 1 ]]; then
    usage
    exit 1
fi

if [[ "$1" == "--help" || "$1" == "-h" ]]; then
    usage
    exit 0
fi

# Run main function
main "$@"
