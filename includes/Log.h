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

int print_time();

int init_log(char* path);

void print_log(char* str, ...);

void printf_fd(int fd, char* str, ...);
