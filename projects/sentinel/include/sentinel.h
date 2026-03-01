#pragma once

#include <sys/types.h>

/**
 * supervisor_t - Structure to manage process supervision and lifecycle
 * @is_shutting_down: Flag indicating whether the supervisor is in shutdown state (1) or running (0)
 */
typedef struct
{
    int is_shutting_down;
} supervisor_t;

/**
 * supervisor_init - Initialize the supervisor with default state
 * @supervisor: Pointer to supervisor_t structure to initialize
 */
void supervisor_init(supervisor_t* supervisor);

/**
 * supervisor_shutdown - Initiate graceful shutdown of the supervisor
 * @supervisor: Pointer to supervisor_t structure to shutdown
 */
void supervisor_shutdown(supervisor_t* supervisor);


