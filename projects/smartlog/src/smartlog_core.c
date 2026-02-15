/*
 * src/smartlog_core.c
 *
 * Core logging implementation for SmartLog.
 *
 * Handles the actual work of logging:
 *   - Check if file exists and has right permissions
 *   - Format log message with timestamp and process ID
 *   - Rotate log file if it gets too big
 *   - Write message to file safely
 *   - Sync to disk if durable mode is on
 *
 * This file has the main logic. It is separate from the CLI tool
 * so it can be used in libraries and other programs.
 *
 * Author: Aravinthraj Ganesan
 * Version: 4.0
 */

#define _GNU_SOURCE
#define _POSIX_C_SOURCE 199309L

/* ============================================================================
 * Standard Includes
 * ============================================================================ */

#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>

/* ============================================================================
 * Project Includes
 * ============================================================================ */

#include <smartlog/utils.h>
#include <smartlog/config.h>
#include <smartlog/smartlog_core.h>

/* ============================================================================
 * Internal Helpers
 * ============================================================================ */

static int smartlog_rotate_if_needed(
    const char* file_path,
    feature_state_t durable,
    feature_state_t max_bytes_config,
    unsigned long max_byte_val,
    int log_len,
    const struct stat* fstat_old,
    int* file1_exist,
    int* stat1_errno
)
{
    if(max_bytes_config != FEATURE_ENABLED || *file1_exist != 0)
    {
        return 0;
    }

    /* Calculate potential new file size if this log entry is added */
    unsigned long long cur_file_size = (unsigned long long)fstat_old->st_size;
    unsigned long long new_file_size = (unsigned long long)cur_file_size + (unsigned long long)log_len;

    /* If adding this entry exceeds max_byte_val, perform rotation */
    if(max_byte_val >= new_file_size)
    {
        return 0;
    }

    /* Build backup filename by appending ".1" */
    char new_path[SMARTLOG_PATH_MAX_LEN];
    int n = snprintf(new_path, sizeof(new_path), "%s.1", file_path);

    if(n < 0 || n >= (int)sizeof(new_path))
    {
        errno = ENAMETOOLONG;
        return 1;
    }

    /*
     * Remove existing .1 backup file if it exists (we'll replace it).
     * Ignore ENOENT error (file doesn't exist).
     */
    if(unlink(new_path) < 0)
    {
        if(errno != ENOENT)
        {
            return 1;
        }
    }

    /* Rename current log file to .1 backup */
    if(rename(file_path, new_path) < 0)
    {
        return 1;
    }

    /*
     * In durable mode: sync parent directory to ensure metadata changes
     * (the rename operation) persist to disk before continuing.
     */
    if(durable == FEATURE_ENABLED)
    {
        if(smartlog_fsync_parent_dir(file_path) != 0)
        {
            return 1;
        }
    }

    /*
     * Update file existence check - the original file no longer exists
     * after rename, so the open() below will create a new file.
     */
    *stat1_errno = ENOENT;
    *file1_exist = -1;

    return 0;
}

/* ============================================================================
 * Core Logging Implementation
 * ============================================================================ */

