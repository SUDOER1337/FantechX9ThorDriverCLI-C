#include "config.h"
#include <sys/stat.h>
#include <errno.h>
#include <libgen.h>
#include <string.h>
#include <dirent.h>

// Get configuration file paths
int config_get_paths(config_paths_t *paths) {
    if (!paths) return -1;
    
    memset(paths, 0, sizeof(config_paths_t));
    
    // Local config path (current directory)
    snprintf(paths->local_config_path, sizeof(paths->local_config_path), "%s", CONFIG_FILENAME);
    
    // User config directory
    const char *home = getenv("HOME");
    if (!home) {
        fprintf(stderr, "HOME environment variable not set\n");
        return -1;
    }
    
    snprintf(paths->user_config_dir, sizeof(paths->user_config_dir), "%s/%s", home, USER_CONFIG_DIR);
    snprintf(paths->user_config_path, sizeof(paths->user_config_path), "%s/%s", paths->user_config_dir, USER_CONFIG_FILENAME);
    snprintf(paths->presets_dir, sizeof(paths->presets_dir), "%s/%s", paths->user_config_dir, PRESETS_DIR);
    
    return 0;
}

// Get active configuration file path (priority: local -> user)
int config_get_active_config_path(char *config_path, size_t max_len) {
    if (!config_path || max_len == 0) return -1;
    
    config_paths_t paths;
    if (config_get_paths(&paths) != 0) return -1;
    
    // Check local config first
    struct stat st;
    if (stat(paths.local_config_path, &st) == 0) {
        strncpy(config_path, paths.local_config_path, max_len - 1);
        config_path[max_len - 1] = '\0';
        return 0;
    }
    
    // Check user config
    if (stat(paths.user_config_path, &st) == 0) {
        strncpy(config_path, paths.user_config_path, max_len - 1);
        config_path[max_len - 1] = '\0';
        return 0;
    }
    
    // Default to user config path
    strncpy(config_path, paths.user_config_path, max_len - 1);
    config_path[max_len - 1] = '\0';
    return 0;
}

// Parse RGB string format "rgb(r,g,b)"
int config_parse_rgb(const char *rgb_str, uint8_t *red, uint8_t *green, uint8_t *blue) {
    if (!rgb_str || !red || !green || !blue) return -1;
    
    int r, g, b;
    if (sscanf(rgb_str, "rgb(%d,%d,%d)", &r, &g, &b) != 3) {
        return -1;
    }
    
    if (r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255) {
        return -1;
    }
    
    *red = (uint8_t)r;
    *green = (uint8_t)g;
    *blue = (uint8_t)b;
    
    return 0;
}

// Format RGB values to string
void config_format_rgb(char *buffer, size_t size, uint8_t red, uint8_t green, uint8_t blue) {
    if (!buffer || size == 0) return;
    
    snprintf(buffer, size, "rgb(%d,%d,%d)", red, green, blue);
}

// Convert color scheme enum to string
const char* config_color_scheme_to_string(color_scheme_t scheme) {
    switch (scheme) {
        case COLOR_SCHEME_FIXED: return "Fixed";
        case COLOR_SCHEME_CYCLIC: return "Cyclic";
        case COLOR_SCHEME_STATIC: return "Static";
        case COLOR_SCHEME_OFF: return "Off";
        default: return "Static";
    }
}

// Convert string to color scheme enum
color_scheme_t config_string_to_color_scheme(const char *scheme_str) {
    if (!scheme_str) return COLOR_SCHEME_STATIC;
    
    if (strcmp(scheme_str, "Fixed") == 0) return COLOR_SCHEME_FIXED;
    if (strcmp(scheme_str, "Cyclic") == 0) return COLOR_SCHEME_CYCLIC;
    if (strcmp(scheme_str, "Static") == 0) return COLOR_SCHEME_STATIC;
    if (strcmp(scheme_str, "Off") == 0) return COLOR_SCHEME_OFF;
    
    return COLOR_SCHEME_STATIC;  // Default
}

