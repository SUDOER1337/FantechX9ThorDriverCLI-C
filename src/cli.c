#include "cli.h"
#include "usb_driver.h"
#include "config.h"
#include "error.h"
#include "device_monitor.h"
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>

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
    printf("  reset                             Reset mouse to firmware defaults\n");
    printf("  config <subcommand>              Manage configuration files\n");
    printf("    info                           Show configuration information\n");
    printf("    migrate [--conf path]           Migrate local config to ~/.config\n");
    printf("    presets                        List available presets\n");
    printf("  monitor <subcommand>             Monitor device events\n");
    printf("    start [--auto path]            Start monitoring (optional auto-apply config)\n");
    printf("    stop                           Stop monitoring\n");
    printf("    status                         Show monitoring status\n");
    printf("    events [timeout]               Show recent events (timeout in seconds)\n");
    printf("  daemon [--auto path]             Run as daemon with auto-apply\n");
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
    printf("  fantech-driver reset\n");
}

// Print version information
void cli_print_version(void) {
    printf("Fantech X9 Thor Driver CLI v1.0.0 (C Implementation)\n");
    printf("Compatible with Fantech X9 Thor RGB Gaming Mouse\n");
}

// Validate DPI value
int cli_validate_dpi(int dpi) {
    if (dpi < 200 || dpi > 4800) {
        RETURN_ERROR_DETAILS(ERROR_OUT_OF_RANGE, "DPI value out of range", 
                           "Supported DPI range is 200-4800");
    }
    return 0;
}

