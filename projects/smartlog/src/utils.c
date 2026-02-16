/*
 * src/utils.c
 *
 * Helper functions implementation.
 *
 * Implements:
 *   - Get current time in nanoseconds
 *   - Write data to file (handle interrupts)
 *   - Sync directory to disk
 *
 * Author: Aravinthraj Ganesan
 * Version: 4.0
 */

#define _GNU_SOURCE
#define _POSIX_C_SOURCE 199309L

/* Standard includes */
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* Project includes */
#include <smartlog/utils.h>
#include <smartlog/config.h>

/* ============================================================================
 * Timestamp Function
 * ============================================================================ */

/**
 * Get current time in nanoseconds.
 */
uint64_t smartlog_timestamp_ns(void)
{
#ifdef SMARTLOG_TEST_FAULTS
    const char* force_fail = getenv("SMARTLOG_FAKE_CLOCK_FAIL");
    if(force_fail != NULL && strcmp(force_fail, "1") == 0)
    {
        errno = EIO;
        return 0;
    }
#endif

    struct timespec ts;
    if(clock_gettime(CLOCK_REALTIME, &ts) != 0)
    {
        return 0;
    }

    return ((uint64_t)ts.tv_sec * UINT64_C(1000000000)) + (uint64_t)ts.tv_nsec;
}

/* ============================================================================
 * Write Function
 * ============================================================================ */

/**
 * Write all bytes to file, retry if interrupted.
 */
int smartlog_write_all(int file_descriptor, const void* msg_buff, size_t msg_length)
{
    const char* message = (const char*) msg_buff;
    size_t total = 0;
    ssize_t bytes_written = 0;
    errno = 0;

    while(total < msg_length)
    {
        bytes_written = write(file_descriptor, message + total, msg_length - total);

        if(bytes_written < 0)
        {
            /* Write failed */
            if(errno == EINTR)
            {
                /* Interrupted by signal - retry */
                continue;
            }
            else
            {
                /* Real error */
                return -1;
            }
        }
        else if(bytes_written == 0)
        {
            /* Zero bytes - unexpected */
            errno = EIO;
            return -1;
        }
        else
        {
            /* Update progress and continue */
            total += (size_t)bytes_written;
        }
    }

    return 0;
}

/* ============================================================================
 * Directory Sync Function
 * ============================================================================ */

/**
 * Sync parent directory to disk.
 */
int smartlog_fsync_parent_dir(const char* path)
{
    /* Validate input */
    if(!path || path[0] == '\0')
    {
        errno = EINVAL;
        return -1;
    }

    /* Check path length */
    char temp[SMARTLOG_PATH_MAX_LEN];
    size_t len = strnlen(path, sizeof(temp));
    if(sizeof(temp) <= len)
    {
        errno = ENAMETOOLONG;
        return -1;
    }

    /* Copy path for manipulation */
    memcpy(temp, path, len + 1);

    /* Find parent directory */
    char* slash = strrchr(temp, '/');
    const char* dir_path = NULL;
    
    if(!slash)
    {
        /* No slash - current directory */
        dir_path = ".";
    }
    else if(slash == temp)
    {
        /* Slash is first - root directory */
        temp[1] = '\0';
        dir_path = temp;
    }
    else
    {
        /* Normal case - truncate at slash */
        *slash = '\0';
        if(temp[0] == '\0')
        {
            dir_path = ".";
        }
        else
        {
            dir_path = temp;
        }
    }

    /* Open directory */
    int fd_dir = open(dir_path, O_RDONLY | O_DIRECTORY);
    if(fd_dir < 0)
    {
        return -1;
    }

    /* Sync to disk */
    if(fsync(fd_dir) < 0)
    {
        int errno_pre = errno;
        close(fd_dir);
        errno = errno_pre;
        return -1;
    }

    /* Close */
    if(close(fd_dir) < 0)
    {
        return -1;
    }

    return 0;
}
