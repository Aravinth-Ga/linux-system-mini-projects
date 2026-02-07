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

#define _POSIX_C_SOURCE 199309L

#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>

#define MESSAGE_LEN_MAX   200


uint64_t time_stamp(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

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

static int write_usage(const char* error_msg)
{
    return write_all(STDERR_FILENO, error_msg, strlen(error_msg));
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
            return write_usage("Usage : ./mini_log <file_path> \"<message>\" [--durable]\n");
        }  
    }
    else if(argc != 3)
    {
        return write_usage("Usage : ./mini_log <file_path> \"<message>\" [--durable]\n");
    }

    const char* file_path = argv[1];
    const char* msg = argv[2];
    size_t msg_len = strlen(msg);

    if(msg[0] == '\0')
        return write_usage("Log message is Empty.\n");
   
    // 2. Permission check
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
            printf("The file %s exist.\n", file_path);
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

    // filemode
    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP;

    // open flag 
    // O_APPEND is added to update the file EOF automatically.
    int flag = O_WRONLY | O_CREAT | O_APPEND;

    int fd = open(file_path, flag, mode);

    if(fd < 0)
    {
        perror("open");
        return 1;
    }

    struct stat fstat_new;
    int file2_exist = stat(file_path, &fstat_new);
    int stat2_errno = errno;
    
    
    if(file1_exist !=0 && stat1_errno == ENOENT)
    {
        if(file2_exist == 0)
        {
            printf("[LOG FILE PERMISSION CHECK]\n");            
            printf("The file %s does not exist and it is created now.\n",file_path);
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
    // 4. Write the data into the log file

    char buff[MESSAGE_LEN_MAX + 1];
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
    
    int log_len = snprintf(log_buffer, sizeof(log_buffer), "[%llu ns] [PID = %ld] [MESSAGE = %s]\n", (unsigned long long)time_ns, (long)pid, msg);

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
