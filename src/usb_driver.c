#include "usb_driver.h"
#include "error.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Initialize USB driver
int usb_driver_init(usb_driver_t *driver) {
    if (!driver) RETURN_ERROR(ERROR_INVALID_ARGS, "Driver pointer is NULL");
    
    memset(driver, 0, sizeof(usb_driver_t));
    
    int result = libusb_init(&driver->ctx);
    if (result < 0) {
        RETURN_ERROR_USB(ERROR_USB_INIT, "Failed to initialize libusb", result);
    }
    
    return 0;
}

// Cleanup USB driver
void usb_driver_cleanup(usb_driver_t *driver) {
    if (!driver) return;
    
    if (driver->conquered) {
        usb_driver_liberate(driver);
    }
    
    if (driver->device) {
        libusb_close(driver->device);
    }
    
    if (driver->ctx) {
        libusb_exit(driver->ctx);
    }
    
    memset(driver, 0, sizeof(usb_driver_t));
}

// Find the Fantech X9 Thor device
int usb_driver_find_device(usb_driver_t *driver) {
    if (!driver || !driver->ctx) RETURN_ERROR(ERROR_INVALID_ARGS, "Invalid driver or context");
    
    printf("Trying to find device...\n");
    
    driver->device = libusb_open_device_with_vid_pid(driver->ctx, FANTECH_VENDOR_ID, FANTECH_PRODUCT_ID);
    
    if (!driver->device) {
        RETURN_ERROR(ERROR_DEVICE_NOT_FOUND, "Fantech X9 Thor device not found");
    }
    
    printf("Device found.\n");
    return 0;
}

// Check device state
device_state_t usb_driver_check_state(usb_driver_t *driver) {
    if (!driver || !driver->device) {
        return DEVICE_STATE_NOT_FOUND;
    }
    
    int result = libusb_kernel_driver_active(driver->device, W_INDEX);
    if (result < 0) {
        if (result == LIBUSB_ERROR_ACCESS) {
            ERROR_SET_USB(ERROR_DEVICE_ACCESS, "Permission denied accessing device", result);
            printf("Try adding a udev rule for your mouse or running as root.\n");
            return DEVICE_STATE_PERMISSION_ERROR;
        }
        RETURN_ERROR_USB(ERROR_DEVICE_NOT_FOUND, "Error checking kernel driver", result);
    }
    
    driver->device_busy = result;
    printf("Device is ready to be configured\n");
    return DEVICE_STATE_READY;
}

// Conquer device from kernel driver
int usb_driver_conquer(usb_driver_t *driver) {
    if (!driver || !driver->device) RETURN_ERROR(ERROR_INVALID_ARGS, "Invalid driver or device");
    
    if (driver->device_busy && !driver->conquered) {
        int result = libusb_detach_kernel_driver(driver->device, W_INDEX);
        if (result < 0) {
            RETURN_ERROR_USB(ERROR_USB_DETACH, "Failed to detach kernel driver", result);
        }
        
        result = libusb_claim_interface(driver->device, W_INDEX);
        if (result < 0) {
            RETURN_ERROR_USB(ERROR_USB_CLAIM, "Failed to claim interface", result);
        }
        
        driver->conquered = 1;
    }
    
    return 0;
}

// Release device back to kernel
void usb_driver_liberate(usb_driver_t *driver) {
    if (!driver || !driver->device) return;
    
    if (driver->conquered) {
        int result = libusb_release_interface(driver->device, W_INDEX);
        if (result < 0) {
            fprintf(stderr, "Failed to release interface: %s\n", libusb_error_name(result));
        } else {
            result = libusb_attach_kernel_driver(driver->device, W_INDEX);
            if (result < 0) {
                fprintf(stderr, "Failed to reattach kernel driver: %s\n", libusb_error_name(result));
            }
        }
        driver->conquered = 0;
    }
}

// Send payload to device
int usb_driver_send_payload(usb_driver_t *driver, const usb_payload_t *payload) {
    if (!driver || !driver->device || !payload) return -1;
    
    int result = libusb_control_transfer(
        driver->device,
        BM_REQUEST_TYPE,
        B_REQUEST,
        W_VALUE,
        W_INDEX,
        (unsigned char*)payload->data,
        payload->length,
        1000  // timeout in ms
    );
    
    if (result < 0) {
        RETURN_ERROR_USB(ERROR_USB_TRANSFER, "USB control transfer failed", result);
    }
    
    if (result != payload->length) {
        printf("Warning: Only %d of %zu bytes sent\n", result, payload->length);
    }
    
    return 0;
}

// Initialize payload with instruction code
void usb_init_payload(usb_payload_t *payload, uint8_t instruction_code) {
    if (!payload) return;
    
    memset(payload, 0, sizeof(usb_payload_t));
    payload->data[0] = 0x07;
    payload->data[1] = instruction_code;
    payload->length = 2;
}

// Add zero bytes to payload
void usb_add_zero_bytes(usb_payload_t *payload, int count) {
    if (!payload || count <= 0) return;
    
    for (int i = 0; i < count; i++) {
        if (payload->length < sizeof(payload->data)) {
            payload->data[payload->length++] = 0x00;
        }
    }
}