// Validate profile number
int cli_validate_profile(int profile) {
    if (profile < 0 || profile > 5) {
        RETURN_ERROR_DETAILS(ERROR_OUT_OF_RANGE, "Profile number out of range", 
                           "Supported profile range is 0-5");
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
        
    } else if (strcmp(argv[1], "reset") == 0) {
        args->command = CMD_RESET;
        
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
        
    } else if (strcmp(argv[1], "monitor") == 0) {
        args->command = CMD_MONITOR;
        if (argc < 3) {
            RETURN_ERROR_DETAILS(ERROR_INVALID_ARGS, "monitor requires a subcommand", 
                               "Use: start, stop, status, or events");
        }
        
        strncpy(args->monitor_subcommand, argv[2], sizeof(args->monitor_subcommand) - 1);
        
        // Parse monitor subcommand arguments
        for (int i = 3; i < argc; i++) {
            if (strcmp(argv[i], "--auto") == 0 && i + 1 < argc) {
                strncpy(args->config_path, argv[i + 1], sizeof(args->config_path) - 1);
                i++;
            } else if (strcmp(args->monitor_subcommand, "events") == 0 && i == 3) {
                args->monitor_timeout = atoi(argv[i]);
                if (args->monitor_timeout <= 0) args->monitor_timeout = 10; // Default 10 seconds
            }
        }
        
    } else if (strcmp(argv[1], "daemon") == 0) {
        args->command = CMD_DAEMON;
        args->monitor_timeout = 0; // Run indefinitely
        
        // Parse daemon arguments
        for (int i = 2; i < argc; i++) {
            if (strcmp(argv[i], "--auto") == 0 && i + 1 < argc) {
                strncpy(args->config_path, argv[i + 1], sizeof(args->config_path) - 1);
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
    
    // Apply DPI settings for all profiles
    for (int i = 0; i < MAX_PROFILES; i++) {
        if (config.profiles[i].enabled) {
            usb_payload_t dpi_payload;
            usb_create_dpi_profile_config(&dpi_payload, config.profiles[i].dpi, i);
            if (usb_driver_send_payload(driver, &dpi_payload) == 0) {
                printf("DPI for profile %d set to %d.\n", i, config.profiles[i].dpi);
            } else {
                fprintf(stderr, "Failed to set DPI for profile %d\n", i);
            }
        }
    }
    
    // Apply color settings for all profiles
    for (int i = 0; i < MAX_PROFILES; i++) {
        if (config.profiles[i].enabled) {
            usb_payload_t color_payload;
            usb_create_color_profile_config(&color_payload, i + 1, 
                                          config.profiles[i].red,
                                          config.profiles[i].green, 
                                          config.profiles[i].blue);
            if (usb_driver_send_payload(driver, &color_payload) == 0) {
                printf("Set profile %d color to R:%d G:%d B:%d.\n", 
                       i + 1, config.profiles[i].red, config.profiles[i].green, config.profiles[i].blue);
            } else {
                fprintf(stderr, "Failed to set color for profile %d\n", i + 1);
            }
        }
    }
    
    // Apply RGB lighting mode
    usb_payload_t rgb_payload;
    usb_create_rgb_lights_config(&rgb_payload, config.color_scheme, config.scheme_duration);
    if (usb_driver_send_payload(driver, &rgb_payload) == 0) {
        printf("Lighting mode set to %s with speed %d.\n", 
               config_color_scheme_to_string(config.color_scheme), config.scheme_duration);
    } else {
        fprintf(stderr, "Failed to set lighting mode\n");
    }
    
    printf("Configuration from '%s' has been applied to mouse (active profile %d)\n", 
           config_path, config.active_profile);
    
    return 0;
}

// Handle reset command
int cmd_handle_reset(usb_driver_t *driver, const cli_args_t *args) {
    (void)args;  // Unused parameter
    
    printf("Resetting mouse to firmware defaults...\n");
    
    // Firmware default DPI settings: [200, 600, 1200, 1600, 2400, 4000]
    int default_dpis[] = {200, 600, 1200, 1600, 2400, 4000};
    
    // Apply default DPI settings for all profiles
    for (int i = 0; i < 6; i++) {
        usb_payload_t dpi_payload;
        usb_create_dpi_profile_config(&dpi_payload, default_dpis[i], i);
        if (usb_driver_send_payload(driver, &dpi_payload) == 0) {
            printf("DPI for profile %d reset to %d.\n", i, default_dpis[i]);
        } else {
            fprintf(stderr, "Failed to reset DPI for profile %d\n", i);
        }
    }
    
    // Apply default color settings (orange: 255, 73, 0) for all profiles
    for (int i = 1; i <= 6; i++) {
        usb_payload_t color_payload;
        usb_create_color_profile_config(&color_payload, i, 255, 73, 0);
        if (usb_driver_send_payload(driver, &color_payload) == 0) {
            printf("Profile %d color reset to R:255 G:73 B:0.\n", i);
        } else {
            fprintf(stderr, "Failed to reset color for profile %d\n", i);
        }
    }
    
    // Apply default RGB lighting mode (Static with speed 6)
    usb_payload_t rgb_payload;
    usb_create_rgb_lights_config(&rgb_payload, COLOR_SCHEME_STATIC, 6);
    if (usb_driver_send_payload(driver, &rgb_payload) == 0) {
        printf("Lighting mode reset to Static with speed 6.\n");
    } else {
        fprintf(stderr, "Failed to reset lighting mode\n");
    }
    
    printf("Mouse has been reset to firmware defaults.\n");
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
        preset_info_t presets[MAX_PRESETS];
        int preset_count = 0;
        
        if (config_list_presets(presets, &preset_count) == 0) {
            if (preset_count == 0) {
                printf("No presets found. Create presets using the preset system.\n");
                printf("Presets are stored in: ~/.config/fantech-x9-thor/presets/\n");
            } else {
                printf("Available presets (%d found):\n", preset_count);
                for (int i = 0; i < preset_count; i++) {
                    printf("  %-20s - %s\n", presets[i].name, presets[i].description);
                    printf("  Path: %s\n", presets[i].path);
                }
                printf("\nUsage examples:\n");
                printf("  fantech-driver preset --conf ~/.config/fantech-x9-thor/presets/gaming.conf\n");
                printf("  fantech-driver preset --conf %s\n", presets[0].path);
            }
        } else {
            fprintf(stderr, "Error: Could not list presets\n");
            return -1;
        }
        
    } else {
        fprintf(stderr, "Unknown config subcommand: %s\n", args->config_subcommand);
        return -1;
    }
    
    return 0;
}

// Global monitor instance for CLI commands
static device_monitor_t *g_monitor = NULL;

// Device event callback for CLI
static void cli_event_callback(const device_event_t *event, void *user_data) {
    (void)user_data;
    
    const char *event_type_str = "Unknown";
    switch (event->type) {
        case DEVICE_EVENT_CONNECT: event_type_str = "Connected"; break;
        case DEVICE_EVENT_DISCONNECT: event_type_str = "Disconnected"; break;
        case DEVICE_EVENT_ERROR: event_type_str = "Error"; break;
    }
    
    printf("[%s] %s - VID:0x%04X PID:0x%04X (%s)\n", 
           event->timestamp, event_type_str, event->vendor_id, event->product_id, event->details);
}

// Handle monitor command
int cmd_handle_monitor(usb_driver_t *driver, const cli_args_t *args) {
    (void)driver; // USB driver not needed for monitoring
    
    if (strcmp(args->monitor_subcommand, "start") == 0) {
        if (g_monitor && device_monitor_is_running(g_monitor)) {
            printf("Device monitoring is already running\n");
            return 0;
        }
        
        device_monitor_config_t config = {
            .target_vendor_id = FANTECH_VENDOR_ID,
            .target_product_id = FANTECH_PRODUCT_ID,
            .poll_interval_ms = 500,
            .auto_apply_config = false
        };
        
        if (strlen(args->config_path) > 0) {
            strncpy(config.config_path, args->config_path, sizeof(config.config_path) - 1);
            config.auto_apply_config = true;
        }
        
        g_monitor = device_monitor_create(&config);
        if (!g_monitor) {
            RETURN_ERROR(ERROR_MEMORY, "Failed to create device monitor");
        }
        
        if (device_monitor_set_callback(g_monitor, cli_event_callback, NULL) != 0) {
            device_monitor_destroy(g_monitor);
            g_monitor = NULL;
            RETURN_ERROR(ERROR_GENERIC, "Failed to set monitor callback");
        }
        
        if (device_monitor_start(g_monitor) != 0) {
            device_monitor_destroy(g_monitor);
            g_monitor = NULL;
            RETURN_ERROR(ERROR_GENERIC, "Failed to start device monitoring");
        }
        
        printf("Device monitoring started%s\n", config.auto_apply_config ? " with auto-apply" : "");
        printf("Press Ctrl+C to stop monitoring\n");
        
        // Wait for interrupt
        while (device_monitor_is_running(g_monitor)) {
            sleep(1);
        }
        
    } else if (strcmp(args->monitor_subcommand, "stop") == 0) {
        if (!g_monitor) {
            printf("Device monitoring is not running\n");
            return 0;
        }
        
        device_monitor_stop(g_monitor);
        device_monitor_destroy(g_monitor);
        g_monitor = NULL;
        printf("Device monitoring stopped\n");
        
    } else if (strcmp(args->monitor_subcommand, "status") == 0) {
        if (!g_monitor) {
            printf("Device monitoring: Not started\n");
        } else if (device_monitor_is_running(g_monitor)) {
            printf("Device monitoring: Running\n");
            // Note: We can't access g_monitor->config directly due to incomplete type
            // This would need to be exposed via device_monitor API if needed
        } else {
            printf("Device monitoring: Stopped\n");
        }
        
    } else if (strcmp(args->monitor_subcommand, "events") == 0) {
        if (!g_monitor) {
            printf("Device monitoring is not running\n");
            return 0;
        }
        
        printf("Listening for events (timeout: %d seconds)...\n", args->monitor_timeout);
        
        device_event_t event;
        int timeout_ms = args->monitor_timeout * 1000;
        
        while (device_monitor_is_running(g_monitor) && timeout_ms > 0) {
            if (device_monitor_get_next_event(g_monitor, &event, 1000) == 0) {
                cli_event_callback(&event, NULL);
            }
            timeout_ms -= 1000;
        }
        
    } else {
        RETURN_ERROR_DETAILS(ERROR_INVALID_ARGS, "Unknown monitor subcommand", 
                           "Use: start, stop, status, or events");
    }
    
    return 0;
}

// Handle daemon command
int cmd_handle_daemon(usb_driver_t *driver, const cli_args_t *args) {
    (void)driver; // USB driver not needed for daemon mode
    
    device_monitor_config_t config = {
        .target_vendor_id = FANTECH_VENDOR_ID,
        .target_product_id = FANTECH_PRODUCT_ID,
        .poll_interval_ms = 1000,
        .auto_apply_config = false
    };
    
    if (strlen(args->config_path) > 0) {
        strncpy(config.config_path, args->config_path, sizeof(config.config_path) - 1);
        config.auto_apply_config = true;
    }
    
    g_monitor = device_monitor_create(&config);
    if (!g_monitor) {
        RETURN_ERROR(ERROR_MEMORY, "Failed to create device monitor");
    }
    
    if (device_monitor_start(g_monitor) != 0) {
        device_monitor_destroy(g_monitor);
        g_monitor = NULL;
        RETURN_ERROR(ERROR_GENERIC, "Failed to start daemon");
    }
    
    printf("Fantech X9 Thor daemon started%s\n", config.auto_apply_config ? " with auto-apply" : "");
    printf("Monitoring device changes...\n");
    
    // Run indefinitely
    while (device_monitor_is_running(g_monitor)) {
        sleep(1);
    }
    
    return 0;
}

