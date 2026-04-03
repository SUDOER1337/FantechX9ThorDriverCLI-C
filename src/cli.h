#ifndef CLI_H
#define CLI_H

#include <stdint.h>
#include "protocol.h"
#include "usb_driver.h"

// Command types
typedef enum {
    CMD_NONE,
    CMD_FIND,
    CMD_SET_DPI,
    CMD_SET_RGB,
    CMD_SET_COLOR,
    CMD_PRESET,
    CMD_CONFIG,
    CMD_RESET,
    CMD_MONITOR,
    CMD_DAEMON,
} command_type_t;

// CLI arguments structure
typedef struct {
    command_type_t command;
    char config_path[512];
    
    // SET_DPI arguments
    int dpi_value;
    int dpi_profile;
    
    // SET_RGB arguments
    color_scheme_t rgb_mode;
    int rgb_speed;
    
    // SET_COLOR arguments
    int color_profile;
    uint8_t color_red;
    uint8_t color_green;
    uint8_t color_blue;
    
    // CONFIG subcommands
    char config_subcommand[64];
    char config_source[512];
    
    // MONITOR arguments
    char monitor_subcommand[64];
    int monitor_timeout;
} cli_args_t;

// Function declarations
int cli_parse_args(int argc, char *argv[], cli_args_t *args);
void cli_print_help(void);
void cli_print_version(void);

// Command handlers
int cmd_handle_find(usb_driver_t *driver, const cli_args_t *args);
int cmd_handle_set_dpi(usb_driver_t *driver, const cli_args_t *args);
int cmd_handle_set_rgb(usb_driver_t *driver, const cli_args_t *args);
int cmd_handle_set_color(usb_driver_t *driver, const cli_args_t *args);
int cmd_handle_preset(usb_driver_t *driver, const cli_args_t *args);
int cmd_handle_config(usb_driver_t *driver, const cli_args_t *args);
int cmd_handle_reset(usb_driver_t *driver, const cli_args_t *args);
int cmd_handle_monitor(usb_driver_t *driver, const cli_args_t *args);
int cmd_handle_daemon(usb_driver_t *driver, const cli_args_t *args);

// Utility functions
int cli_validate_dpi(int dpi);
int cli_validate_profile(int profile);
int cli_validate_rgb_values(uint8_t red, uint8_t green, uint8_t blue);

#endif // CLI_H
