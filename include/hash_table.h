#ifndef HASH_TABLE_H
#define HASH_TABLE_H

typedef struct hash_table_entry hash_table_entry;
typedef struct hash_table hash_table;
typedef struct hash_table_iter hash_table_iter;

/**
 * Allocate necessary resources and setup.
 *
 * @param ht hash_table to create.
 * @param data_size size in bytes of each value stored.
 * @param hashfn hashing function, if set to NULL FNV-1a  is used.
 * @param freefn function used to deallocate user defined struct.
 *
 * @return 0 on success, 2 indicates memory allocation failed.
 */
int hash_table_create(hash_table **ht, unsigned int data_size,
                      unsigned int (*hashfn)(const char *, unsigned int),
                      void (*freefn)(void **));


/**
 * Retrive the number of entries in the hash table.
 *
 * @param ht the hash table to access.
 *
 * @return -1 indicates hash table is NULL, positive number otherwise.
 */
int hash_table_get_size(hash_table *ht);

/**
 * Insert an entry to the hash table.
 *
 * This function DOES NOT change the value of an existing entry.
 * To update the value use 'hash_table_insert_and_replace'.
 *
 * @param ht hash table to be modified.
 * @param key identifier used to access the data stored.
 * @param value item to insert.
 *
 * @return 0 on success,
 *         1 indicates cannot update existing entry.
 *         5 indicates hash table is NULL.
 */
int hash_table_insert(hash_table *ht, const char *key, const void *value);

/**
 * Insert/Update an entry to the hash table.
 *
 * This function DOES change the value of an existing entry.
 * To prevent this use 'hash_table_insert' instead.
 *
 * @param ht hash table to be modified.
 * @param key identifier used to access the data stored.
 * @param value item to insert.
 *
 * @return 0 on success, 5 indicates hash table is NULL.
 */
int hash_table_insert_and_replace(hash_table *ht, const char *key, void *value);

/**
 * Search for an entry in the hash table.
 *
 *
 *
 * @param ht hash table to be modified.
 * @param key identifier used to access the hash table entry.
 * @param value pointer used to get a reference to the entry's value.
 *
 * @return 0 on success, value is set to NULL if key if not found ,
 *         1 indicates key was not found,
 *         3 indicates ht is empty,
 *         5 indicates hash table is NULL.
 */
int hash_table_search(hash_table *ht, const char *key, void **value);

/**
 * Delete an entry from the hash table.
 *
 * @param ht hash table to be modified.
 * @param key identifier used to access the data stored.
 *
 * @return 0 on success,
 *         1 indicates key was not found,
 *         3 indicates ht is empty,
 *         5 indicates hash table is NULL.
 */
int hash_table_delete(hash_table *ht, const char *key);

/**
 * Retreive the key from the given hash table entry.
 *
 * Usedful when iterating throught the hash table.
 *
 * @param entry hash table entry.
 * @param key where to store the key of entry.
 *
 */
void hash_table_get_entry_key(hash_table_entry *entry, char **key);

/**
 * Retreive the value from the given hash table entry.
 *
 * Usedful when iterating throught the hash table.
 *
 * @param entry hash table entry.
 * @param value where to store the value of entry.
 *
 */
void hash_table_get_entry_value(hash_table_entry *entry, void **value);

/**
 * Deallocate and set to NULL.
 *
 * @param ht hash_table to deallocate.
 */
void hash_table_destroy(hash_table **ht);

/**
 * Allocate necessary resources and setup.
 *
 * Use to iterate through a hash table.
 *
 * @param it hash table iterator to create.
 * @param ht hash table to iterate through.
 *
 * @return 0 on success,
 *         2 indicates memory allocation failed,
 *         3 indicates ht is empty,
 *         5 indicates hash table is NULL.
 */
int hash_table_iter_create(hash_table_iter **it, hash_table *table);

/**
 * Get the next entry in the hash table.
 *
 * @param it hash table iterator
 * @param entry value used to hold the next entry in the hash table.
 *
 * @return 0 on success,
 *         1 indicates no more entries,
 *         3 indicates iterator is empty,
 *         4 indicates iterator is out of bounds,
 *         5 indicates hash table is NULL.
 */
int hash_table_iter_next(hash_table_iter *it, hash_table_entry **entry);

/**
 * Reset the hash table iterator.
 *
 * Use before iterating hash_table for a second time.
 *
 * @param it hash table iterator
 */
void hash_table_iter_reset(hash_table_iter *it);

/**
 * Deallocate and set to NULL.
 *
 * @param it hash table iterator to deallocate.
 */
void hash_table_iter_destroy(hash_table_iter **it);

#endif  // HASH_TABLE_H
