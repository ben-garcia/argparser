#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum status_codes {
  STATUS_SUCCESS,
  STATUS_FAILURE,
  STATUS_MEMORY_FAILURE,
  STATUS_IS_EMPTY,
  STATUS_OUT_OF_BOUNDS,
  STATUS_IS_NULL,
};

#define TERMINAL_RED "\033[31m"
#define TERMINAL_BLUE "\033[34m"
#define TERMINAL_YELLOW "\033[33m"
#define TERMINAL_RESET "\033[0m"

#define LOG_ERROR(format, ...)                                                 \
  fprintf(stderr, TERMINAL_RED "error" TERMINAL_RESET ": %s:%d: " format "\n", \
          __FILE__, __LINE__, ##__VA_ARGS__)

#define LOG_WARN(format, ...)                                               \
  fprintf(stderr,                                                           \
          TERMINAL_YELLOW "warning" TERMINAL_RESET ": %s:%d: " format "\n", \
          __FILE__, __LINE__, ##__VA_ARGS__)

#define LOG_INFO(format, ...)                                                  \
  fprintf(stderr, TERMINAL_BLUE "info" TERMINAL_RESET ": %s:%d: " format "\n", \
          __FILE__, __LINE__, ##__VA_ARGS__)

#define LOG_DEBUG(format, ...)                                      \
  fprintf(stderr, "debug: %s:%d: " format "\n", __FILE__, __LINE__, \
          ##__VA_ARGS__)

#define RETURN_DEFER(s) \
  do {                  \
    result = s;         \
    goto defer;         \
  } while (0)

#define MALLOC(size) malloc((size))
#define REALLOC(ptr, size) realloc((ptr), (size))
#define CALLOC(nmemb, size) calloc((nmemb), (size))
#define FREE(ptr) free((ptr))
#define MEMCPY(dst, src, size) memcpy((dst), (src), (size))
#define MEMSET(ptr, ch, size) memset((ptr), (ch), (size))
#define MEMMOVE(ptr, ch, size) memmove((ptr), (ch), (size))

#endif  // UTILS_H
