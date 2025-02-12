#include "hash_table.h"

#include <stdio.h>
#include <string.h>

#include "utils.h"

#define HASH_TABLE_LOAD_FACTOR 0.75
#define HASH_TABLE_INITIAL_SIZE 8

struct hash_table_entry {
  char *key;
  void *value;
};

struct hash_table {
  hash_table_entry *entries;
  unsigned size;
  unsigned capacity;
  unsigned data_size;
  unsigned int (*hashfn)(const char *, unsigned int);
  void (*freefn)(void **);
};

struct hash_table_iter {
  hash_table_entry *entries;
  unsigned size;
  unsigned capacity;
  unsigned int index;
};

/**
 * FNV-1A hashing function.
 */
static unsigned int hash(const char *key, unsigned int length) {
  unsigned int hash_value = 2166136261U;

  for (unsigned int i = 0; i < length; i++) {
    // for FNV-1 which the order
    hash_value ^= (unsigned char)key[i];
    hash_value *= 16777619;
  }

  return hash_value;
}

/**
 * Retreive a hash table entry.
 *
 * Used in search/insert/delete operations.
 *
 * @param array of hash table entries.
 * @param capacity max number of entries the hash table can hold at this time.
 * @param key identifier used to search for.
 * @param key identifier used to search for.
 *
 * @return hash table entry to modify.
 */
static hash_table_entry *find_entry(hash_table_entry *entries,
                                    unsigned int capacity, const char *key,
                                    unsigned int (*hashfn)(const char *,
                                                           unsigned int)) {
  unsigned int index = hashfn(key, strlen(key)) & (capacity - 1);
  hash_table_entry *tombstone = NULL;

  while (1) {
    hash_table_entry *entry = &entries[index];

    if (entry->key == NULL) {
      // Found empty entry
      if (entry->value == NULL) {
        return tombstone != NULL ? tombstone : entry;
      } else {
        // Found tombstone(prevously deleted entry).
        if (tombstone == NULL) {
          tombstone = entry;
        }
      }
    } else if (strcmp(entry->key, key) == 0) {
      return entry;
    }

    index = (index + 1) & (capacity - 1);
  }
}

/**
 * Resize the hash table after load factor has been reached/exceeded.
 *
 * @param ht hash table to modifiy.
 * @param capacity the new capacity.
 */
static void resize(hash_table *ht, int capacity) {
  hash_table_entry *new_entries = MALLOC(capacity * sizeof(hash_table_entry));

  for (int i = 0; i < capacity; i++) {
    new_entries[i].key = NULL;
    new_entries[i].value = NULL;
  }

  ht->size = 0;

  for (unsigned i = 0; i < ht->capacity; i++) {
    hash_table_entry *entry = &ht->entries[i];

    if (entry->key == NULL) {
      continue;
    }

    hash_table_entry *dest =
        find_entry(new_entries, capacity, entry->key, ht->hashfn);

    dest->key = entry->key;
    dest->value = entry->value;
    ht->size++;
  }

  FREE(ht->entries);

  ht->entries = new_entries;
  ht->capacity = capacity;
}

int hash_table_create(hash_table **ht, unsigned int data_size,
                      unsigned int (*hashfn)(const char *, unsigned int),
                      void (*freefn)(void **)) {
  int result = STATUS_SUCCESS;

  if ((*ht = MALLOC(sizeof(hash_table))) == NULL) {
    RETURN_DEFER(STATUS_MEMORY_FAILURE);
  }

  (*ht)->data_size = data_size;
  (*ht)->size = 0;
  (*ht)->capacity = HASH_TABLE_INITIAL_SIZE;
  (*ht)->hashfn = hashfn == NULL ? hash : hashfn;
  (*ht)->freefn = freefn;
  (*ht)->entries = NULL;

defer:
  return result;
}

int hash_table_get_size(hash_table *ht) {
  if (ht == NULL) {
    return -1;
  }
  return ht->size;
}

