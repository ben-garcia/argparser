#include "string_slice.h"

#include <stdlib.h>
#include <string.h>

#include "logger.h"

struct string_slice {
  char *string;
  unsigned int length;
};

int string_slice_create(string_slice **ss, char *str, unsigned int length) {
  int result = STATUS_SUCCESS;

  if ((*ss = malloc(sizeof(string_slice))) == NULL) {
    RETURN_DEFER(STATUS_MEMORY_FAILURE);
  }
  (*ss)->string = str;
  (*ss)->length = length;

defer:
  return result;
}

int string_slice_advance(string_slice **ss) {
  int result = STATUS_SUCCESS;

  if (*ss == NULL) {
    RETURN_DEFER(STATUS_IS_NULL);
  }

  (*ss)->length++;

defer:
  return result;
}

int string_slice_split(string_slice *ss, string_slice *output,
                       const char delimiter) {
  int result = STATUS_SUCCESS;

  if (ss->string == NULL) {
    output->string = NULL;
    output->length = 0;
    RETURN_DEFER(STATUS_IS_NULL);
  }

  // string slice is empty.
  if (ss->length == 0) {
    output->string = NULL;
    output->length = 0;
    RETURN_DEFER(STATUS_IS_EMPTY);
  }

  output->string = ss->string;
  output->length = ss->length;

  for (unsigned int i = 0; i < ss->length; i++) {
    if (ss->string[i] == delimiter) {
      ss->string += i + 1;
      ss->length -= i + 1;

      output->length = i;

      RETURN_DEFER(STATUS_SUCCESS);
    }
  }

  // delimiter was not found.
  ss->string = NULL;
  ss->length = 0;

defer:
  return result;
}

int string_slice_trim(string_slice *ss) {
  int result = STATUS_SUCCESS;

  // string slice must be defined.
  if (ss->string == NULL) {
    RETURN_DEFER(STATUS_IS_NULL);
  }

  // string slice is empty.
  if (ss->length == 0) {
    RETURN_DEFER(STATUS_IS_EMPTY);
  }

  // trim left
  while (ss->string[0] == ' ') {
    ss->length--;
    ss->string++;
  }

  // trim right
  while (ss->string[ss->length - 1] == ' ') {
    ss->length--;
  }

defer:
  return result;
}

int string_slice_to_string(string_slice *ss, char **str) {
  int result = STATUS_SUCCESS;

  // string slice is empty.
  if (ss->string == NULL) {
    *str = NULL;
    ss->length = 0;
    RETURN_DEFER(STATUS_IS_NULL);
  }

  // string slice is empty.
  if (ss->length == 0) {
    *str = NULL;
    ss->string = NULL;
    RETURN_DEFER(STATUS_IS_EMPTY);
  }

  if (((*str) = malloc(ss->length + 1)) == NULL) {
    RETURN_DEFER(STATUS_MEMORY_FAILURE);
  }

  memcpy(*str, ss->string, ss->length);
  (*str)[ss->length] = '\0';

defer:
  return result;
}

void string_slice_destroy(string_slice **ss) {
  if (*ss != NULL) {
    free(*ss);
    *ss = NULL;
  }
}
