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
    * Version: 3.0
*/

#define _GNU_SOURCE
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
#include <signal.h>

#define MESSAGE_LEN_MAX         200
#define ENABLE                  1
#define DISABLE                 0
#define CURRENT_WORKING_DIR     0xAA
#define USER_INPUT_DIR          0xBB

volatile sig_atomic_t stop = 0;

/*
    * Return the current time in nanoseconds.
    * Used to timestamp each log line.
*/
uint64_t time_stamp(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    return ((uint64_t) (ts.tv_sec * 1000000000ull) + (uint64_t)ts.tv_nsec);
}


/*
    * Write the full buffer to the file descriptor.
    * Retries on EINTR.
    * Returns 0 on success, 1 on error.
*/
static int write_all(int file_descriptor, const void* msg_buff, size_t msg_length)
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

void signal_handle(int sig)
{
    (void) sig;
    stop = 1;
}

/*
    * Entry point for the mini log writer.
    * Steps:
    * 1) parse arguments
    * 2) validate inputs and existing file (if any)
    * 3) open/create the log file
    * 4) format and write one log line
    * 5) optionally flush to disk
*/
int main(int argc, char* argv[])
{
    uint8_t durable = DISABLE;
    unsigned long max_byte_val = 0;    
    uint8_t max_bytes_config = DISABLE;
    struct stat fstat_new;
    
    signal(SIGINT, signal_handle);

    // Validate argument count.
    if(argc < 3 || argc > 6)
    {
        return write_usage("Usage :./mini_log <file_path> \"<message>\" [--durable] [--max-bytes] [size]\n"); 
    }

    // 1. Read command-line arguments.
    for(int arg_idx = 3; arg_idx < argc; arg_idx++)
    {
        if(strcmp(argv[arg_idx],"--durable") == 0) 
        {
            durable = ENABLE;
        }
        else if(strcmp(argv[arg_idx], "--max-bytes") == 0)
        {
            // Ensure a value is provided after --max-bytes.
            if(argc <= (arg_idx + 1))
                return write_usage("Usage : max-bytes values are missig.\n"); 

            arg_idx += 1;
            char* endpoint;
            errno = 0;
            // Parse the max-bytes value.
            unsigned long temp = strtoul(argv[arg_idx], &endpoint, 10);

            if(endpoint == argv[arg_idx] || *endpoint != '\0' || errno == ERANGE || temp == 0)
                return write_usage("Error: --max-bytes requires a positive integer\n");

            max_byte_val = temp;
            max_bytes_config = ENABLE;

        }
        else
        {
            // Unknown option.
            return write_usage("Error: Unknown option.\nUsage :./mini_log <file_path> \"<message>\" [--durable] [--max-bytes] [size]\n"); 
        }

    }

    // 2. Validate the command line arguments.

    const char* file_path = argv[1];
    const char* msg = argv[2];
    size_t msg_len = strlen(msg);

    // Message must not be empty.
    if(msg[0] == '\0')
        return write_usage("Log message is Empty.\n");

  
    // 3. Check whether the log path already exists.
    struct stat fstat_old;
    int file1_exist = stat(file_path, &fstat_old); 
    int stat1_errno = errno;

    if(file1_exist == 0) 
    {
        if(S_ISDIR(fstat_old.st_mode) != 0)
        {
            fprintf(stderr, "Error : %s is a directory. Provide a file name.\n",file_path);
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

    // 4. Build the log entry before open.
    char log_buffer[1024];
    int log_len = 0; 
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
    
    log_len = snprintf(
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
        return 1;
    }   

    // 5. log rotation if enabled
    if(max_bytes_config == ENABLE && file1_exist == 0)
    {
        unsigned long long cur_file_size = (unsigned long long)fstat_old.st_size;
        unsigned long long new_file_size = (unsigned long long)cur_file_size + log_len;

        if(max_byte_val < new_file_size)
        {
            char new_path[4096];
            int n = snprintf(new_path, sizeof(new_path), "%s.1", file_path);

            if(n < 0 || n >= (int)sizeof(new_path))
            {
                perror("snprintf");
                return 1;
            }

            if(unlink(new_path) < 0)
            {
                if(errno != ENOENT)
                {
                    perror("unlink");
                    return 1;
                }
            }
            if(rename(file_path, new_path) < 0)
            {
                perror("rename");
                return 1;
            }

            // sync the local directory
            int fd_dir = open(".", O_RDONLY | O_DIRECTORY);

            if(fd_dir < 0)
            {
                perror("open");
                return 1;
            }

            if(fsync(fd_dir)< 0)
            {
                perror("fsync");
                close(fd_dir);
                return 1;
            }

            if(close(fd_dir) < 0)
            {
                perror("close");
                return 1;
            }

            // Ensure the flags are set once the file is renamed.
            stat1_errno = ENOENT;
            file1_exist = -1;
        }

    }

    // 6. Open the file (create it if it does not exist).
    // File mode (before umask). The final permissions can be masked by umask.
    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP;

    // Open flags. O_APPEND keeps writes at the end of the file.
    int flag = O_WRONLY | O_CREAT | O_APPEND;

    int fd = open(file_path, flag, mode);

    if(fd < 0)
    {
        perror("open");
        return 1;
    }

    // 7. Re-check permissions if the file was created by open().
    int file2_exist = fstat(fd, &fstat_new);
    
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

    if(stop == 1)
    {
        const char* error_msg = "Caught SIGINT, exiting cleanly.\n";
        write_all(STDERR_FILENO, error_msg, strlen(error_msg));
        close(fd);
        return 1;
    }

    // 8. Write the log entry.
    if(write_all(fd, log_buffer, (size_t)log_len) != 0)
    {
        perror("write");
        close(fd);
        return 1;
    }

    
    // 9. Durability: flush data to disk when --durable is used.
    if(durable != 0)
    {
        printf("durability is enabled = %d\n", durable);
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

    // 10. Close the file.
    if(close(fd)<0)
    {
        perror("close");
        return 1;
    }

    return 0;
}