// Load device configuration from file
int config_load_device_config(const char *filename, device_config_t *config) {
    if (!filename || !config) return -1;
    
    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Cannot open config file: %s\n", filename);
        return -1;
    }
    
    // Initialize with defaults
    memset(config, 0, sizeof(device_config_t));
    config->active_profile = 1;
    config->color_scheme = COLOR_SCHEME_STATIC;
    config->scheme_duration = 6;
    
    for (int i = 0; i < MAX_PROFILES; i++) {
        config->profiles[i].dpi = 1200;
        config->profiles[i].red = 255;
        config->profiles[i].green = 73;
        config->profiles[i].blue = 0;
        config->profiles[i].enabled = 1;
    }
    
    char line[MAX_CONFIG_LINE];
    char section[64] = "";
    
    while (fgets(line, sizeof(line), file)) {
        // Remove newline
        line[strcspn(line, "\n")] = 0;
        
        // Skip empty lines and comments
        if (strlen(line) == 0 || line[0] == ';' || line[0] == '#') {
            continue;
        }
        
        // Section header
        if (line[0] == '[' && line[strlen(line) - 1] == ']') {
            strncpy(section, line + 1, strlen(line) - 2);
            section[strlen(line) - 2] = '\0';
            continue;
        }
        
        // Key-value pair
        char *key = strtok(line, "=");
        char *value = strtok(NULL, "=");
        
        if (!key || !value) continue;
        
        // Trim whitespace
        while (*key == ' ' || *key == '\t') key++;
        while (*value == ' ' || *value == '\t') value++;
        
        if (strcmp(section, "Active_Profile") == 0) {
            if (strcmp(key, "profile") == 0) {
                config->active_profile = atoi(value);
            }
        } else if (strcmp(section, "Profile_DPIs") == 0) {
            if (strncmp(key, "profile_", 8) == 0) {
                int profile_num = atoi(key + 8) - 1;
                if (profile_num >= 0 && profile_num < MAX_PROFILES) {
                    config->profiles[profile_num].dpi = atoi(value);
                }
            }
        } else if (strcmp(section, "Profile_States") == 0) {
            if (strncmp(key, "profile_", 8) == 0) {
                int profile_num = atoi(key + 8) - 1;
                if (profile_num >= 0 && profile_num < MAX_PROFILES) {
                    config->profiles[profile_num].enabled = atoi(value);
                }
            }
        } else if (strcmp(section, "Profile_Colors") == 0) {
            if (strncmp(key, "profile_", 8) == 0) {
                int profile_num = atoi(key + 8) - 1;
                if (profile_num >= 0 && profile_num < MAX_PROFILES) {
                    config_parse_rgb(value, 
                                   &config->profiles[profile_num].red,
                                   &config->profiles[profile_num].green,
                                   &config->profiles[profile_num].blue);
                }
            }
        } else if (strcmp(section, "Color_Scheme") == 0) {
            if (strcmp(key, "type") == 0) {
                config->color_scheme = config_string_to_color_scheme(value);
            } else if (strcmp(key, "duration") == 0) {
                config->scheme_duration = atoi(value);
            }
        } else if (strcmp(section, "Cyclic_Colors") == 0) {
            const char *colors[] = {"yellow", "blue", "violet", "green", "red", "cyan", "white"};
            for (int i = 0; i < MAX_CYCLIC_COLORS; i++) {
                if (strcmp(key, colors[i]) == 0) {
                    config->cyclic_colors[i] = atoi(value);
                    break;
                }
            }
        }
    }
    
    fclose(file);
    return 0;
}

