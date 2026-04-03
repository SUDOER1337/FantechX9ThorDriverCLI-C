#include "error.h"

// Global error context
error_context_t g_last_error = {0};

// Convert error code to string
const char* error_code_to_string(error_code_t code) {
    switch (code) {
        case ERROR_NONE: return "Success";
        case ERROR_GENERIC: return "Generic error";
        case ERROR_INVALID_ARGS: return "Invalid arguments";
        case ERROR_DEVICE_NOT_FOUND: return "Device not found";
        case ERROR_DEVICE_ACCESS: return "Device access denied";
        case ERROR_DEVICE_BUSY: return "Device busy";
        case ERROR_USB_TRANSFER: return "USB transfer failed";
        case ERROR_CONFIG_LOAD: return "Failed to load configuration";
        case ERROR_CONFIG_SAVE: return "Failed to save configuration";
        case ERROR_CONFIG_PARSE: return "Configuration parse error";
        case ERROR_OUT_OF_RANGE: return "Value out of range";
        case ERROR_MEMORY: return "Memory allocation error";
        case ERROR_FILE_NOT_FOUND: return "File not found";
        case ERROR_PERMISSION_DENIED: return "Permission denied";
        case ERROR_USB_INIT: return "USB initialization failed";
        case ERROR_USB_CLAIM: return "Failed to claim USB interface";
        case ERROR_USB_DETACH: return "Failed to detach USB driver";
        case ERROR_USB_RELEASE: return "Failed to release USB interface";
        case ERROR_VERSION_REQUEST: return "Version information requested";
        default: return "Unknown error";
    }
}

// Set error context
void error_set(error_code_t code, const char *function, const char *message, const char *details) {
    memset(&g_last_error, 0, sizeof(g_last_error));
    g_last_error.code = code;
    strncpy(g_last_error.function, function ? function : "unknown", sizeof(g_last_error.function) - 1);
    strncpy(g_last_error.message, message ? message : "No message", sizeof(g_last_error.message) - 1);
    if (details) {
        strncpy(g_last_error.details, details, sizeof(g_last_error.details) - 1);
    }
    g_last_error.system_errno = errno;
}

// Set error context with USB error
void error_set_usb(error_code_t code, const char *function, const char *message, int libusb_errno) {
    memset(&g_last_error, 0, sizeof(g_last_error));
    g_last_error.code = code;
    strncpy(g_last_error.function, function ? function : "unknown", sizeof(g_last_error.function) - 1);
    strncpy(g_last_error.message, message ? message : "No message", sizeof(g_last_error.message) - 1);
    g_last_error.libusb_errno = libusb_errno;
    
    // Add libusb error details
    if (libusb_errno != 0) {
        snprintf(g_last_error.details, sizeof(g_last_error.details), 
                "libusb error: %s (%d)", libusb_error_name(libusb_errno), libusb_errno);
    }
}

// Set error context with system error
void error_set_system(error_code_t code, const char *function, const char *message, int system_errno) {
    memset(&g_last_error, 0, sizeof(g_last_error));
    g_last_error.code = code;
    strncpy(g_last_error.function, function ? function : "unknown", sizeof(g_last_error.function) - 1);
    strncpy(g_last_error.message, message ? message : "No message", sizeof(g_last_error.message) - 1);
    g_last_error.system_errno = system_errno;
    
    // Add system error details
    if (system_errno != 0) {
        snprintf(g_last_error.details, sizeof(g_last_error.details), 
                "System error: %s (%d)", strerror(system_errno), system_errno);
    }
}

// Print full error information
void error_print(void) {
    if (g_last_error.code == ERROR_NONE) {
        return;
    }
    
    fprintf(stderr, "\n");
    fprintf(stderr, "ERROR: %s\n", error_code_to_string(g_last_error.code));
    fprintf(stderr, "Message: %s\n", g_last_error.message);
    
    if (strlen(g_last_error.function) > 0) {
        fprintf(stderr, "Function: %s\n", g_last_error.function);
    }
    
    if (strlen(g_last_error.details) > 0) {
        fprintf(stderr, "Details: %s\n", g_last_error.details);
    }
    
    if (g_last_error.system_errno != 0) {
        fprintf(stderr, "System errno: %d (%s)\n", g_last_error.system_errno, strerror(g_last_error.system_errno));
    }
    
    if (g_last_error.libusb_errno != 0) {
        fprintf(stderr, "libusb errno: %d (%s)\n", g_last_error.libusb_errno, libusb_error_name(g_last_error.libusb_errno));
    }
    
    fprintf(stderr, "\n");
}

// Print brief error message
void error_print_brief(void) {
    if (g_last_error.code == ERROR_NONE) {
        return;
    }
    
    fprintf(stderr, "Error: %s", g_last_error.message);
    
    if (g_last_error.libusb_errno != 0) {
        fprintf(stderr, " (%s)", libusb_error_name(g_last_error.libusb_errno));
    } else if (g_last_error.system_errno != 0) {
        fprintf(stderr, " (%s)", strerror(g_last_error.system_errno));
    }
    
    fprintf(stderr, "\n");
}

// Get last error message
const char* error_get_last_message(void) {
    return g_last_error.message;
}