// Create RGB lights configuration payload
void usb_create_rgb_lights_config(usb_payload_t *payload, color_scheme_t scheme, int duration) {
    if (!payload) return;
    
    usb_init_payload(payload, INST_RGB_LIGHTS);
    
    // Add cyclic colors (placeholder - will be set properly)
    payload->data[payload->length++] = 0x00;
    
    // Add scheme and duration
    switch (scheme) {
        case COLOR_SCHEME_FIXED:
            payload->data[payload->length++] = 0x86 - duration;
            break;
        case COLOR_SCHEME_CYCLIC:
            payload->data[payload->length++] = 0x96 - duration;
            break;
        case COLOR_SCHEME_STATIC:
            payload->data[payload->length++] = 0x86;
            break;
        case COLOR_SCHEME_OFF:
            payload->data[payload->length++] = 0x87;
            break;
    }
    
    usb_add_zero_bytes(payload, 4);
}

// Create scrollwheel configuration payload
void usb_create_scrollwheel_config(usb_payload_t *payload, int is_volume) {
    if (!payload) return;
    
    usb_init_payload(payload, INST_SCROLLWHEEL);
    payload->data[payload->length++] = is_volume ? 0x01 : 0x00;
    usb_add_zero_bytes(payload, 5);
}

// Create DPI profile configuration payload
void usb_create_dpi_profile_config(usb_payload_t *payload, int dpi, int profile) {
    if (!payload) return;
    
    usb_init_payload(payload, INST_DPI_PROFILE);
    
    // Profile byte (simplified - using active profile 1)
    payload->data[payload->length++] = 0x40;
    
    // DPI byte
    payload->data[payload->length++] = usb_set_dpi_for_profile(dpi, profile);
    
    // Active profiles (placeholder - all enabled)
    payload->data[payload->length++] = 0x3F;  // All 6 profiles enabled
    
    usb_add_zero_bytes(payload, 3);
}

// Create color profile configuration payload
void usb_create_color_profile_config(usb_payload_t *payload, int profile, uint8_t red, uint8_t green, uint8_t blue) {
    if (!payload) return;
    
    if (profile < 1 || profile > 6) {
        fprintf(stderr, "Profile %d out of range (1-6)\n", profile);
        return;
    }
    
    usb_init_payload(payload, INST_COLOR_PROFILE);
    
    int internal_profile = (profile - 1) * 2;
    uint8_t internal_red = (255 - red) / 16;
    uint8_t internal_green = (255 - green) / 16;
    uint8_t internal_blue = (255 - blue) / 16;
    
    // First byte: profile + green
    uint8_t byte1 = internal_profile * 16 + internal_green;
    payload->data[payload->length++] = byte1;
    
    // Second byte: red + blue
    uint8_t byte2 = internal_red * 16 + internal_blue;
    payload->data[payload->length++] = byte2;
    
    // Active profiles (all enabled)
    payload->data[payload->length++] = 0x3F;
    
    usb_add_zero_bytes(payload, 3);
}

// Find closest supported DPI
int usb_find_closest_dpi(int dpi) {
    if (dpi < 200 || dpi > 4800) {
        return 1200;  // Default fallback
    }
    
    // Check if exact match exists
    for (int i = 0; i < SUPPORTED_DPIS_COUNT; i++) {
        if (SUPPORTED_DPIS[i] == dpi) {
            return dpi;
        }
    }
    
    // Find closest match
    int closest = SUPPORTED_DPIS[0];
    int min_diff = abs(dpi - closest);
    
    for (int i = 1; i < SUPPORTED_DPIS_COUNT; i++) {
        int diff = abs(dpi - SUPPORTED_DPIS[i]);
        if (diff < min_diff) {
            min_diff = diff;
            closest = SUPPORTED_DPIS[i];
        }
    }
    
    return closest;
}

// Set DPI for specific profile (returns internal byte value)
uint8_t usb_set_dpi_for_profile(int dpi, int profile) {
    int actual_dpi = usb_find_closest_dpi(dpi);
    uint8_t internal_dpi = 0;
    
    if (actual_dpi >= 200 && actual_dpi <= 1200) {
        internal_dpi = actual_dpi / 200;
    } else {
        switch (actual_dpi) {
            case 1600: internal_dpi = 0x7; break;
            case 2000: internal_dpi = 0x9; break;
            case 2400: internal_dpi = 0xb; break;
            case 3200: internal_dpi = 0xd; break;
            case 4000: internal_dpi = 0xe; break;
            case 4800: internal_dpi = 0xf; break;
            default: internal_dpi = 0x6; break;  // 1200 DPI default
        }
    }
    
    uint8_t internal_profile = profile + 7;
    return (internal_dpi * 16) + internal_profile;
}

// Set active profiles bitmask
uint8_t usb_set_active_profiles(const int profile_states[MAX_PROFILES]) {
    uint8_t byte = 0;
    for (int i = 0; i < MAX_PROFILES; i++) {
        if (profile_states[i]) {
            byte |= (1 << i);
        }
    }
    return byte;
}

// Set cyclic colors bitmask
uint8_t usb_set_cyclic_colors(const int cyclic_colors[MAX_CYCLIC_COLORS]) {
    uint8_t byte = 0;
    for (int i = 0; i < MAX_CYCLIC_COLORS; i++) {
        if (cyclic_colors[i]) {
            byte |= (1 << i);
        }
    }
    return byte;
}
