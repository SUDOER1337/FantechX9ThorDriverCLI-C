#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "protocol.h"
#include "usb_driver.h"
#include "cli.h"
#include "config.h"
#include "error.h"

int main(int argc, char *argv[]) {
    cli_args_t args;
    int result = cli_parse_args(argc, argv, &args);
    
    if (result == -2) {
        // Help or version was printed, exit successfully
        return 0;
    } else if (result != 0) {
        // Argument parsing failed
        ERROR_PRINT();
        return 1;
    }
    
    // Initialize USB driver
    usb_driver_t driver;
    if (usb_driver_init(&driver) != 0) {
        ERROR_PRINT();
        return 1;
    }
    
    // Find device
    if (usb_driver_find_device(&driver) != 0) {
        ERROR_PRINT();
        usb_driver_cleanup(&driver);
        return 1;
    }
    
    // Check device state
    device_state_t state = usb_driver_check_state(&driver);
    if (state != DEVICE_STATE_READY) {
        if (state == DEVICE_STATE_PERMISSION_ERROR) {
            ERROR_PRINT();
        }
        usb_driver_cleanup(&driver);
        return 1;
    }
    
    // Conquer device from kernel driver
    if (usb_driver_conquer(&driver) != 0) {
        ERROR_PRINT();
        usb_driver_cleanup(&driver);
        return 1;
    }
    
    // Execute command
    int command_result = -1;
    
    switch (args.command) {
        case CMD_FIND:
            command_result = cmd_handle_find(&driver, &args);
            break;
            
        case CMD_SET_DPI:
            command_result = cmd_handle_set_dpi(&driver, &args);
            break;
            
        case CMD_SET_RGB:
            command_result = cmd_handle_set_rgb(&driver, &args);
            break;
            
        case CMD_SET_COLOR:
            command_result = cmd_handle_set_color(&driver, &args);
            break;
            
        case CMD_PRESET:
            command_result = cmd_handle_preset(&driver, &args);
            break;
            
        case CMD_RESET:
            command_result = cmd_handle_reset(&driver, &args);
            break;
            
        case CMD_CONFIG:
            command_result = cmd_handle_config(&driver, &args);
            break;
            
        case CMD_MONITOR:
            command_result = cmd_handle_monitor(&driver, &args);
            break;
            
        case CMD_DAEMON:
            command_result = cmd_handle_daemon(&driver, &args);
            break;
            
        default:
            fprintf(stderr, "Unknown command\n");
            command_result = -1;
            break;
    }
    
    // Release device back to kernel
    usb_driver_liberate(&driver);
    usb_driver_cleanup(&driver);
    
    return (command_result == 0) ? 0 : 1;
}
