#ifndef ERROR_H
#define ERROR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <libusb-1.0/libusb.h>

// Error codes
typedef enum {
    ERROR_NONE = 0,
    ERROR_GENERIC = -1,
    ERROR_INVALID_ARGS = -2,
    ERROR_DEVICE_NOT_FOUND = -3,
    ERROR_DEVICE_ACCESS = -4,
    ERROR_DEVICE_BUSY = -5,
    ERROR_USB_TRANSFER = -6,
    ERROR_CONFIG_LOAD = -7,
    ERROR_CONFIG_SAVE = -8,
    ERROR_CONFIG_PARSE = -9,
    ERROR_OUT_OF_RANGE = -10,
    ERROR_MEMORY = -11,
    ERROR_FILE_NOT_FOUND = -12,
    ERROR_PERMISSION_DENIED = -13,
    ERROR_USB_INIT = -14,
    ERROR_USB_CLAIM = -15,
    ERROR_USB_DETACH = -16,
    ERROR_USB_RELEASE = -17,
    ERROR_VERSION_REQUEST = -18
} error_code_t;

// Error context structure
typedef struct {
    error_code_t code;
    char function[64];
    char message[256];
    char details[512];
    int system_errno;
    int libusb_errno;
} error_context_t;

// Global error context
extern error_context_t g_last_error;

// Error reporting functions
void error_set(error_code_t code, const char *function, const char *message, const char *details);
void error_set_usb(error_code_t code, const char *function, const char *message, int libusb_errno);
void error_set_system(error_code_t code, const char *function, const char *message, int system_errno);
void error_print(void);
void error_print_brief(void);
const char* error_code_to_string(error_code_t code);
const char* error_get_last_message(void);

// Error reporting macros
#define ERROR_SET(code, msg) error_set(code, __func__, msg, NULL)
#define ERROR_SET_DETAILS(code, msg, details) error_set(code, __func__, msg, details)
#define ERROR_SET_USB(code, msg, usb_err) error_set_usb(code, __func__, msg, usb_err)
#define ERROR_SET_SYSTEM(code, msg, sys_err) error_set_system(code, __func__, msg, sys_err)

#define ERROR_PRINT() error_print()
#define ERROR_PRINT_BRIEF() error_print_brief()

#define RETURN_ERROR(code, msg) do { ERROR_SET(code, msg); return code; } while(0)
#define RETURN_ERROR_DETAILS(code, msg, details) do { ERROR_SET_DETAILS(code, msg, details); return code; } while(0)
#define RETURN_ERROR_USB(code, msg, usb_err) do { ERROR_SET_USB(code, msg, usb_err); return code; } while(0)
#define RETURN_ERROR_SYSTEM(code, msg, sys_err) do { ERROR_SET_SYSTEM(code, msg, sys_err); return code; } while(0)

#endif // ERROR_H
