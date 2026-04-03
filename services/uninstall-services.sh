#!/bin/bash

# Fantech X9 Thor Driver Service Uninstallation Script

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

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

# Remove SystemD services
remove_systemd() {
    local service_type=$1  # "user" or "system"
    
    print_status "Removing SystemD services ($service_type mode)..."
    
    if [[ "$service_type" == "system" ]]; then
        # Stop and disable system service
        if systemctl is-active --quiet fantech-driver.service 2>/dev/null; then
            sudo systemctl stop fantech-driver.service
        fi
        
        if systemctl is-enabled --quiet fantech-driver.service 2>/dev/null; then
            sudo systemctl disable fantech-driver.service
        fi
        
        # Remove service file
        sudo rm -f /etc/systemd/system/fantech-driver.service
        sudo systemctl daemon-reload
        
        print_success "SystemD system service removed"
        
    else  # user mode
        local target_user=${SUDO_USER:-$USER}
        
        # Stop and disable user service
        if sudo -u "$target_user" systemctl --user is-active --quiet "fantech-driver@$target_user.service" 2>/dev/null; then
            sudo -u "$target_user" systemctl --user stop "fantech-driver@$target_user.service"
        fi
        
        if sudo -u "$target_user" systemctl --user is-enabled --quiet "fantech-driver@$target_user.service" 2>/dev/null; then
            sudo -u "$target_user" systemctl --user disable "fantech-driver@$target_user.service"
        fi
        
        # Remove service file
        sudo rm -f /etc/systemd/user/fantech-driver@.service
        sudo -u "$target_user" systemctl --user daemon-reload
        
        print_success "SystemD user service removed for user: $target_user"
    fi
}

# Remove OpenRC services
remove_openrc() {
    local service_type=$1  # "user" or "system"
    
    print_status "Removing OpenRC services ($service_type mode)..."
    
    if [[ "$service_type" == "system" ]]; then
        # Stop and disable service
        if rc-service fantech-driver status 2>/dev/null | grep -q "started"; then
            sudo rc-service fantech-driver stop
        fi
        
        if rc-update show default | grep -q fantech-driver; then
            sudo rc-update del fantech-driver default
        fi
        
        # Remove service file
        sudo rm -f /etc/init.d/fantech-driver
        
        print_success "OpenRC system service removed"
        
    else  # user mode
        # Remove user script
        sudo rm -f /usr/local/bin/fantech-driver-service
        
        print_success "OpenRC user service script removed"
    fi
}

# Remove configuration files
remove_configs() {
    local remove_system=${1:-false}
    local remove_user=${2:-false}
    
    if [[ "$remove_system" == "true" ]]; then
        print_status "Removing system-wide configuration..."
        sudo rm -rf /etc/fantech-x9-thor
    fi
    
    if [[ "$remove_user" == "true" ]]; then
        print_status "Removing user configuration..."
        rm -rf ~/.config/fantech-x9-thor
    fi
}

# Remove udev rules
remove_udev_rules() {
    print_status "Removing udev rules..."
    sudo rm -f /etc/udev/rules.d/50-fantech-driver.rules
    sudo udevadm control --reload-rules 2>/dev/null || true
    sudo udevadm trigger 2>/dev/null || true
}

# Remove log rotation
remove_logrotate() {
    print_status "Removing log rotation..."
    sudo rm -f /etc/logrotate.d/fantech-driver
}

# Main uninstallation
main() {
    local uninstall_type=${1:-"all"}
    local remove_configs=${2:-false}
    local init_system
    
    print_status "Fantech X9 Thor Driver Service Uninstallation"
    print_status "Uninstall type: $uninstall_type"
    
    # Detect init system
    init_system=$(detect_init_system)
    print_status "Detected init system: $init_system"
    
    if [[ "$init_system" == "unknown" ]]; then
        print_warning "Unknown init system, proceeding with generic cleanup"
    fi
    
    # Remove services based on uninstall type
    case "$uninstall_type" in
        "system")
            if [[ "$init_system" == "systemd" ]]; then
                remove_systemd "system"
            elif [[ "$init_system" == "openrc" ]]; then
                remove_openrc "system"
            fi
            ;;
        "user")
            if [[ "$init_system" == "systemd" ]]; then
                remove_systemd "user"
            elif [[ "$init_system" == "openrc" ]]; then
                remove_openrc "user"
            fi
            ;;
        "all"|*)
            # Remove both system and user services
            if [[ "$init_system" == "systemd" ]]; then
                remove_systemd "system"
                remove_systemd "user"
            elif [[ "$init_system" == "openrc" ]]; then
                remove_openrc "system"
                remove_openrc "user"
            fi
            ;;
    esac
    
    # Remove common files
    remove_udev_rules
    remove_logrotate
    
    # Remove configuration files if requested
    if [[ "$remove_configs" == "true" ]]; then
        remove_configs "true" "true"
    else
        print_status "Configuration files preserved"
        print_status "To remove configs, run: $0 all configs"
    fi
    
    print_success "Uninstallation completed!"
    print_status "Note: The fantech-driver binary was not removed"
    print_status "To remove the binary, run: sudo make uninstall"
}

# Show usage
usage() {
    echo "Fantech X9 Thor Driver Service Uninstallation"
    echo ""
    echo "Usage: $0 [uninstall_type] [configs]"
    echo ""
    echo "Uninstall types:"
    echo "  system    - Remove system-wide service only"
    echo "  user      - Remove per-user service only"
    echo "  all       - Remove all services (default)"
    echo ""
    echo "Configuration removal:"
    echo "  configs   - Also remove configuration files"
    echo ""
    echo "Examples:"
    echo "  sudo $0 system          # Remove system service only"
    echo "  $0 user                 # Remove user service only"
    echo "  sudo $0 all configs     # Remove everything including configs"
}

# Check arguments
if [[ $# -gt 2 ]]; then
    usage
    exit 1
fi

if [[ "$1" == "--help" || "$1" == "-h" ]]; then
    usage
    exit 0
fi

# Run main function
main "$@"
