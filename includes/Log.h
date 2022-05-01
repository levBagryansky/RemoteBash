#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <fcntl.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>

#define log(fmt, ...) print_log(fmt, ##__VA_ARGS__)
#define log_info(fmt, ...) log("[INFO] " fmt, ##__VA_ARGS__)
#define log_error(fmt, ...) log("[ERROR] %s:%d " fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#define log_perror(fmt, ...) log ("[PERROR] %s:%d " fmt ": %s\n", __FILE__, __LINE__, strerror(errno), ##__VA_ARGS__)

#define TRY(cmd) \
  if (cmd < 0) \
  {              \
    log_perror(#cmd); \
    perror(#cmd); \
    return(-1); \
  }

#define LOG_SIZE (1 << 14)

static int log_fd = -1;
static char buf_log[LOG_SIZE];

int print_time()
{
    struct tm* curtime;

    time_t t;
    t = time(NULL);

    if (t == -1)
        exit(1);

    curtime = localtime(&t);

    if (!curtime)
        exit(1);

    return dprintf(log_fd, "%02d.%02d.%d %02d:%02d:%02d ", curtime -> tm_mday, curtime -> tm_mon + 1, curtime -> tm_year + 1900, curtime -> tm_hour, curtime -> tm_min, curtime -> tm_sec);
}

int init_log(char* path)
{
    static char* default_path = "./MyServerLogs";

    log_fd = open(path ? path : default_path, O_CREAT | O_RDWR | O_APPEND, 0666);
    if (log_fd < 0)
        exit(1);

    print_time();
    return dprintf(log_fd, "Successful log init.\n");
}

void print_log(char* str, ...)
{
    va_list ap;
    va_start(ap, str);

    if (log_fd < 0)
        init_log(NULL);

    print_time();

    int ret = vsnprintf(buf_log, LOG_SIZE, str, ap);
    write(log_fd, buf_log, ret);

    va_end(ap);
}

void printf_fd(int fd, char* str, ...)
{
    va_list ap;
    va_start(ap, str);

    char buf_printf[10000];

    if (fd < 0)
    {
        printf("no such file\n");
        exit(1);
    }

    int ret = vsnprintf(buf_printf, 10000, str, ap);
    write(fd, buf_printf, ret);

    va_end(ap);
}
