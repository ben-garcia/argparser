#ifndef STRING_BUILDER_H
#define STRING_BUILDER_H

/*
 * Dynamic string implemented using dynamic array.
 *
 * Creates a string character by character.
 */

#include "dynamic_array.h"

typedef struct string_builder string_builder;

/**
 * Allocate necessary resources and setup.
 *
 * @param sb string_builder to create.
 *
 * @return 0 on success, 2 indicates memory allocation failed.
 */
int string_builder_create(string_builder **sb);

/**
 * Add a string to the string builder.
 *
 * @param sb string_builder to modify.
 * @param str string to add.
 * @param length Number of characters in the str.
 *
 * @return 0 on success, 5 otherwise.
 */
int string_builder_append(string_builder *sb, const char *str,
                          unsigned int legnth);

/**
 * Add a character to the string builder.
 *
 * @param sb string_builder to modify.
 * @param ch character to add.
 *
 * @return 0 on success, 5 otherwise.
 */
int string_builder_append_char(string_builder *sb, const char ch);

/**
 * Add a formatted string to the string builder.
 *
 * @param sb string_builder to modify.
 * @param format formatted string.
 * @param ... variable number of arguments.
 *
 * @return 0 on success,
 *         1 indicates formated string could not be added,
 *         2 indicates memory allocation failure.
 */
int string_builder_append_fmtstr(string_builder *sb, const char *format, ...);

/**
 * Build string.
 *
 * @param sb string_builder to access.
 * @param buffer where to copy the string.
 *
 * @return 0 on success,
 *         1 indicates failure to build.
 *         2 indicates memory allocation failure.
 */
int string_builder_build(string_builder *sb, char **buffer);

/**
 * Check if string_builder is empty.
 *
 * @param sb string_builder to check.
 *
 * @return 0 on success, 1 otherwise.
 */
int string_builder_is_empty(string_builder *sb);

/**
 * Deallocate and set to NULL.
 *
 * @param sb string_builder to deallocate.
 */
void string_builder_destroy(string_builder **sb);

#endif  // STRING_BUILDER_H
