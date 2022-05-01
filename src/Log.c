#include "../includes/Log.h"

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