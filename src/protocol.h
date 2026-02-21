#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include <stddef.h>

// USB Device identifiers
#define FANTECH_VENDOR_ID     0x18f8
#define FANTECH_PRODUCT_ID    0x0fc0

// USB Control transfer parameters
#define BM_REQUEST_TYPE       0x21
#define B_REQUEST             0x09
#define W_VALUE               0x0307
#define W_INDEX               0x0001

// Instruction codes
#define INST_RGB_LIGHTS       0x13
#define INST_SCROLLWHEEL      0x11
#define INST_DPI_PROFILE      0x09
#define INST_COLOR_PROFILE    0x14

// Supported DPI values
static const int SUPPORTED_DPIS[] = {200, 400, 600, 800, 1000, 1200, 1600, 2000, 2400, 3200, 4000, 4800};
#define SUPPORTED_DPIS_COUNT  12

// Profile configuration
#define MAX_PROFILES          6
#define MAX_CYCLIC_COLORS     7

// Color scheme types
typedef enum {
    COLOR_SCHEME_FIXED,
    COLOR_SCHEME_CYCLIC,
    COLOR_SCHEME_STATIC,
    COLOR_SCHEME_OFF
} color_scheme_t;

// Device state
typedef enum {
    DEVICE_STATE_READY = 1,
    DEVICE_STATE_PERMISSION_ERROR = -1,
    DEVICE_STATE_NOT_FOUND = -2
} device_state_t;

// Profile configuration structure
typedef struct {
    int dpi;
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    int enabled;
} profile_config_t;

// Device configuration structure
typedef struct {
    int active_profile;
    profile_config_t profiles[MAX_PROFILES];
    color_scheme_t color_scheme;
    int scheme_duration;
    int cyclic_colors[MAX_CYCLIC_COLORS];
} device_config_t;

// USB payload structure
typedef struct {
    uint8_t data[64];
    size_t length;
} usb_payload_t;

#endif // PROTOCOL_H