int hash_table_insert(hash_table *ht, const char *key, const void *value) {
  int result = STATUS_SUCCESS;

  if (ht == NULL) {
    RETURN_DEFER(STATUS_IS_NULL);
  }

  if (ht->size == 0) {
    ht->entries = MALLOC(ht->capacity * sizeof(hash_table_entry));

    // Setting keys and values to NULL indicates the position is empty. As
    // opposed to a tombstone.
    for (unsigned i = 0; i < ht->capacity; i++) {
      ht->entries[i].key = NULL;
      ht->entries[i].value = NULL;
    }
  }

  // Double the capacity of the ht when load factor is reached.
  if (ht->size + 1 >= ht->capacity * HASH_TABLE_LOAD_FACTOR) {
    resize(ht, ht->capacity * 2);
  }

  hash_table_entry *entry =
      find_entry(ht->entries, ht->capacity, key, ht->hashfn);
  int is_new_key = entry->key == 0;
  int key_length = strlen(key);

  if (!is_new_key) {
    // Key is already found
    RETURN_DEFER(STATUS_FAILURE);
  }

  if (entry->value != (void *)1) {
    ht->size++;
  }

  entry->key = MALLOC(sizeof(char) * (key_length + 1));
  strcpy(entry->key, key);
  entry->key[key_length] = '\0';

  if (ht->freefn == NULL) {
    entry->value = MALLOC(ht->data_size);
    MEMCPY((void *)entry->value, value, ht->data_size);
  } else {
    // Indicates user is responsible for allocate/deallocate memory.
    entry->value = (void *)value;
  }

defer:
  return result;
}

int hash_table_insert_and_replace(hash_table *ht, const char *key,
                                  void *value) {
  int result = STATUS_SUCCESS;

  if (ht == NULL) {
    RETURN_DEFER(STATUS_IS_NULL);
  }

  if (ht->size == 0) {
    ht->entries = MALLOC(ht->capacity * sizeof(hash_table_entry));

    // Setting keys and values to NULL indicates the position is empty. As
    // opposed to a tombstone.
    for (unsigned int i = 0; i < ht->capacity; i++) {
      ht->entries[i].key = NULL;
      ht->entries[i].value = NULL;
    }
  }

  // Double the capacity of the ht when load factor is reached.
  if (ht->size + 1 >= ht->capacity * HASH_TABLE_LOAD_FACTOR) {
    resize(ht, ht->capacity * 2);
  }

  hash_table_entry *entry =
      find_entry(ht->entries, ht->capacity, key, ht->hashfn);
  int is_new_key = entry->key == NULL;
  int key_length = strlen(key);

  if (is_new_key) {
    ht->size++;

    entry->key = MALLOC(sizeof(char) * (key_length + 1));
    strcpy(entry->key, key);
    entry->key[key_length] = '\0';

    if (ht->freefn == NULL) {
      entry->value = MALLOC(ht->data_size);
      MEMCPY((void *)entry->value, value, ht->data_size);
    } else {
      // Indicates user is responsible for allocate/deallocate memory.
      entry->value = (void *)value;
    }
  } else {
    if (ht->freefn == NULL) {
      MEMCPY((void *)entry->value, value, ht->data_size);
    } else {
      ht->freefn(&entry->value);
      entry->value = (void *)value;
    }
  }

defer:
  return result;
}

int hash_table_search(hash_table *ht, const char *key, void **value) {
  int result = STATUS_SUCCESS;

  if (ht == NULL) {
    RETURN_DEFER(STATUS_IS_NULL);
  }

  if (ht->size == 0) {
    RETURN_DEFER(STATUS_IS_EMPTY);
  }

  if (strlen(key) == 0) {
    if (value != NULL) {
      // Ignore value
      *value = NULL;
    }
    RETURN_DEFER(STATUS_FAILURE);
  }

  hash_table_entry *entry =
      find_entry(ht->entries, ht->capacity, key, ht->hashfn);

  if (entry->key == NULL) {
    if (value != NULL) {
      // Ignore value
      *value = NULL;
    }
    RETURN_DEFER(STATUS_FAILURE);
  }

  if (value != NULL) {
    // Ignore value
    *value = entry->value;
  }

defer:
  return result;
}

