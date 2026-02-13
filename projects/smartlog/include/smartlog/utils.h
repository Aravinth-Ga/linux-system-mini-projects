/*
 * include/smartlog/utils.h
 *
 * Helper functions for SmartLog.
 *
 * Provides:
 *   - Get current time in nanoseconds
 *   - Write data to file (handles interrupts)
 *   - Sync directory changes to disk
 *
 * Author: Aravinthraj Ganesan
 * Version: 4.0
 */

#ifndef SMARTLOG_UTILS_H
#define SMARTLOG_UTILS_H

#include <stddef.h>
#include <stdint.h>

/* ============================================================================
 * Helper Functions
 * ============================================================================ */

/**
 * Get current time in nanoseconds.
 *
 * Uses CLOCK_REALTIME system clock.
 *
 * Return: Time value in nanoseconds
 */
uint64_t smartlog_timestamp_ns(void);

/**
 * Write data to file descriptor, handle interrupts.
 *
 * Keeps writing until all bytes are sent, even if signal interrupts it.
 *
 * Parameters:
 *   fd     - File descriptor to write to
 *   data   - Data to write
 *   size   - Number of bytes to write
 *
 * Return: 0 on success, -1 on error
 */
int smartlog_write_all(int fd, const void* data, size_t size);
/**
 * Sync directory changes to disk.
 *
 * Makes sure file creation and rename operations are saved to disk.
 *
 * Parameters:
 *   path - Path to a file (uses its parent directory)
 *
 * Return: 0 on success, 1 on error
 */
int smartlog_fsync_parent_dir(const char* path);

#endif /* SMARTLOG_UTILS_H */
