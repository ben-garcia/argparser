#ifndef DYNAMIC_ARRAY_H
#define DYNAMIC_ARRAY_H

typedef struct dynamic_array dynamic_array;
typedef struct dynamic_array_iter dynamic_array_iter;

/**
 * Allocate necessary resources and setup.
 *
 * @param array dynamic_array to create.
 * @param data_size Size in bytes of each value stored.
 * @param freefn Function used to deallocate user defined structure.
 * @param matchfn Function used to find a user defined struture.
 *
 * @return 0 on success, 2 indicates memory allocation failed.
 */
int dynamic_array_create(dynamic_array **array, unsigned int data_size,
                         void (*freefn)(void **),
                         int (*matchfn)(void *, void *));

/**
 * Add a new element to the array.
 *
 * @param array dynamic_array to modify.
 * @param item the element to add.
 *
 * @return 0 on success, 5 indicates array or item is NULL.
 */
int dynamic_array_add(dynamic_array *array, const void *item);

/**
 * Add a new string to the array.
 *
 * NOTE: This is not ideal. I would like to have a single add function.
 *       However, add function uses memcpy while strcpy is necessary for
 *       strings.
 *
 * @param array dynamic_array to modify.
 * @param str the string to add.
 *
 * @return 0 on success, 5 indicates array or item is NULL.
 */
int dynamic_array_add_str(dynamic_array *array, const char *str);

/**
 * Add a multiple elements to the array.
 *
 * @param array dynamic_array to modify.
 * @param items the elements to add.
 * @param length number of elements to add.
 *
 * @return 0 on success, 5 indicates array or item is NULL.
 */
int dynamic_array_add_many(dynamic_array *array, void **items,
                           unsigned int length);

/**
 * Get the value at a given index.
 *
 * @param array dynamic_array to modify.
 * @param index Index to access.
 * @param item Where to store the value.
 *
 * @return 0 on success,
 *         3 indicates array is empty,
 *         5 indicates array or item is NULL,
 *         5 indicates array is NULL.
 */
int dynamic_array_find(dynamic_array *array, unsigned int index, void **item);

/**
 * Get a reference to the element at a given index.
 *
 * @param array dynamic_array to modify.
 * @param index Index to access.
 * @param item Where to store the value.
 *
 * @return 0 on success,
 *         3 indicates array is empty,
 *         5 indicates array or item is NULL,
 */
int dynamic_array_find_ref(dynamic_array *array, unsigned int index,
                           void **item);

/**
 * Get a reference to string value at a given index.
 *
 * @param array dynamic_array to modify.
 * @param index Index to access.
 * @param item Where to store the value.
 *
 * @return 0 on success,
 *         3 indicates array is empty,
 *         5 indicates array or item is NULL,
 *         5 indicates array is NULL.
 */
int dynamic_array_find_ref_str(dynamic_array *array, unsigned int index,
                               void **str);

/**
 * Retrive the number of elements in the array.
 *
 * @param array the array to access.
 *
 * @return -1 indicates array is NULL, positive number otherwise.
 */
int dynamic_array_get_size(dynamic_array *array);

/**
 * Check if array is empty.
 *
 * @param array dynamic_array to check.
 *
 * @return 0 on success, 1 otherwise.
 */
int dynamic_array_is_empty(dynamic_array *array);

/**
 * Remove an element at a given index.
 *
 * @param array dynamic_array to modify.
 * @param index Index to access.
 *
 * @return 0 on success,
 *         3 indicates array is empty,
 *         5 indicates array is NULL.
 */
int dynamic_array_remove(dynamic_array *array, unsigned int index);

/**
 * Reallocate to only take up the necessary memory to hold the current size.
 *
 * @param array dynamic_array to modify.
 *
 * @return 0 on success,
 *         3 indicates array is empty,
 *         5 indicates array is NULL.
 */
int dynamic_array_shrink_to_fit(dynamic_array *array);

/**
 * Deallocate and set to NULL.
 *
 * @param array dynamic_array to deallocate.
 */
void dynamic_array_destroy(dynamic_array **array);

/**
 * Allocate necessary resources and setup.
 *
 * Use to iterate through a dynamic array.
 *
 * @param it dynamic array iterator to create.
 * @param array dynamic_array array to iterate through.
 *
 * @return 0 on success, 2 indicates memory allocation failed,
 */
int dynamic_array_iter_create(dynamic_array_iter **it, dynamic_array *array);

/**
 * Get the next element of the dynamic array
 *
 * @param it dynamic array iterator
 * @param item value used to hold the next elmeent in the array.
 *
 * @return 0 on success,
 *         1 indicates no more entries,
 *         3 indicates iterator is empty,
 *         4 indicates iterator is out of bounds,
 *         5 indicates dynamic array is NULL.
 */
int dynamic_array_iter_next(dynamic_array_iter *it, void **item);

/**
 * Get the next string of the dynamic array
 *
 * @param it dynamic array iterator
 * @param item value used to hold the next elmeent in the array.
 *
 * @return 0 on success,
 *         1 indicates no more entries,
 *         3 indicates iterator is empty,
 *         4 indicates iterator is out of bounds,
 *         5 indicates dynamic array is NULL.
 */
int dynamic_array_iter_next_str(dynamic_array_iter *it, char **item);

/**
 * Reset the dynamic array iterator.
 *
 * @param it dynamic array iterator
 */
void dynamic_array_iter_reset(dynamic_array_iter *it);

/**
 * Deallocate and set to NULL.
 *
 * @param it dynamic array iterator to deallocate.
 */
void dynamic_array_iter_destroy(dynamic_array_iter **it);

#endif  // DYNAMIC_ARRAY_H
