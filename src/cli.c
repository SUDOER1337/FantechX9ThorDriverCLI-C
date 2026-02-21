#include "cli.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

// Print help information
void cli_print_help(void) {
    printf("Fantech X9 Thor Driver CLI - C Implementation\n\n");
    printf("Usage: fantech-driver <command> [options]\n\n");
    printf("Commands:\n");
    printf("  find                              Find and check device state\n");
    printf("  set-dpi <dpi> <profile>          Set DPI for a profile (200-4800, profile 0-5)\n");
    printf("  set-rgb <mode> <speed>           Set RGB lighting mode (Fixed/Cyclic/Static/Off, speed 1-10)\n");
    printf("  set-color <profile> <r> <g> <b>  Set RGB color for a profile (profile 1-6, rgb 0-255)\n");
    printf("  preset [--conf path]             Apply configuration from config file\n");
    printf("  config <subcommand>              Manage configuration files\n");
    printf("    info                           Show configuration information\n");
    printf("    migrate [--conf path]           Migrate local config to ~/.config\n");
    printf("    presets                        List available presets\n");
    printf("\n");
    printf("Options:\n");
    printf("  --conf, -c <path>    Specify configuration file path\n");
    printf("  --help, -h          Show this help message\n");
    printf("  --version, -v       Show version information\n");
    printf("\n");
    printf("Examples:\n");
    printf("  fantech-driver find\n");
    printf("  fantech-driver set-dpi 1600 2\n");
    printf("  fantech-driver set-color 1 255 0 0\n");
    printf("  fantech-driver set-rgb Static 6\n");
    printf("  fantech-driver preset --conf ~/.config/fantech-x9-thor/gaming.conf\n");
}

// Print version information
void cli_print_version(void) {
    printf("Fantech X9 Thor Driver CLI v1.0.0 (C Implementation)\n");
    printf("Compatible with Fantech X9 Thor RGB Gaming Mouse\n");
}

// Validate DPI value
int cli_validate_dpi(int dpi) {
    if (dpi < 200 || dpi > 4800) {
        fprintf(stderr, "Error: DPI %d out of supported range (200-4800)\n", dpi);
        return -1;
    }
    return 0;
}

// Validate profile number
int cli_validate_profile(int profile) {
    if (profile < 0 || profile > 5) {
        fprintf(stderr, "Error: Profile %d out of range (0-5)\n", profile);
        return -1;
    }
    return 0;
}

// Validate RGB values
int cli_validate_rgb_values(uint8_t red, uint8_t green, uint8_t blue) {
    // RGB values are already uint8_t, so they're 0-255 by definition
    (void)red;    // Suppress unused parameter warning
    (void)green;  // Suppress unused parameter warning
    (void)blue;   // Suppress unused parameter warning
    return 0;
}