int hash_table_delete(hash_table *ht, const char *key) {
  int result = STATUS_SUCCESS;

  if (ht == NULL) {
    RETURN_DEFER(STATUS_IS_NULL);
  }

  if (ht->size == 0) {
    RETURN_DEFER(STATUS_IS_EMPTY);
  }

  hash_table_entry *entry =
      find_entry(ht->entries, ht->capacity, key, ht->hashfn);

  // Key not found, there is no entry to delete.
  if (entry->key == NULL) {
    RETURN_DEFER(STATUS_FAILURE);
  }

  ht->size--;

  FREE(entry->key);
  entry->key = NULL;

  FREE(entry->value);
  // entry value of 1 means the entry is a tombstone .
  entry->value = (void *)1;

defer:
  return result;
}

void hash_table_get_entry_key(hash_table_entry *entry, char **key) {
  *key = entry->key;
}

void hash_table_get_entry_value(hash_table_entry *entry, void **value) {
  *value = entry->value;
}

void hash_table_destroy(hash_table **ht) {
  // Deallocation only occurs for a previously created hash table.
  if (*ht != NULL) {
    // When the hash table is empty.
    if ((*ht)->size == 0) {
      FREE(*ht);
      *ht = NULL;
      return;
    }

    for (unsigned int i = 0; i < (*ht)->capacity; i++) {
      if ((*ht)->entries[i].key != NULL && (*ht)->entries[i].value != NULL) {
        FREE((*ht)->entries[i].key);

        if ((*ht)->freefn != NULL) {
          (*ht)->freefn(&(*ht)->entries[i].value);
        } else {
          FREE((*ht)->entries[i].value);
        }
      }
    }

    FREE((*ht)->entries);
    (*ht)->entries = NULL;
    FREE(*ht);
    *ht = NULL;
  }
}

int hash_table_iter_create(hash_table_iter **it, hash_table *ht) {
  int result = STATUS_SUCCESS;

  if (ht == NULL) {
    RETURN_DEFER(STATUS_IS_NULL);
  }

  if (ht->size == 0) {
    RETURN_DEFER(STATUS_IS_EMPTY);
  }
  if (((*it) = MALLOC(sizeof(hash_table_iter))) == NULL) {
    RETURN_DEFER(STATUS_MEMORY_FAILURE);
  }

  (*it)->entries = ht->entries;
  (*it)->size = ht->size;
  (*it)->capacity = ht->capacity;
  (*it)->index = 0;

defer:
  return result;
}

int hash_table_iter_next(hash_table_iter *it, hash_table_entry **entry) {
  int result = STATUS_SUCCESS;

  if (it == NULL) {
    RETURN_DEFER(STATUS_IS_NULL);
  }

  if (it->size == 0) {
    RETURN_DEFER(STATUS_IS_EMPTY);
  }

  if (it->index > it->capacity - 1) {
    RETURN_DEFER(STATUS_OUT_OF_BOUNDS);
  }

  for (unsigned int i = it->index; i <= it->capacity - 1; i++) {
    *entry = &it->entries[i];

    if ((*entry)->key != NULL && (*entry)->value != NULL) {
      it->index++;
      break;
    }

    // If there is no entry at the last index there is no need to print.
    if (it->index >= it->capacity - 1) {
      RETURN_DEFER(STATUS_FAILURE);
    }

    it->index++;
  }

defer:
  return result;
}

void hash_table_iter_reset(hash_table_iter *it) { it->index = 0; }

void hash_table_iter_destroy(hash_table_iter **it) {
  FREE(*it);
  *it = NULL;
}
