/*
    * mini_log.c
    *
    * Description: A simple logging utility for the SmartLog project.
    * 
    * System calls used:
    *    - open(): To create or open the log file.
    *    - write(): To write log entries to the file.
    *    - close(): To close the log file after writing.
    *
    * Flags for open():
    *    - O_WRONLY: Open the file for write-only access.
    *    - O_CREAT: Create the file if it does not exist.
    *    - O_APPEND: Append to the end of the file if it already exists.
    *
    * Note: This is a simplified implementation for demonstration purposes. In a production environment, consider adding error handling, log rotation, and support for different log levels (e.g., INFO, WARNING, ERROR).
    * 
    * Author: Aravinthraj Ganesan
    * Date: 2024-06-01
    * Version: 1.0

*/


#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>

#define ERROR_MSG   0
#define LOG_MSG     1

uint64_t time_stamp(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    return ((uint64_t) (ts.tv_sec * 1000000000ull) + (uint64_t)ts.tv_nsec);
}


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

static int write_usage(void)
{
    char* info = "Usage : ./mini_log <file_path> \"<message>\" [--durable]\n";

    return write_all(STDERR_FILENO, info, strlen(info));
}

int main(int argc, char* argv[])
{
    uint8_t durable = 0;
    
    // 1. Get the command line argument
    if(argc == 4)
    {
        if(strcmp(argv[3], "--durable") == 0)
        {
            durable = 1;
        }
        else
        {
            return write_usage();
        }
    }
    else if(argc != 3)
    {
        return write_usage();
    }

    const char* file_path = argv[1];
    const char* msg = argv[2];
   

    // 2. Open the file

    // filemode
    mode_t mode = S_IRUSR | S_IWUSR | S_IROTH;

    // open flag
    int flag = O_WRONLY | O_CREAT | O_APPEND;

    int fd = open(file_path, flag, mode);

    if(fd < 0)
    {
        perror("open");
        return 1;
    }

    // 3. Permission check
    

    // 4. Write the data into the log file

    uint64_t time_ns = time_stamp();
    uint8_t pid = getgid();   
    char log_buffer[1024];
    
    int log_len = snprintf(log_buffer, sizeof(log_buffer), "[%lu ns] [pid = %d] [%s]\n", time_ns, pid, msg);

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

    // 5. durability

    if(durable != 0)
    {
        /*
            fdatasync() is used to flush all modified data of the fd to the disk.This ensures that all written data is physically stored on the disk, providing durability in case of power failures or system crashes.
            This provides better performance compared to fsync() when only data durability is required without the need to flush metadata.
        */
       if(fdatasync(fd) < 0)
       {
            perror("fdatasync");
            close(fd);
            return 1;
       }

    }

    // 6. Close
    if(close(fd)<0)
    {
        perror("close");
        return 1;
    }

    return 0;
}
