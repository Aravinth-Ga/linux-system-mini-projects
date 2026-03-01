// Include the sentinel header file for supervisor
#include "sentinel.h"
#include <stdio.h>

// Main program entry point
int main()
{
    // Create a supervisor variable
    supervisor_t supervisor;

    // Start the supervisor
    supervisor_init(&supervisor);

    // Print a message that the program is running
    fprintf(stderr,"[main] running. skeleton changes only!!!");

    // Stop the supervisor
    supervisor_shutdown(&supervisor);

    // Return success
    return 0;
}