// Save device configuration to file
int config_save_device_config(const char *filename, const device_config_t *config) {
    if (!filename || !config) return -1;
    
    FILE *file = fopen(filename, "w");
    if (!file) {
        fprintf(stderr, "Cannot create config file: %s\n", filename);
        return -1;
    }
    
    fprintf(file, "[Active_Profile]\n");
    fprintf(file, "profile = %d\n\n", config->active_profile);
    
    fprintf(file, "[Profile_DPIs]\n");
    for (int i = 0; i < MAX_PROFILES; i++) {
        fprintf(file, "profile_%d = %d\n", i + 1, config->profiles[i].dpi);
    }
    fprintf(file, "\n");
    
    fprintf(file, "[Profile_States]\n");
    for (int i = 0; i < MAX_PROFILES; i++) {
        fprintf(file, "profile_%d = %d\n", i + 1, config->profiles[i].enabled);
    }
    fprintf(file, "\n");
    
    fprintf(file, "[Profile_Colors]\n");
    for (int i = 0; i < MAX_PROFILES; i++) {
        char rgb_str[32];
        config_format_rgb(rgb_str, sizeof(rgb_str), 
                         config->profiles[i].red,
                         config->profiles[i].green,
                         config->profiles[i].blue);
        fprintf(file, "profile_%d = %s\n", i + 1, rgb_str);
    }
    fprintf(file, "\n");
    
    fprintf(file, "[Color_Scheme]\n");
    fprintf(file, "type = %s\n", config_color_scheme_to_string(config->color_scheme));
    fprintf(file, "duration = %d\n\n", config->scheme_duration);
    
    fprintf(file, "[Cyclic_Colors]\n");
    const char *colors[] = {"yellow", "blue", "violet", "green", "red", "cyan", "white"};
    for (int i = 0; i < MAX_CYCLIC_COLORS; i++) {
        fprintf(file, "%s = %d\n", colors[i], config->cyclic_colors[i]);
    }
    
    fclose(file);
    return 0;
}

// Create default configuration file
int config_create_default(const char *filename) {
    if (!filename) return -1;
    
    device_config_t default_config;
    memset(&default_config, 0, sizeof(device_config_t));
    
    default_config.active_profile = 1;
    default_config.color_scheme = COLOR_SCHEME_STATIC;
    default_config.scheme_duration = 6;
    
    // Default profile settings
    int default_dpis[] = {200, 600, 1200, 1600, 2400, 4000};
    for (int i = 0; i < MAX_PROFILES; i++) {
        default_config.profiles[i].dpi = default_dpis[i];
        default_config.profiles[i].red = 255;
        default_config.profiles[i].green = 73;
        default_config.profiles[i].blue = 0;
        default_config.profiles[i].enabled = 1;
    }
    
    // Create directory if needed - simpler approach
    char *last_slash = strrchr(filename, '/');
    if (last_slash) {
        size_t dir_len = last_slash - filename;
        char *dir = malloc(dir_len + 1);
        if (dir) {
            strncpy(dir, filename, dir_len);
            dir[dir_len] = '\0';
            mkdir(dir, 0755);
            free(dir);
        }
    }
    
    return config_save_device_config(filename, &default_config);
}

// Migrate local config to user config directory
int config_migrate_to_user_config(const char *source_path) {
    if (!source_path) return -1;
    
    config_paths_t paths;
    if (config_get_paths(&paths) != 0) return -1;
    
    // Check if source exists
    struct stat st;
    if (stat(source_path, &st) != 0) {
        fprintf(stderr, "Source config not found: %s\n", source_path);
        return -1;
    }
    
    // Create user config directory
    mkdir(paths.user_config_dir, 0755);
    
    // Copy file
    FILE *src = fopen(source_path, "r");
    FILE *dst = fopen(paths.user_config_path, "w");
    
    if (!src || !dst) {
        if (src) fclose(src);
        if (dst) fclose(dst);
        return -1;
    }
    
    char buffer[1024];
    size_t bytes;
    while ((bytes = fread(buffer, 1, sizeof(buffer), src)) > 0) {
        fwrite(buffer, 1, bytes, dst);
    }
    
    fclose(src);
    fclose(dst);
    
    printf("Migrated config from %s to %s\n", source_path, paths.user_config_path);
    return 0;
}

// Create preset directory
int config_create_preset_directory(void) {
    config_paths_t paths;
    if (config_get_paths(&paths) != 0) return -1;
    
    // Create user config directory if it doesn't exist
    mkdir(paths.user_config_dir, 0755);
    
    // Create presets directory
    if (mkdir(paths.presets_dir, 0755) != 0 && errno != EEXIST) {
        fprintf(stderr, "Could not create presets directory: %s\n", paths.presets_dir);
        return -1;
    }
    
    return 0;
}

