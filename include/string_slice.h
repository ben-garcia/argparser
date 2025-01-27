#ifndef STRING_SLICE_H
#define STRING_SLICE_H

typedef struct string_slice string_slice;

/**
 * Allocate necessary resources and setup.
 *
 * @param ss string_slice to create.
 * @param str string to slice.
 * @param length number of characters in str.
 *
 * @return 0 on success, 2 indicates memory allocation failed.
 */
int string_slice_create(string_slice **ss, char *str, unsigned int length);

/**
 * Increment the length of the string slice. 
 *
 * @param ss string_slice to create.
 *
 * @return 0 on success, 5 ss is NULL. 
 */
int string_slice_advance(string_slice **ss);

/**
 * Seperate string into different regions determined by a delimioter.
 *
 * @param ss string_slice to modify.
 * @param output where to store different regions.
 * @param delimiter character to mark the seperation.
 *
 * @example
 *   ...
 *   while (string_slice_split(ss, output, ':') == 0) {
 *     // Do Work
 *   }
 *
 * @return 0 on success, 2 indicates memory allocation failed.
 */
int string_slice_split(string_slice *ss, string_slice *output,
                       const char delimiter);

/**
 * Remove whitespace before and after the string being built.
 *
 * @param ss string_slice to modify.
 *
 * @return 0 on success,
 *         3 indicates 'ss' is empty,
 *         5 indicates 'ss' is NULL,
 */
int string_slice_trim(string_slice *ss);

/**
 * Remove whitespace before and after the string being built.
 *
 * @param ss string_slice to modify.
 * @param str where to store the constructed string slice.
 *
 * @return 0 on success,
 *         2 indicates failure to allocation memory for 'str',
 *         5 indicates ss is NULL,
 */
int string_slice_to_string(string_slice *ss, char **str);

/**
 * Deallocate and set to NULL.
 *
 * @param ss string_builder to deallocate.
 */
void string_slice_destroy(string_slice **ss);

#endif  // STRING_SLICE_H