int smartlog_write_log_entry(
    const char* file_path,
    const char* msg,
    feature_state_t durable,
    feature_state_t max_bytes_config,
    unsigned long max_byte_val
)
{
    if(file_path == NULL || msg == NULL)
    {
        errno = EINVAL;
        return 1;
    }

    /* ====================================================================
     * STEP 1: Validate Log Message and Check File Existence
     * ==================================================================== */
    /* Extract and validate the log message */
    size_t msg_len = strlen(msg);

    /* Message must not be empty - check for empty string */
    if(msg[0] == '\0')
    {
        errno = EINVAL;
        return 1;
    }

    /* 
     * Check whether the log file path already exists using stat().
     * This determines whether we're appending or creating new.
     */
    struct stat fstat_old;
    errno = 0;
    int file1_exist = stat(file_path, &fstat_old);
    int stat1_errno = errno;

    /* If the file exists, verify it's not a directory */
    if(file1_exist == 0) 
    {
        if(S_ISDIR(fstat_old.st_mode) != 0)
        {
            errno = EISDIR;
            return 1;
        }            
    }
    else 
    {
        /* If stat() failed, verify the error is just ENOENT (file doesn't exist) */
        if(stat1_errno != ENOENT)
        {   
            return 1;
        }
        /* File doesn't exist - will be created by open() with O_CREAT flag */
    }

    /* ====================================================================
     * STEP 2: Build Formatted Log Entry
     * ==================================================================== */
    /* 
     * Format the log message with timestamp and PID.
     * Uses buffer on the stack with SMARTLOG_LOG_BUFFER_SZ capacity.
     */
    char log_buffer[SMARTLOG_LOG_BUFFER_SZ];
    int log_len = 0; 
    char buff[SMARTLOG_MSG_MAX_LEN + 1];

    /* 
     * Enforce message length limit - truncate if necessary and append "..."
     * to indicate truncation.
     */
    if(msg_len > SMARTLOG_MSG_MAX_LEN)
    {
        memcpy(buff, msg, (SMARTLOG_MSG_MAX_LEN - 3));
        memcpy(buff + (SMARTLOG_MSG_MAX_LEN - 3), "...", 3);
        buff[SMARTLOG_MSG_MAX_LEN] = '\0';
        msg = buff;
    }

    /* Get current timestamp in nanoseconds */
    uint64_t time_ns = smartlog_timestamp_ns();
    pid_t pid = getpid();   
    
    /* Format the complete log entry: [timestamp] [PID] [message] */
    log_len = snprintf(
        log_buffer,
        sizeof(log_buffer),
        "[%llu ns] [PID = %ld] [MESSAGE = %s]\n",
        (unsigned long long)time_ns,
        (long)pid,
        msg
    );

    /* Verify snprintf didn't fail or truncate output buffer */
    if(log_len < 0 || (log_len >= (int)sizeof(log_buffer)))
    {
        errno = EOVERFLOW;
        return 1;
    }   

    /* ====================================================================
     * STEP 3: Handle Log Rotation if Enabled
     * ==================================================================== */
    /* 
     * If max-bytes feature is enabled and the log file exists,
     * check whether adding this log entry would exceed the limit.
     * If so, rotate the log file by renaming current to .1 backup.
     */
    if(smartlog_rotate_if_needed(
        file_path,
        durable,
        max_bytes_config,
        max_byte_val,
        log_len,
        &fstat_old,
        &file1_exist,
        &stat1_errno
    ) != 0)
    {
        return 1;
    }

    /* ====================================================================
     * STEP 4: Open or Create Log File
     * ==================================================================== */
    /* 
     * Open the log file for writing. File will be created if it doesn't exist
     * with permissions based on SMARTLOG_FILE_MODE (subject to umask).
     * O_APPEND flag ensures writes go to end of file even with concurrent access.
     */
    mode_t mode = SMARTLOG_FILE_MODE;

    /* 
     * Open flags:
     * - O_WRONLY: open for writing only
     * - O_CREAT: create file if it doesn't exist
     * - O_APPEND: all writes happen at end of file
     */
    int flag = O_WRONLY | O_CREAT | O_APPEND;

    /* Attempt to open the file with the specified flags and permissions */
    int fd = open(file_path, flag, mode);

    /* Verify open() succeeded */
    if(fd < 0)
    {
        return 1;
    }

    /* ====================================================================
     * STEP 5: Verify File Permissions After Open/Create
     * ==================================================================== */
    /* 
     * Get file descriptor stats to verify permissions.
     * If file was newly created, confirm it has expected permissions.
     */
    /* If original file didn't exist and was just created, verify fstat works */
    if(file1_exist != 0 && stat1_errno == ENOENT)
    {
        struct stat fstat_new;
        if(fstat(fd, &fstat_new) != 0)
        {
            int saved_errno = errno;
            close(fd);
            errno = saved_errno;
            return 1;
        }
    }

    /* ====================================================================
     * STEP 6: Write Log Entry to File
     * ==================================================================== */
    /* 
     * Write the formatted log entry to the file descriptor.
     * Uses smartlog_write_all() to ensure complete write even if interrupted.
     */
    if(smartlog_write_all(fd, log_buffer, (size_t)log_len) != 0)
    {
        int saved_errno = errno;
        close(fd);
        errno = saved_errno;
        return 1;
    }

    /* ====================================================================
     * STEP 7: Optionally Sync to Disk (Durable Mode)
     * ==================================================================== */
    /* 
     * In durable mode: call fdatasync() to ensure this log entry
     * is durably written to storage before returning to caller.
     * This provides stronger guarantees at the cost of performance.
     */
    if(durable == FEATURE_ENABLED)
    {
        /* Sync file data to disk */
        if(fdatasync(fd) != 0)
        {
            int saved_errno = errno;
            close(fd);
            errno = saved_errno;
            return 1;
        }

        /* Sync parent directory metadata to ensure rename/create is persistent */
        if(smartlog_fsync_parent_dir(file_path) != 0)
        {
            int saved_errno = errno;
            close(fd);
            errno = saved_errno;
            return 1;
        }
    }

    /* ====================================================================
     * STEP 8: Close File and Return Success
     * ==================================================================== */
    /* Close file descriptor - all operations complete */
    if(close(fd) < 0)
    {
        return 1;
    }

    /* All operations succeeded - return success status */
    return 0;
}
