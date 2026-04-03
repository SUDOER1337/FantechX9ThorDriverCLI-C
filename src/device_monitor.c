#define _POSIX_C_SOURCE 199309L
#define _BSD_SOURCE
#include "device_monitor.h"
#include "error.h"
#include "config.h"
#include "usb_driver.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <libusb-1.0/libusb.h>
#include <sys/time.h>

// Device monitor structure
struct device_monitor {
    device_monitor_config_t config;
    bool running;
    bool thread_active;
    pthread_t monitor_thread;
    pthread_mutex_t event_mutex;
    pthread_cond_t event_cond;
    
    // Event queue
    device_event_t event_queue[10];
    int event_queue_head;
    int event_queue_tail;
    int event_queue_count;
    
    // Callback
    device_event_callback_t callback;
    void *callback_user_data;
    
    // libusb context for hotplug
    libusb_context *libusb_ctx;
    libusb_hotplug_callback_handle hotplug_handle;
};

// Get current timestamp
static void get_timestamp(char *buffer, size_t size) {
    struct timeval tv;
    struct tm *tm_info;
    
    gettimeofday(&tv, NULL);
    tm_info = localtime(&tv.tv_sec);
    
    snprintf(buffer, size, "%04d-%02d-%02d %02d:%02d:%02d.%03ld",
             tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
             tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec,
             tv.tv_usec / 1000);
}

// Add event to queue
static int add_event_to_queue(device_monitor_t *monitor, device_event_type_t type, const char *details) {
    if (!monitor || monitor->event_queue_count >= 10) return -1;
    
    pthread_mutex_lock(&monitor->event_mutex);
    
    device_event_t *event = &monitor->event_queue[monitor->event_queue_head];
    event->type = type;
    event->vendor_id = monitor->config.target_vendor_id;
    event->product_id = monitor->config.target_product_id;
    get_timestamp(event->timestamp, sizeof(event->timestamp));
    strncpy(event->details, details ? details : "", sizeof(event->details) - 1);
    event->details[sizeof(event->details) - 1] = '\0';
    
    monitor->event_queue_head = (monitor->event_queue_head + 1) % 10;
    monitor->event_queue_count++;
    
    pthread_cond_signal(&monitor->event_cond);
    pthread_mutex_unlock(&monitor->event_mutex);
    
    return 0;
}

// Hotplug callback function
static int LIBUSB_CALL hotplug_callback(libusb_context *ctx, libusb_device *device,
                                       libusb_hotplug_event event, void *user_data) {
    device_monitor_t *monitor = (device_monitor_t*)user_data;
    struct libusb_device_descriptor desc;
    
    if (libusb_get_device_descriptor(device, &desc) != 0) {
        return 0;
    }
    
    // Check if this is our target device
    if (desc.idVendor == monitor->config.target_vendor_id && 
        desc.idProduct == monitor->config.target_product_id) {
        
        if (event == LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED) {
            add_event_to_queue(monitor, DEVICE_EVENT_CONNECT, "Device connected");
            
            // Auto-apply configuration if enabled
            if (monitor->config.auto_apply_config && strlen(monitor->config.config_path) > 0) {
                printf("Auto-applying configuration: %s\n", monitor->config.config_path);
                
                usb_driver_t driver;
                if (usb_driver_init(&driver) == 0 && 
                    usb_driver_find_device(&driver) == 0 &&
                    usb_driver_conquer(&driver) == 0) {
                    
                    device_config_t config;
                    if (config_load_device_config(monitor->config.config_path, &config) == 0) {
                        // Apply DPI settings
                        for (int i = 0; i < MAX_PROFILES; i++) {
                            if (config.profiles[i].enabled) {
                                usb_payload_t dpi_payload;
                                usb_create_dpi_profile_config(&dpi_payload, config.profiles[i].dpi, i);
                                usb_driver_send_payload(&driver, &dpi_payload);
                            }
                        }
                        
                        // Apply color settings
                        for (int i = 0; i < MAX_PROFILES; i++) {
                            if (config.profiles[i].enabled) {
                                usb_payload_t color_payload;
                                usb_create_color_profile_config(&color_payload, i + 1,
                                                              config.profiles[i].red,
                                                              config.profiles[i].green,
                                                              config.profiles[i].blue);
                                usb_driver_send_payload(&driver, &color_payload);
                            }
                        }
                        
                        // Apply RGB lighting
                        usb_payload_t rgb_payload;
                        usb_create_rgb_lights_config(&rgb_payload, config.color_scheme, config.scheme_duration);
                        usb_driver_send_payload(&driver, &rgb_payload);
                        
                        printf("Configuration auto-applied successfully\n");
                    }
                    
                    usb_driver_liberate(&driver);
                }
                usb_driver_cleanup(&driver);
            }
            
        } else if (event == LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT) {
            add_event_to_queue(monitor, DEVICE_EVENT_DISCONNECT, "Device disconnected");
        }
        
        // Call user callback if set
        if (monitor->callback) {
            device_event_t event_data;
            pthread_mutex_lock(&monitor->event_mutex);
            if (monitor->event_queue_count > 0) {
                event_data = monitor->event_queue[(monitor->event_queue_tail + monitor->event_queue_count - 1) % 10];
            }
            pthread_mutex_unlock(&monitor->event_mutex);
            monitor->callback(&event_data, monitor->callback_user_data);
        }
    }
    
    return 0;
}

// Monitor thread function
static void* monitor_thread_func(void *arg) {
    device_monitor_t *monitor = (device_monitor_t*)arg;
    
    while (monitor->running) {
        libusb_handle_events_completed(monitor->libusb_ctx, NULL);
        usleep(monitor->config.poll_interval_ms * 1000);
    }
    
    return NULL;
}

