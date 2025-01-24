#include "string_builder.h"

#include <stdarg.h>

#include "utils.h"

struct string_builder {
  dynamic_array *string;  // string being built
};

int string_builder_create(string_builder **sb) {
  int result = STATUS_SUCCESS;

  if (((*sb) = MALLOC(sizeof(string_builder))) == NULL) {
    LOG_ERROR("failed to allocated memory for string builder");
    RETURN_DEFER(STATUS_MEMORY_FAILURE);
  }

  result = dynamic_array_create(&(*sb)->string, sizeof(char), NULL, NULL);

defer:
  return result;
}

int string_builder_append(string_builder *sb, const char *str,
                          unsigned int length) {
  return dynamic_array_add_many(sb->string, (void **)str, length);
}

int string_builder_append_char(string_builder *sb, const char ch) {
  return dynamic_array_add(sb->string, &ch);
}

int string_builder_append_fmtstr(string_builder *sb, const char *format, ...) {
  // man vsnprintf example.
  int result = STATUS_SUCCESS;
  // number of characters the formatted string will contain
  // excluding the null byte.
  int num_chars = 0;
  // number bytes to allocate.
  int size = 0;
  // temp buffer to store formatted string.
  char *buffer = NULL;
  va_list args;

  va_start(args, format);
  num_chars = vsnprintf(buffer, size, format, args);
  va_end(args);

  size = num_chars + 1;  // extra byte for '\0'

  if ((buffer = MALLOC(size)) == NULL) {
    LOG_ERROR("failed to allocate memory for buffer");
    RETURN_DEFER(STATUS_MEMORY_FAILURE);
  }

  va_start(args, format);
  num_chars = vsnprintf(buffer, size, format, args);
  va_end(args);

  if ((string_builder_append(sb, buffer, num_chars)) != 0) {
    RETURN_DEFER(STATUS_FAILURE);
  }

defer:
  if (buffer != NULL) {
    free(buffer);
  }
  return result;
}

int string_builder_build(string_builder *sb, char **buffer) {
  int result = STATUS_SUCCESS;

  if ((dynamic_array_shrink_to_fit(sb->string)) != 0) {
    RETURN_DEFER(STATUS_FAILURE);
  }

  int bytes = sizeof(char) * dynamic_array_get_size(sb->string);

  if (((*buffer) = MALLOC(bytes + 1)) == NULL) {
    RETURN_DEFER(STATUS_MEMORY_FAILURE);
  }

  void *item = NULL;
  dynamic_array_find_ref(sb->string, 0, &item);

  MEMCPY(*buffer, item, bytes);
  (*buffer)[bytes] = '\0';

defer:
  return result;
}

void string_builder_destroy(string_builder **sb) {
  if (*sb != NULL) {
    dynamic_array_destroy(&(*sb)->string);

    free(*sb);
    (*sb) = NULL;
  }
}