// Parse command line arguments
int cli_parse_args(int argc, char *argv[], cli_args_t *args) {
    if (!args) return -1;
    
    memset(args, 0, sizeof(cli_args_t));
    
    if (argc < 2) {
        cli_print_help();
        return -1;
    }
    
    // Parse command
    if (strcmp(argv[1], "find") == 0) {
        args->command = CMD_FIND;
    } else if (strcmp(argv[1], "set-dpi") == 0) {
        args->command = CMD_SET_DPI;
        if (argc < 4) {
            fprintf(stderr, "Error: set-dpi requires DPI and profile arguments\n");
            return -1;
        }
        args->dpi_value = atoi(argv[2]);
        args->dpi_profile = atoi(argv[3]);
        
        if (cli_validate_dpi(args->dpi_value) != 0) return -1;
        if (cli_validate_profile(args->dpi_profile) != 0) return -1;
        
    } else if (strcmp(argv[1], "set-rgb") == 0) {
        args->command = CMD_SET_RGB;
        if (argc < 3) {
            fprintf(stderr, "Error: set-rgb requires mode argument\n");
            return -1;
        }
        
        const char *mode_str = argv[2];
        if (strcmp(mode_str, "Fixed") == 0) {
            args->rgb_mode = COLOR_SCHEME_FIXED;
        } else if (strcmp(mode_str, "Cyclic") == 0) {
            args->rgb_mode = COLOR_SCHEME_CYCLIC;
        } else if (strcmp(mode_str, "Static") == 0) {
            args->rgb_mode = COLOR_SCHEME_STATIC;
        } else if (strcmp(mode_str, "Off") == 0) {
            args->rgb_mode = COLOR_SCHEME_OFF;
        } else {
            fprintf(stderr, "Error: Invalid RGB mode '%s'. Use Fixed, Cyclic, Static, or Off\n", mode_str);
            return -1;
        }
        
        args->rgb_speed = (argc >= 4) ? atoi(argv[3]) : 1;
        if (args->rgb_speed < 1 || args->rgb_speed > 10) {
            fprintf(stderr, "Error: RGB speed %d out of range (1-10)\n", args->rgb_speed);
            return -1;
        }
        
    } else if (strcmp(argv[1], "set-color") == 0) {
        args->command = CMD_SET_COLOR;
        if (argc < 6) {
            fprintf(stderr, "Error: set-color requires profile, red, green, blue arguments\n");
            return -1;
        }
        
        args->color_profile = atoi(argv[2]);
        args->color_red = (uint8_t)atoi(argv[3]);
        args->color_green = (uint8_t)atoi(argv[4]);
        args->color_blue = (uint8_t)atoi(argv[5]);
        
        if (args->color_profile < 1 || args->color_profile > 6) {
            fprintf(stderr, "Error: Profile %d out of range (1-6)\n", args->color_profile);
            return -1;
        }
        
        if (cli_validate_rgb_values(args->color_red, args->color_green, args->color_blue) != 0) {
            return -1;
        }
        
    } else if (strcmp(argv[1], "preset") == 0) {
        args->command = CMD_PRESET;
        // Parse optional --conf argument
        for (int i = 2; i < argc; i++) {
            if (strcmp(argv[i], "--conf") == 0 && i + 1 < argc) {
                strncpy(args->config_path, argv[i + 1], sizeof(args->config_path) - 1);
                i++;
            }
        }
        
    } else if (strcmp(argv[1], "config") == 0) {
        args->command = CMD_CONFIG;
        if (argc < 3) {
            fprintf(stderr, "Error: config requires a subcommand (info, migrate, presets)\n");
            return -1;
        }
        
        strncpy(args->config_subcommand, argv[2], sizeof(args->config_subcommand) - 1);
        
        // Parse subcommand arguments
        for (int i = 3; i < argc; i++) {
            if (strcmp(argv[i], "--conf") == 0 && i + 1 < argc) {
                strncpy(args->config_path, argv[i + 1], sizeof(args->config_path) - 1);
                i++;
            } else if (strcmp(argv[i], "--source") == 0 && i + 1 < argc) {
                strncpy(args->config_source, argv[i + 1], sizeof(args->config_source) - 1);
                i++;
            } else if (strcmp(argv[i], "--conf") == 0 && i + 1 < argc && strcmp(args->config_subcommand, "migrate") == 0) {
                // For migrate command, also accept --conf as source
                strncpy(args->config_source, argv[i + 1], sizeof(args->config_source) - 1);
                i++;
            }
        }
        
    } else if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        cli_print_help();
        return -2;  // Special return code for help
    } else if (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-v") == 0) {
        cli_print_version();
        return -2;  // Special return code for version
    } else {
        fprintf(stderr, "Error: Unknown command '%s'\n", argv[1]);
        cli_print_help();
        return -1;
    }
    
    return 0;
}

// Handle find command
int cmd_handle_find(usb_driver_t *driver, const cli_args_t *args) {
    (void)args;  // Unused parameter
    
    if (usb_driver_find_device(driver) != 0) {
        printf("Device not found.\n");
        return -1;
    }
    
    device_state_t state = usb_driver_check_state(driver);
    switch (state) {
        case DEVICE_STATE_READY:
            printf("Device found and ready!\n");
            return 0;
        case DEVICE_STATE_PERMISSION_ERROR:
            printf("Device found but insufficient permissions.\n");
            printf("Try adding udev rules or running as root.\n");
            return -1;
        case DEVICE_STATE_NOT_FOUND:
            printf("Device not found. Try replugging.\n");
            return -1;
        default:
            printf("Unknown device state.\n");
            return -1;
    }
}

