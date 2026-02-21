#ifndef USB_DRIVER_H
#define USB_DRIVER_H

#include <libusb-1.0/libusb.h>
#include "protocol.h"

// USB Driver structure
typedef struct {
    libusb_context *ctx;
    libusb_device_handle *device;
    int conquered;
    int device_busy;
} usb_driver_t;

// Function declarations
int usb_driver_init(usb_driver_t *driver);
void usb_driver_cleanup(usb_driver_t *driver);
int usb_driver_find_device(usb_driver_t *driver);
device_state_t usb_driver_check_state(usb_driver_t *driver);
int usb_driver_conquer(usb_driver_t *driver);
void usb_driver_liberate(usb_driver_t *driver);
int usb_driver_send_payload(usb_driver_t *driver, const usb_payload_t *payload);

// Payload creation functions
void usb_init_payload(usb_payload_t *payload, uint8_t instruction_code);
void usb_add_zero_bytes(usb_payload_t *payload, int count);
void usb_create_rgb_lights_config(usb_payload_t *payload, color_scheme_t scheme, int duration);
void usb_create_scrollwheel_config(usb_payload_t *payload, int is_volume);
void usb_create_dpi_profile_config(usb_payload_t *payload, int dpi, int profile);
void usb_create_color_profile_config(usb_payload_t *payload, int profile, uint8_t red, uint8_t green, uint8_t blue);

// Utility functions
int usb_find_closest_dpi(int dpi);
uint8_t usb_set_dpi_for_profile(int dpi, int profile);
uint8_t usb_set_active_profiles(const int profile_states[MAX_PROFILES]);
uint8_t usb_set_cyclic_colors(const int cyclic_colors[MAX_CYCLIC_COLORS]);

#endif // USB_DRIVER_H
