/**
 * supervisor.c - Implementation of the process supervisor
 * 
 * This module handles the initialization and shutdown of the supervisor component,
 * which is responsible for managing and monitoring child processes.
 */

#include "sentinel.h"
#include <stdio.h>




/**
 * supervisor_init - Initialize the supervisor to a running state
 * @supervisor: Pointer to supervisor_t structure to initialize
 *
 * Sets the supervisor's shutdown flag to 0 
 */
void supervisor_init(supervisor_t* supervisor)
{
    supervisor->is_shutting_down = 0;
    fprintf(stderr, "[supervisor] init\n");
}


/**
 * supervisor_shutdown - Handle shutdown of the supervisor
 * @supervisor: Pointer to supervisor_t structure
 *
 */
void supervisor_shutdown(supervisor_t* supervisor)
{
    // Suppress unused parameter warning for potential future use
    (void)supervisor;

    fprintf(stderr, "[supervisor] shutdown\n");
}