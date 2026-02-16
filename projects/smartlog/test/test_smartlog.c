#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include <smartlog/config.h>
#include <smartlog/smartlog_core.h>

static int read_file(const char* path, char* out, size_t out_sz)
{
    int fd = open(path, O_RDONLY);
    if(fd < 0)
    {
        return -1;
    }

    ssize_t n = read(fd, out, out_sz - 1);
    int saved_errno = errno;
    close(fd);
    errno = saved_errno;

    if(n < 0)
    {
        return -1;
    }

    out[n] = '\0';
    return 0;
}

static int file_exists(const char* path)
{
    struct stat st;
    return stat(path, &st) == 0;
}

static int test_basic_write(const char* dir)
{
    char path[512];
    snprintf(path, sizeof(path), "%s/basic.log", dir);

    errno = 0;
    if(smartlog_write_log_entry(path, "hello-basic", FEATURE_DISABLED, FEATURE_DISABLED, 0) != 0)
    {
        perror("basic write");
        return 1;
    }

    char content[1024];
    if(read_file(path, content, sizeof(content)) != 0)
    {
        perror("read basic");
        return 1;
    }
    if(strstr(content, "MESSAGE = hello-basic") == NULL)
    {
        fprintf(stderr, "basic write content mismatch\n");
        return 1;
    }

    return 0;
}

static int test_rotation(const char* dir)
{
    char path[512];
    char backup[520];
    snprintf(path, sizeof(path), "%s/rotate.log", dir);
    snprintf(backup, sizeof(backup), "%s.1", path);

    if(smartlog_write_log_entry(path, "first", FEATURE_DISABLED, FEATURE_ENABLED, 10) != 0)
    {
        perror("rotation first");
        return 1;
    }
    if(smartlog_write_log_entry(path, "second", FEATURE_DISABLED, FEATURE_ENABLED, 10) != 0)
    {
        perror("rotation second");
        return 1;
    }

    if(!file_exists(backup))
    {
        fprintf(stderr, "rotation backup file missing\n");
        return 1;
    }

    char cur[1024];
    char old[1024];
    if(read_file(path, cur, sizeof(cur)) != 0 || read_file(backup, old, sizeof(old)) != 0)
    {
        perror("read rotation files");
        return 1;
    }
    if(strstr(cur, "MESSAGE = second") == NULL || strstr(old, "MESSAGE = first") == NULL)
    {
        fprintf(stderr, "rotation content mismatch\n");
        return 1;
    }

    return 0;
}

static int test_durable_write(const char* dir)
{
    char path[512];
    snprintf(path, sizeof(path), "%s/durable.log", dir);

    if(smartlog_write_log_entry(path, "durable-first", FEATURE_DISABLED, FEATURE_DISABLED, 0) != 0)
    {
        perror("durable first");
        return 1;
    }
    if(smartlog_write_log_entry(path, "durable-second", FEATURE_ENABLED, FEATURE_DISABLED, 0) != 0)
    {
        perror("durable second");
        return 1;
    }

    char content[2048];
    if(read_file(path, content, sizeof(content)) != 0)
    {
        perror("read durable");
        return 1;
    }
    if(strstr(content, "MESSAGE = durable-first") == NULL ||
       strstr(content, "MESSAGE = durable-second") == NULL)
    {
        fprintf(stderr, "durable content mismatch\n");
        return 1;
    }

    return 0;
}

static int test_empty_message_validation(const char* dir)
{
    char path[512];
    snprintf(path, sizeof(path), "%s/empty.log", dir);

    errno = 0;
    if(smartlog_write_log_entry(path, "", FEATURE_DISABLED, FEATURE_DISABLED, 0) == 0)
    {
        fprintf(stderr, "empty message unexpectedly succeeded\n");
        return 1;
    }
    if(errno != EINVAL)
    {
        fprintf(stderr, "empty message expected EINVAL, got %d\n", errno);
        return 1;
    }

    return 0;
}

static int test_timestamp_failure(const char* dir)
{
    char path[512];
    snprintf(path, sizeof(path), "%s/timestamp.log", dir);

    if(setenv("SMARTLOG_FAKE_CLOCK_FAIL", "1", 1) != 0)
    {
        perror("setenv");
        return 1;
    }

    errno = 0;
    int rc = smartlog_write_log_entry(path, "should-fail", FEATURE_DISABLED, FEATURE_DISABLED, 0);
    int saved_errno = errno;
    unsetenv("SMARTLOG_FAKE_CLOCK_FAIL");

    if(rc == 0)
    {
        fprintf(stderr, "timestamp failure unexpectedly succeeded\n");
        return 1;
    }
    if(saved_errno != EIO)
    {
        fprintf(stderr, "timestamp failure expected EIO, got %d\n", saved_errno);
        return 1;
    }

    return 0;
}

int main(void)
{
    char tmp_template[] = "/tmp/smartlog_tests_XXXXXX";
    char* dir = mkdtemp(tmp_template);
    if(dir == NULL)
    {
        perror("mkdtemp");
        return 1;
    }

    if(test_basic_write(dir) != 0) return 1;
    if(test_rotation(dir) != 0) return 1;
    if(test_durable_write(dir) != 0) return 1;
    if(test_empty_message_validation(dir) != 0) return 1;
    if(test_timestamp_failure(dir) != 0) return 1;

    return 0;
}
