#ifndef DEVICE_MONITOR_H
#define DEVICE_MONITOR_H

#include <stdint.h>
#include <stdbool.h>
#include "usb_driver.h"

// Device event types
typedef enum {
    DEVICE_EVENT_CONNECT = 1,
    DEVICE_EVENT_DISCONNECT = 2,
    DEVICE_EVENT_ERROR = 3
} device_event_type_t;

// Device event structure
typedef struct {
    device_event_type_t type;
    uint16_t vendor_id;
    uint16_t product_id;
    char timestamp[64];
    char details[256];
} device_event_t;

// Device monitor configuration
typedef struct {
    uint16_t target_vendor_id;
    uint16_t target_product_id;
    int poll_interval_ms;
    bool auto_apply_config;
    char config_path[512];
} device_monitor_config_t;

// Device monitor handle
typedef struct device_monitor device_monitor_t;

// Function declarations
device_monitor_t* device_monitor_create(const device_monitor_config_t *config);
void device_monitor_destroy(device_monitor_t *monitor);
int device_monitor_start(device_monitor_t *monitor);
int device_monitor_stop(device_monitor_t *monitor);
bool device_monitor_is_running(const device_monitor_t *monitor);
int device_monitor_get_next_event(device_monitor_t *monitor, device_event_t *event, int timeout_ms);
int device_monitor_apply_config_on_connect(device_monitor_t *monitor, const char *config_path);

// Callback function for device events
typedef void (*device_event_callback_t)(const device_event_t *event, void *user_data);
int device_monitor_set_callback(device_monitor_t *monitor, device_event_callback_t callback, void *user_data);

#endif // DEVICE_MONITOR_H
