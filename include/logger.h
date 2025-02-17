#ifndef UTILS_H
#define UTILS_H

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

#define LOG_ERROR(format, ...)                                          \
  fprintf(stderr, TERMINAL_RED "error" TERMINAL_RESET ": " format "\n", \
          ##__VA_ARGS__)

#define LOG_WARN(format, ...)                                                \
  fprintf(stderr, TERMINAL_YELLOW "warning" TERMINAL_RESET ": " format "\n", \
          ##__VA_ARGS__)

#define LOG_INFO(format, ...)                                           \
  fprintf(stderr, TERMINAL_BLUE "info" TERMINAL_RESET ": " format "\n", \
          ##__VA_ARGS__)

#define LOG_DEBUG(format, ...) \
  fprintf(stderr, "debug: " format "\n", ##__VA_ARGS__)

#define RETURN_DEFER(s) \
  do {                  \
    result = s;         \
    goto defer;         \
  } while (0)

#endif  // UTILS_H
