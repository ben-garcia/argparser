#include "dynamic_array.h"

#include <stdlib.h>
#include <string.h>

#include "logger.h"

struct dynamic_array {
  void **items;
  void (*freefn)(void **);
  int (*matchfn)(void *, void *);
  unsigned int capacity;   // Limit of items before resizing.
  unsigned int size;       // Number of elements in the array.
  unsigned int data_size;  // Bytes needed for every element..
};

struct dynamic_array_iter {
  void **items;
  unsigned data_size;
  unsigned size;
  unsigned index;  // current element iterator is pointing to.
};

#define DYNAMIC_ARRAY_INTIAL_CAPACITY 8

/**
 * Resize the dynamic array after capacity has been reached/exceeded.
 *
 * @param array dynamic array to modifiy.
 *
 * @return 0 on success, 2 indicates memory allocation failed.
 */
static int dynamic_array_resize(dynamic_array **array) {
  int result = STATUS_SUCCESS;
  size_t old_capacity = (*array)->capacity;

  if (((*array)->items =
           realloc((*array)->items,
                   (*array)->data_size * ((*array)->capacity <<= 1))) == NULL) {
    RETURN_DEFER(STATUS_MEMORY_FAILURE);
  }

  // since realloc doesn't zero out new space allocated,
  // it is done manually.
  memset((void *)(*array)->items + (old_capacity * (*array)->data_size), 0,
         (*array)->capacity - old_capacity);

defer:
  return result;
}

int dynamic_array_create(dynamic_array **array, unsigned int data_size,
                         void (*freefn)(void **),
                         int (*matchfn)(void *, void *)) {
  int result = STATUS_SUCCESS;

  if ((*array = malloc(sizeof(dynamic_array))) == NULL) {
    RETURN_DEFER(STATUS_MEMORY_FAILURE);
  }

  (*array)->capacity = DYNAMIC_ARRAY_INTIAL_CAPACITY;
  (*array)->data_size = data_size;
  (*array)->freefn = freefn;
  (*array)->matchfn = matchfn;
  (*array)->items = NULL;
  (*array)->size = 0;

defer:
  return result;
}

int dynamic_array_add(dynamic_array *array, const void *item) {
  int result = STATUS_SUCCESS;

  // array must be defined.
  if (array == NULL) {
    RETURN_DEFER(STATUS_IS_NULL);
  }

  // item must be defined.
  if (item == NULL) {
    RETURN_DEFER(STATUS_IS_NULL);
  }

  // array can't be empty.
  if (array->size == 0) {
    array->items = calloc(array->capacity, array->data_size);
  }

  if (array->size == array->capacity) {
    dynamic_array_resize(&array);
  }

  memcpy((void *)array->items + array->data_size * array->size++, item,
         array->data_size);

defer:
  return result;
}

int dynamic_array_add_str(dynamic_array *array, const char *str) {
  int result = STATUS_SUCCESS;

  // array must be defined.
  if (array == NULL) {
    RETURN_DEFER(STATUS_IS_NULL);
  }

  // item must be defined.
  if (str == NULL) {
    RETURN_DEFER(STATUS_IS_NULL);
  }

  // array can't be empty.
  if (array->size == 0) {
    array->items = calloc(array->capacity, array->data_size);
  }

  if (array->size == array->capacity) {
    dynamic_array_resize(&array);
  }

  array->items[array->size++] = (void *)str;

defer:
  return result;
}

int dynamic_array_add_many(dynamic_array *array, void **items,
                           unsigned int length) {
  int result = STATUS_SUCCESS;

  // array must be defined.
  if (array == NULL) {
    RETURN_DEFER(STATUS_IS_NULL);
  }

  // item must be defined.
  if ((*items) == NULL) {
    RETURN_DEFER(STATUS_IS_NULL);
  }

  if (dynamic_array_is_empty(array)) {
    array->items = calloc(array->capacity, array->data_size);
  }

  // make sure array has the necessary memory.
  while (array->capacity < array->size + length) {
    dynamic_array_resize(&array);
  }

  memcpy((void *)array->items + array->size * array->data_size, items,
         array->data_size * length);

  array->size += length;

defer:
  return result;
}

int dynamic_array_find(dynamic_array *array, unsigned int index, void **item) {
  int result = STATUS_SUCCESS;

  // array must be defined.
  if (array == NULL) {
    RETURN_DEFER(STATUS_IS_NULL);
  }

  // Array holds no elements.
  if (dynamic_array_is_empty(array)) {
    RETURN_DEFER(STATUS_IS_EMPTY);
  }

  // Index out of bounds.
  if (index >= array->size) {
    RETURN_DEFER(STATUS_OUT_OF_BOUNDS);
  }

  memcpy(*item, array->items + index, array->data_size);

defer:
  return result;
}

