/*
 * src/mini_log.c
 *
 * SmartLog command-line interface (CLI wrapper).
 *
 * This is the entry point for the SmartLog CLI tool. It handles:
 *   - Command-line argument parsing and validation
 *   - Signal handling for graceful shutdown
 *   - Delegation to smartlog_core for the actual logging
 *
 * The core logging logic is in smartlog_core.c to allow reuse
 * in libraries, daemons, or other applications.
 *
 * Command-line usage:
 *   mini_log <file_path> "<message>" [--durable] [--max-bytes <size>]
 *
 * Options:
 *   --durable: Enable fdatasync() after write for crash-safety
 *   --max-bytes <size>: Enable automatic rotation at specified byte size
 *
 * Author: Aravinthraj Ganesan
 * Version: 4.0
 */

#define _GNU_SOURCE
#define _POSIX_C_SOURCE 199309L

/* ============================================================================
 * Standard Includes
 * ============================================================================ */

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>

/* ============================================================================
 * Project Includes
 * ============================================================================ */

#include <smartlog/utils.h>
#include <smartlog/config.h>
#include <smartlog/smartlog_core.h>

/* ============================================================================
 * Global Variables
 * ============================================================================ */

/** Signal handler flag - set to 1 when SIGINT is caught */
static volatile sig_atomic_t stop = 0;

/* ============================================================================
 * Helper Functions
 * ============================================================================ */

/**
 * Print error message to stderr.
 * Uses smartlog_write_all for reliable output even during shutdown.
 */
static int write_usage(const char* error_msg)
{
    (void)smartlog_write_all(STDERR_FILENO, error_msg, strlen(error_msg));
    return 2;
}

/**
 * Signal handler for graceful shutdown.
 * Sets global flag when SIGINT is received.
 */
static void signal_handle(int sig)
{
    (void) sig;  /* Unused parameter */
    stop = 1;
}

/* ============================================================================
 * Main Entry Point
 * ============================================================================ */

/**
 * SmartLog CLI main function.
 *
 * Workflow:
 *   Step 1: Register signal handler for graceful shutdown
 *   Step 2: Validate command-line argument count
 *   Step 3: Parse and validate command-line options
 *   Step 4: Call smartlog_write_log_entry() with parsed parameters
 *   Step 5: Return exit status
 *
 * Return: 0 on success, non-zero on error
 */
int main(int argc, char* argv[])
{
    /* ====================================================================
     * STEP 1: Register Signal Handler
     * ==================================================================== */
    /* Set up graceful shutdown handler for SIGINT (Ctrl+C) */
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handle;
    sa.sa_flags = SA_RESTART;
    if(sigemptyset(&sa.sa_mask) != 0)
    {
        perror("sigemptyset");
        return 1;
    }

    if(sigaction(SIGINT, &sa, NULL) != 0)
    {
        perror("sigaction");
        return 1;
    }

    /* ====================================================================
     * STEP 2: Validate Command-Line Argument Count
     * ==================================================================== */
    /* Require at least 3 arguments: program, file path, message */
    if(argc < 3 || argc > 6)
    {
        return write_usage("Usage: ./mini_log <file_path> \"<message>\" [--durable] [--max-bytes <size>]\n");
    }

    /* ====================================================================
     * STEP 3: Parse Command-Line Options
     * ==================================================================== */
    /* Variables for parsed feature flags */
    feature_state_t durable = FEATURE_DISABLED;
    unsigned long max_byte_val = 0;    
    feature_state_t max_bytes_config = FEATURE_DISABLED;

    /* Parse optional arguments starting from index 3 */
    for(int arg_idx = 3; arg_idx < argc; arg_idx++)
    {
        if(strcmp(argv[arg_idx], "--durable") == 0) 
        {
            durable = FEATURE_ENABLED;
        }
        else if(strcmp(argv[arg_idx], "--max-bytes") == 0)
        {
            /* Ensure a value is provided after --max-bytes */
            if(argc <= (arg_idx + 1))
                return write_usage("Error: --max-bytes requires a value.\n");

            arg_idx += 1;
            char* endpoint = NULL;
            errno = 0;
            /* Parse the max-bytes value as unsigned long integer */
            unsigned long temp = strtoul(argv[arg_idx], &endpoint, 10);

            if(endpoint == argv[arg_idx] || *endpoint != '\0' || errno == ERANGE || temp == 0)
                return write_usage("Error: --max-bytes requires a positive integer\n");

            max_byte_val = temp;
            max_bytes_config = FEATURE_ENABLED;
        }
        else
        {
            /* Unknown option provided */
            return write_usage("Error: Unknown option.\nUsage: ./mini_log <file_path> \"<message>\" [--durable] [--max-bytes <size>]\n");
        }
    }

    /* ====================================================================
     * STEP 4: Call Core Logging Function
     * ==================================================================== */
    /* 
     * Delegate the actual logging work to smartlog_core.
     * This separates CLI concerns from business logic.
     */
    int result = smartlog_write_log_entry(
        argv[1],                    /* file_path */
        argv[2],                    /* msg */
        durable,                    /* durable flag */
        max_bytes_config,           /* rotation feature flag */
        max_byte_val                /* rotation size limit */
    );
    if(result != 0)
    {
        if(stop == 1 && errno == EINTR)
        {
            const char* error_msg = "Interrupted by SIGINT.\n";
            (void)smartlog_write_all(STDERR_FILENO, error_msg, strlen(error_msg));
            return 130;
        }
        perror("smartlog_write_log_entry");
    }

    /* Return the result from core logging operation */
    return result;
}
