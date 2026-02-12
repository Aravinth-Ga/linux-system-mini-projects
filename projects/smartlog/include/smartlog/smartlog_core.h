/*
 * include/smartlog/smartlog_core.h
 *
 * Main API for SmartLog logging.
 *
 * Provides function to write log entries. Features:
 *   - Check file and permissions
 *   - Format message with time and process ID
 *   - Auto rotate file if too big
 *   - Optionally sync to disk for safety
 *
 * This is the main function that can be used in programs and libraries.
 *
 * Author: Aravinthraj Ganesan
 * Version: 4.0
 */

#ifndef SMARTLOG_CORE_H
#define SMARTLOG_CORE_H

#include "config.h"

/* ============================================================================
 * Core Logging Functions
 * ============================================================================ */

/**
 * Write a log entry to a file.
 *
 * Parameters:
 *   file_path        - Path to log file
 *   msg              - Log message (max 256 bytes)
 *   durable          - If on, sync to disk for safety
 *   max_bytes_config - If on, rotate file when too big
 *   max_byte_val     - Max size before rotation
 *
 * Return: 0 on success, 1 on error
 *
 * What it does:
 *   - Check if file exists and show permissions
 *   - Rotate file if it is too big
 *   - Write message with timestamp and process ID
 *   - Sync to disk if durable mode is on
 */
int smartlog_write_log_entry(
    const char* file_path,
    const char* msg,
    feature_state_t durable,
    feature_state_t max_bytes_config,
    unsigned long max_byte_val
);

#endif /* SMARTLOG_CORE_H */