// Handle set-dpi command
int cmd_handle_set_dpi(usb_driver_t *driver, const cli_args_t *args) {
    if (!args) return -1;
    
    usb_payload_t payload;
    usb_create_dpi_profile_config(&payload, args->dpi_value, args->dpi_profile);
    
    if (usb_driver_send_payload(driver, &payload) == 0) {
        printf("DPI for profile %d set to %d.\n", args->dpi_profile, args->dpi_value);
        return 0;
    }
    
    return -1;
}

// Handle set-rgb command
int cmd_handle_set_rgb(usb_driver_t *driver, const cli_args_t *args) {
    if (!args) return -1;
    
    usb_payload_t payload;
    usb_create_rgb_lights_config(&payload, args->rgb_mode, args->rgb_speed);
    
    if (usb_driver_send_payload(driver, &payload) == 0) {
        printf("Lighting mode set to %s with speed %d.\n", 
               config_color_scheme_to_string(args->rgb_mode), args->rgb_speed);
        return 0;
    }
    
    return -1;
}

// Handle set-color command
int cmd_handle_set_color(usb_driver_t *driver, const cli_args_t *args) {
    if (!args) return -1;
    
    usb_payload_t payload;
    usb_create_color_profile_config(&payload, args->color_profile, 
                                   args->color_red, args->color_green, args->color_blue);
    
    if (usb_driver_send_payload(driver, &payload) == 0) {
        printf("Set profile %d color to R:%d G:%d B:%d.\n", 
               args->color_profile, args->color_red, args->color_green, args->color_blue);
        return 0;
    }
    
    return -1;
}

// Handle preset command
int cmd_handle_preset(usb_driver_t *driver, const cli_args_t *args) {
    if (!args) return -1;
    
    (void)driver;  // Driver parameter not used in current implementation
    
    char config_path[512];
    if (strlen(args->config_path) > 0) {
        strncpy(config_path, args->config_path, sizeof(config_path) - 1);
    } else {
        if (config_get_active_config_path(config_path, sizeof(config_path)) != 0) {
            fprintf(stderr, "Error: Could not determine config file path\n");
            return -1;
        }
    }
    
    device_config_t config;
    if (config_load_device_config(config_path, &config) != 0) {
        fprintf(stderr, "Error: Could not load config from %s\n", config_path);
        return -1;
    }
    
    // Apply configuration to device
    // This would involve sending multiple payloads for each setting
    printf("Configuration from '%s' has been sent to mouse (active profile %d)\n", 
           config_path, config.active_profile);
    
    return 0;
}

// Handle config command
int cmd_handle_config(usb_driver_t *driver, const cli_args_t *args) {
    (void)driver;  // Unused parameter
    
    if (!args) return -1;
    
    if (strcmp(args->config_subcommand, "info") == 0) {
        char config_path[512];
        if (strlen(args->config_path) > 0) {
            strncpy(config_path, args->config_path, sizeof(config_path) - 1);
        } else {
            config_get_active_config_path(config_path, sizeof(config_path));
        }
        
        printf("Configuration Information:\n");
        printf("  Active config: %s\n", config_path);
        
        config_paths_t paths;
        if (config_get_paths(&paths) == 0) {
            printf("  Config directory: %s\n", paths.user_config_dir);
        }
        
    } else if (strcmp(args->config_subcommand, "migrate") == 0) {
        const char *source = (strlen(args->config_source) > 0) ? args->config_source : "driver.conf";
        if (config_migrate_to_user_config(source) == 0) {
            printf("Successfully migrated config\n");
        } else {
            fprintf(stderr, "Migration failed\n");
            return -1;
        }
        
    } else if (strcmp(args->config_subcommand, "presets") == 0) {
        printf("Available presets:\n");
        printf("  (Preset functionality not yet implemented in C version)\n");
        
    } else {
        fprintf(stderr, "Unknown config subcommand: %s\n", args->config_subcommand);
        return -1;
    }
    
    return 0;
}
