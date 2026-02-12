/*
 * smartlog/config.h
 *
 * Configuration and settings for SmartLog.
 *
 * Defines:
 *   - Buffer and file size limits
 *   - File permission settings
 *   - Feature enums for flags
 *
 * Author: Aravinthraj Ganesan
 * Version: 4.0
 */

#ifndef SMARTLOG_CONFIG_H
#define SMARTLOG_CONFIG_H

#include <sys/stat.h>

/* ============================================================================
 * Buffer and File Settings
 * ============================================================================ */

#define SMARTLOG_MSG_MAX_LEN    256   /* Max message length */
#define SMARTLOG_PATH_MAX_LEN   4096  /* Max file path length */
#define SMARTLOG_LOG_BUFFER_SZ  1024  /* Internal buffer size */

#define SMARTLOG_TIMESTAMP_ENABLED 1  /* Always use timestamps */
#define SMARTLOG_FILE_MODE (S_IRUSR | S_IWUSR | S_IRGRP)  /* rw-r----- */

/* ============================================================================
 * Feature Flags
 * ============================================================================ */

typedef enum {
    FEATURE_DISABLED = 0,   /* Off */
    FEATURE_ENABLED = 1     /* On */
} feature_state_t;

typedef enum {
    DIR_CURRENT_WORKING = 0xAA,  /* Current directory */
    DIR_USER_INPUT = 0xBB        /* User directory */
} directory_type_t;

#endif /* SMARTLOG_CONFIG_H */
