/*
    * mini_log.c
    *
    * Simple log writer for the SmartLog project.
    * It appends one line to a file and can optionally flush to disk.
    *
    * System calls used:
    *    - open(): create or open the log file
    *    - write(): write the log entry
    *    - close(): close the file when done
    *
    * Flags for open():
    *    - O_WRONLY: open the file for writing only
    *    - O_CREAT: create the file if it is missing
    *    - O_APPEND: always write at the end of the file
    *
    * Note: This is a small demo. A full logger would add log levels,
    * rotation, and stronger error handling.
    *
    * Author: Aravinthraj Ganesan
    * Date: 2024-06-01
    * Version: 2.0
*/

#define _POSIX_C_SOURCE 199309L

#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

#define MESSAGE_LEN_MAX   200


/*
    * Return a current timestamp in nanoseconds.
    * Used to tag each log line.
*/
uint64_t time_stamp(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    return ((uint64_t) (ts.tv_sec * 1000000000ull) + (uint64_t)ts.tv_nsec);
}


/*
    * Write the full buffer to the file descriptor.
    * Returns 0 on success, 1 on error.
*/
static int write_all(int file_descriptor, const void* msg_buff, size_t msg_length)
{
    const char* message = (const char*) msg_buff;
    size_t total = 0;
    ssize_t bytes_written = 0;

    while(total < msg_length)
    {
        bytes_written = write(file_descriptor, message + total, msg_length - total);

        if(bytes_written < 0)
        {
            if(errno == EINTR)
                continue;
            else
                return 1;

        }
        else if(bytes_written == 0)
        {
            errno = EIO;
            return 1;
        }
        else
        {
            total += (size_t)bytes_written;
        }
    }

    return 0;
}

/*
    * Print a short error or usage message to stderr.
*/
static int write_usage(const char* error_msg)
{
    return write_all(STDERR_FILENO, error_msg, strlen(error_msg));
}

/*
    * Entry point for the mini log writer.
    * Steps:
    * 1) parse arguments
    * 2) check existing file (if any)
    * 3) open/create the log file
    * 4) format and write one log line
    * 5) optionally flush to disk
*/
int main(int argc, char* argv[])
{
    uint8_t durable = 0;
    
    // Command line argument count validation
    if(argc < 3 || argc > 6)
    {
        return write_usage("Usage :./mini_log <file_path> \"<message>\" [--durable] [--max-bytes] [size]\n"); 
    }

    // 1. Read command-line arguments
    for(uint8_t arg_idx = 3; arg_idx < argc; arg_idx++)
    {
        if(strcmp(argv[arg_idx],"--durable") == 0) 
        {
            durable = 1;
        }
        else if(strcmp(argv[arg_idx], "--max-bytes") == 0)
        {
            // Check if the max-bytes values are provided
            if(argc <= (arg_idx + 1))
            {
                return write_usage("Usage : max-bytes values are missig.\n"); 
            }

            unsigned long max_byte_val;

            arg_idx += 1;
            // convert the max-bytes value from string to unsigned long
            max_byte_val = strtoul(argv[arg_idx],NULL, 10);

            // Check if the max byte values are valid.
            if(max_byte_val == 0)
                return write_usage("Usage : max-bytes values must not be 0.\n"); 

        }
        else
        {
            // Return when the command line arguments has unknown values.
            return write_usage("Error! Unknown option.\nUsage :./mini_log <file_path> \"<message>\" [--durable] [--max-bytes] [size]\n"); 
        }

    }

    const char* file_path = argv[1];
    const char* msg = argv[2];
    size_t msg_len = strlen(msg);

    // Message must not be empty to log
    if(msg[0] == '\0')
        return write_usage("Log message is Empty.\n");

    printf("Path of the log file is: %s\n", file_path);
   
    // 2. Check whether the file exists and is not a directory
    struct stat fstat_old;
    int file1_exist = stat(file_path, &fstat_old); 
    int stat1_errno = errno;

    if(file1_exist == 0) 
    {
        if(S_ISDIR(fstat_old.st_mode) != 0)
        {
            fprintf(stderr, "Error : %s is a directory.\n",file_path);
            return 1;
        }
        else
        {
            printf("[LOG FILE PERMISSION CHECK]\n");            
            printf("The file %s exists.\n", file_path);
            printf("Owner UID : %u\n",fstat_old.st_uid);
            printf("Group ID : %u\n", fstat_old.st_gid);
            printf("File Permission : %04o\n", fstat_old.st_mode & 0777);        
        }             
    }
    else 
    {
        if(stat1_errno != ENOENT)
        {   
            perror("stat");
            return 1;
        }

    }

    // 3. Open the file
    // File mode (before umask). The final permissions can be masked by umask.
    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP;

    // Open flags
    // O_APPEND keeps writes at the end of the file.
    int flag = O_WRONLY | O_CREAT | O_APPEND;

    int fd = open(file_path, flag, mode);

    if(fd < 0)
    {
        perror("open");
        return 1;
    }

    // Re-check permissions if the file was created by open().
    struct stat fstat_new;
    int file2_exist = stat(file_path, &fstat_new);
    
    if(file1_exist !=0 && stat1_errno == ENOENT)
    {
        if(file2_exist == 0)
        {
            printf("[LOG FILE PERMISSION CHECK]\n");            
            printf("The file %s does not exist and is created now.\n",file_path);
            printf("Owner UID : %u\n",fstat_new.st_uid);
            printf("Group ID : %u\n", fstat_new.st_gid);
            printf("File Permission : %04o\n", fstat_new.st_mode & 0777);        
        }
        else
        {
            perror("stat");
            close(fd);
            return 1;
        }
    }
    // 4. Build and write the log entry

    char buff[MESSAGE_LEN_MAX + 1];
    // Keep the log message size bounded.
    if(msg_len > MESSAGE_LEN_MAX)
    {
        memcpy(buff, msg, (MESSAGE_LEN_MAX - 3));
        memcpy(buff + (MESSAGE_LEN_MAX - 3), "...", 3);
        buff[MESSAGE_LEN_MAX] = '\0';
        msg = buff;
    }

    uint64_t time_ns = time_stamp();
    pid_t pid = getpid();   
    char log_buffer[1024];
    
    int log_len = snprintf(
        log_buffer,
        sizeof(log_buffer),
        "[%llu ns] [PID = %ld] [MESSAGE = %s]\n",
        (unsigned long long)time_ns,
        (long)pid,
        msg
    );

    if(log_len < 0 || (log_len >= (int)sizeof(log_buffer)))
    {
        perror("snprintf");
        close(fd);
        return 1;
    }   

    if(write_all(fd, log_buffer, (size_t)log_len) != 0)
    {
        perror("write");
        close(fd);
        return 1;
    }

    // 5. Durability: flush data to disk when --durable is used

    if(durable != 0)
    {
        /*
            fdatasync() flushes written data to disk. This makes the write
            durable across crashes or power loss. It is usually faster than
            fsync() because it does not have to flush all metadata.
        */
       if(fdatasync(fd) < 0)
       {
            perror("fdatasync");
            close(fd);
            return 1;
       }

    }

    // 6. Close the file
    if(close(fd)<0)
    {
        perror("close");
        return 1;
    }

    return 0;
}