int dynamic_array_find_ref(dynamic_array *array, unsigned int index,
                           void **item) {
  int result = STATUS_SUCCESS;

  // array must be defined.
  if (array == NULL) {
    RETURN_DEFER(STATUS_IS_NULL);
  }

  // Array holds no elements.
  if (array->size == 0) {
    RETURN_DEFER(STATUS_IS_EMPTY);
  }

  // Index out of bounds.
  if (index >= array->size) {
    RETURN_DEFER(STATUS_OUT_OF_BOUNDS);
  }

  *item = (void *)array->items + index * array->data_size;

defer:
  return result;
}

int dynamic_array_find_ref_str(dynamic_array *array, unsigned int index,
                               void **str) {
  int result = STATUS_SUCCESS;

  // array must be defined.
  if (array == NULL) {
    RETURN_DEFER(STATUS_IS_NULL);
  }

  // Array holds no elements.
  if (dynamic_array_is_empty(array)) {
    RETURN_DEFER(STATUS_IS_EMPTY);
  }

  // Index out of bounds.
  if (index >= array->size) {
    RETURN_DEFER(STATUS_OUT_OF_BOUNDS);
  }

  *str = array->items[index];

defer:
  return result;
}

int dynamic_array_get_size(dynamic_array *array) {
  if (array == NULL) {
    return -1;
  }
  return array->size;
}

int dynamic_array_is_empty(dynamic_array *array) { return array->size == 0; }

int dynamic_array_remove(dynamic_array *array, unsigned int index) {
  int result = STATUS_SUCCESS;

  // array must be defined.
  if (array == NULL) {
    RETURN_DEFER(STATUS_IS_NULL);
  }

  // array can't be empty.
  if (dynamic_array_is_empty(array)) {
    RETURN_DEFER(STATUS_IS_EMPTY);
  }

  // Index out of bounds.
  if (index >= array->size) {
    RETURN_DEFER(STATUS_OUT_OF_BOUNDS);
  }

  array->size--;
  memmove(array->items + index, array->items + index + 1,
          array->data_size * array->size);

defer:
  return result;
}

int dynamic_array_shrink_to_fit(dynamic_array *array) {
  int result = STATUS_SUCCESS;

  // Array must be defined.
  if (array == NULL) {
    RETURN_DEFER(STATUS_IS_NULL);
  }

  // When array is already shrunk.
  if (array->size == array->capacity) {
    RETURN_DEFER(STATUS_SUCCESS);
  }

  array->items = realloc(array->items, array->size * array->data_size);
  array->capacity = array->size;

defer:
  return result;
}

void dynamic_array_destroy(dynamic_array **array) {
  if (*array != NULL) {
    if ((*array)->freefn != NULL) {
      for (unsigned int i = 0; i < (*array)->size; i++) {
        (*array)->freefn((*array)->items[i]);
      }
    }

    free((*array)->items);
    (*array)->items = NULL;
    free(*array);
    *array = NULL;
  }
}

int dynamic_array_iter_create(dynamic_array_iter **it, dynamic_array *array) {
  int result = STATUS_SUCCESS;

  if ((*it = malloc(sizeof(dynamic_array_iter))) == NULL) {
    RETURN_DEFER(STATUS_MEMORY_FAILURE);
  }

  (*it)->items = array->items;
  (*it)->size = array->size;
  (*it)->data_size = array->data_size;
  (*it)->index = 0;

defer:
  return result;
}

int dynamic_array_iter_next(dynamic_array_iter *it, void **item) {
  int result = STATUS_SUCCESS;

  // if (dynamic_array_is_empty((dynamic_array *)it->items)) {
  if (it->size == 0) {
    RETURN_DEFER(STATUS_IS_EMPTY);
  }

  if (it->index >= it->size) {
    RETURN_DEFER(STATUS_OUT_OF_BOUNDS);
  }

  // NOTE: A reference is returned.
  *item = it->items + (it->index++) * it->data_size;

defer:
  return result;
}

int dynamic_array_iter_next_str(dynamic_array_iter *it, char **item) {
  int result = STATUS_SUCCESS;

  if (it->size == 0) {
    RETURN_DEFER(STATUS_IS_EMPTY);
  }

  if (it->index >= it->size) {
    RETURN_DEFER(STATUS_OUT_OF_BOUNDS);
  }

  // NOTE: A reference is returned.
  *item = it->items[it->index++];

defer:
  return result;
}

void dynamic_array_iter_reset(dynamic_array_iter *it) { it->index = 0; }

void dynamic_array_iter_destroy(dynamic_array_iter **it) {
  if (*it != NULL) {
    free(*it);
    *it = NULL;
  }
}