// Create device monitor
device_monitor_t* device_monitor_create(const device_monitor_config_t *config) {
    if (!config) return NULL;
    
    device_monitor_t *monitor = calloc(1, sizeof(device_monitor_t));
    if (!monitor) return NULL;
    
    memcpy(&monitor->config, config, sizeof(device_monitor_config_t));
    
    // Initialize mutex and condition variable
    if (pthread_mutex_init(&monitor->event_mutex, NULL) != 0 ||
        pthread_cond_init(&monitor->event_cond, NULL) != 0) {
        free(monitor);
        return NULL;
    }
    
    // Initialize libusb
    if (libusb_init(&monitor->libusb_ctx) != 0) {
        pthread_mutex_destroy(&monitor->event_mutex);
        pthread_cond_destroy(&monitor->event_cond);
        free(monitor);
        return NULL;
    }
    
    return monitor;
}

// Destroy device monitor
void device_monitor_destroy(device_monitor_t *monitor) {
    if (!monitor) return;
    
    device_monitor_stop(monitor);
    
    if (monitor->libusb_ctx) {
        libusb_exit(monitor->libusb_ctx);
    }
    
    pthread_mutex_destroy(&monitor->event_mutex);
    pthread_cond_destroy(&monitor->event_cond);
    
    free(monitor);
}

// Start device monitoring
int device_monitor_start(device_monitor_t *monitor) {
    if (!monitor || monitor->running) RETURN_ERROR(ERROR_GENERIC, "Monitor already running or invalid");
    
    // Register hotplug callback
    int result = libusb_hotplug_register_callback(monitor->libusb_ctx,
                                                 LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT,
                                                 LIBUSB_HOTPLUG_ENUMERATE,
                                                 monitor->config.target_vendor_id,
                                                 monitor->config.target_product_id,
                                                 LIBUSB_HOTPLUG_MATCH_ANY,
                                                 hotplug_callback,
                                                 monitor,
                                                 &monitor->hotplug_handle);
    
    if (result != LIBUSB_SUCCESS) {
        RETURN_ERROR_USB(ERROR_USB_INIT, "Failed to register hotplug callback", result);
    }
    
    monitor->running = true;
    monitor->thread_active = true;
    
    // Start monitor thread
    if (pthread_create(&monitor->monitor_thread, NULL, monitor_thread_func, monitor) != 0) {
        monitor->running = false;
        monitor->thread_active = false;
        libusb_hotplug_deregister_callback(monitor->libusb_ctx, monitor->hotplug_handle);
        RETURN_ERROR_SYSTEM(ERROR_GENERIC, "Failed to create monitor thread", errno);
    }
    
    printf("Device monitoring started for VID:0x%04X PID:0x%04X\n", 
           monitor->config.target_vendor_id, monitor->config.target_product_id);
    
    return 0;
}

// Stop device monitoring
int device_monitor_stop(device_monitor_t *monitor) {
    if (!monitor || !monitor->running) return 0;
    
    monitor->running = false;
    
    // Wait for thread to finish
    if (monitor->thread_active) {
        pthread_join(monitor->monitor_thread, NULL);
        monitor->thread_active = false;
    }
    
    // Unregister hotplug callback
    if (monitor->hotplug_handle != 0) {
        libusb_hotplug_deregister_callback(monitor->libusb_ctx, monitor->hotplug_handle);
        monitor->hotplug_handle = 0;
    }
    
    printf("Device monitoring stopped\n");
    return 0;
}

// Check if monitor is running
bool device_monitor_is_running(const device_monitor_t *monitor) {
    return monitor ? monitor->running : false;
}

// Get next event from queue
int device_monitor_get_next_event(device_monitor_t *monitor, device_event_t *event, int timeout_ms) {
    if (!monitor || !event) RETURN_ERROR(ERROR_INVALID_ARGS, "Invalid monitor or event pointer");
    
    pthread_mutex_lock(&monitor->event_mutex);
    
    // Wait for event if queue is empty
    while (monitor->event_queue_count == 0 && monitor->running) {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += timeout_ms / 1000;
        ts.tv_nsec += (timeout_ms % 1000) * 1000000;
        if (ts.tv_nsec >= 1000000000) {
            ts.tv_sec++;
            ts.tv_nsec -= 1000000000;
        }
        
        if (pthread_cond_timedwait(&monitor->event_cond, &monitor->event_mutex, &ts) != 0) {
            pthread_mutex_unlock(&monitor->event_mutex);
            return -1; // Timeout
        }
    }
    
    if (monitor->event_queue_count == 0) {
        pthread_mutex_unlock(&monitor->event_mutex);
        return -1; // No events available
    }
    
    // Copy event from queue
    *event = monitor->event_queue[monitor->event_queue_tail];
    monitor->event_queue_tail = (monitor->event_queue_tail + 1) % 10;
    monitor->event_queue_count--;
    
    pthread_mutex_unlock(&monitor->event_mutex);
    
    return 0;
}

// Set event callback
int device_monitor_set_callback(device_monitor_t *monitor, device_event_callback_t callback, void *user_data) {
    if (!monitor) RETURN_ERROR(ERROR_INVALID_ARGS, "Invalid monitor pointer");
    
    monitor->callback = callback;
    monitor->callback_user_data = user_data;
    
    return 0;
}

// Configure auto-apply on connect
int device_monitor_apply_config_on_connect(device_monitor_t *monitor, const char *config_path) {
    if (!monitor || !config_path) RETURN_ERROR(ERROR_INVALID_ARGS, "Invalid monitor or config path");
    
    strncpy(monitor->config.config_path, config_path, sizeof(monitor->config.config_path) - 1);
    monitor->config.config_path[sizeof(monitor->config.config_path) - 1] = '\0';
    monitor->config.auto_apply_config = true;
    
    return 0;
}