// List available presets
int config_list_presets(preset_info_t *presets, int *count) {
    if (!presets || !count) return -1;
    
    *count = 0;
    
    config_paths_t paths;
    if (config_get_paths(&paths) != 0) return -1;
    
    // Create presets directory if it doesn't exist
    config_create_preset_directory();
    
    DIR *dir = opendir(paths.presets_dir);
    if (!dir) {
        fprintf(stderr, "Could not open presets directory: %s\n", paths.presets_dir);
        return -1;
    }
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL && *count < MAX_PRESETS) {
        // Check for .conf files
        size_t len = strlen(entry->d_name);
        if (len > 5 && strcmp(entry->d_name + len - 5, ".conf") == 0) {
            // Extract preset name (filename without .conf extension)
            strncpy(presets[*count].name, entry->d_name, MAX_PRESET_NAME - 1);
            presets[*count].name[MAX_PRESET_NAME - 1] = '\0';
            presets[*count].name[len - 5] = '\0';  // Remove .conf extension
            
            // Build full path
            snprintf(presets[*count].path, sizeof(presets[*count].path), 
                    "%s/%s", paths.presets_dir, entry->d_name);
            
            // Try to read description from file (first non-comment line that looks like a description)
            FILE *file = fopen(presets[*count].path, "r");
            if (file) {
                char line[MAX_CONFIG_LINE];
                presets[*count].description[0] = '\0';
                
                while (fgets(line, sizeof(line), file) && presets[*count].description[0] == '\0') {
                    // Remove newline
                    line[strcspn(line, "\n")] = 0;
                    
                    // Skip empty lines and comments
                    if (strlen(line) == 0 || line[0] == ';' || line[0] == '#' || line[0] == '[') {
                        continue;
                    }
                    
                    // Use first non-section line as description
                    if (strchr(line, '=')) {
                        // Skip key-value pairs, look for something that might be a description
                        continue;
                    }
                    
                    strncpy(presets[*count].description, line, sizeof(presets[*count].description) - 1);
                    presets[*count].description[sizeof(presets[*count].description) - 1] = '\0';
                }
                
                // If no description found, use a default
                if (presets[*count].description[0] == '\0') {
                    snprintf(presets[*count].description, sizeof(presets[*count].description), 
                            "Preset configuration for %s", presets[*count].name);
                }
                
                fclose(file);
            }
            
            (*count)++;
        }
    }
    
    closedir(dir);
    return 0;
}

// Save preset
int config_save_preset(const char *name, const char *description, const device_config_t *config) {
    if (!name || !config) return -1;
    
    config_paths_t paths;
    if (config_get_paths(&paths) != 0) return -1;
    
    // Create presets directory if needed
    config_create_preset_directory();
    
    // Build preset file path
    char preset_path[512];
    snprintf(preset_path, sizeof(preset_path), "%s/%s.conf", paths.presets_dir, name);
    
    // Save configuration to preset file
    if (config_save_device_config(preset_path, config) != 0) {
        return -1;
    }
    
    // If description provided, prepend it as a comment
    if (description && strlen(description) > 0) {
        FILE *file = fopen(preset_path, "r");
        if (file) {
            // Read existing content
            fseek(file, 0, SEEK_END);
            long file_size = ftell(file);
            fseek(file, 0, SEEK_SET);
            
            char *content = malloc(file_size + 1);
            if (content) {
                fread(content, 1, file_size, file);
                content[file_size] = '\0';
                fclose(file);
                
                // Write back with description
                file = fopen(preset_path, "w");
                if (file) {
                    fprintf(file, "; %s\n", description);
                    fprintf(file, "%s", content);
                    fclose(file);
                }
                
                free(content);
            }
        }
    }
    
    printf("Preset '%s' saved to %s\n", name, preset_path);
    return 0;
}

// Load preset
int config_load_preset(const char *name, device_config_t *config) {
    if (!name || !config) return -1;
    
    config_paths_t paths;
    if (config_get_paths(&paths) != 0) return -1;
    
    // Build preset file path
    char preset_path[512];
    snprintf(preset_path, sizeof(preset_path), "%s/%s.conf", paths.presets_dir, name);
    
    // Load configuration from preset file
    return config_load_device_config(preset_path, config);
}
