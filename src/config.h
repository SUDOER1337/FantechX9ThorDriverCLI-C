#ifndef CONFIG_H
#define CONFIG_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "protocol.h"

// Configuration file paths
#define CONFIG_FILENAME "driver.conf"
#define USER_CONFIG_DIR ".config/fantech-x9-thor"
#define USER_CONFIG_FILENAME "config.conf"
#define PRESETS_DIR "presets"
#define MAX_PRESETS 50
#define MAX_PRESET_NAME 64

// Maximum line length for config parsing
#define MAX_CONFIG_LINE 256

// Configuration structure
typedef struct {
    char config_path[512];
    char user_config_dir[512];
    char local_config_path[512];
    char user_config_path[512];
    char presets_dir[512];
} config_paths_t;

// Preset structure
typedef struct {
    char name[MAX_PRESET_NAME];
    char path[512];
    char description[256];
} preset_info_t;

// Function declarations
int config_get_paths(config_paths_t *paths);
int config_load_device_config(const char *filename, device_config_t *config);
int config_save_device_config(const char *filename, const device_config_t *config);
int config_create_default(const char *filename);
int config_migrate_to_user_config(const char *source_path);
int config_get_active_config_path(char *config_path, size_t max_len);

// Utility functions
int config_parse_rgb(const char *rgb_str, uint8_t *red, uint8_t *green, uint8_t *blue);
void config_format_rgb(char *buffer, size_t size, uint8_t red, uint8_t green, uint8_t blue);
const char* config_color_scheme_to_string(color_scheme_t scheme);
color_scheme_t config_string_to_color_scheme(const char *scheme_str);

// Preset management functions
int config_list_presets(preset_info_t *presets, int *count);
int config_create_preset_directory(void);
int config_save_preset(const char *name, const char *description, const device_config_t *config);
int config_load_preset(const char *name, device_config_t *config);

#endif // CONFIG_H
