#include <stdio.h>
#include <stdlib.h>

#include "argparser.h"
#include "hash_table.h"
#include "utils.h"

// int main(int argc, char *argv[]) {
int main() {
  int result = STATUS_SUCCESS;
  argparser *parser = NULL;
  hash_table *ht = NULL;
  hash_table_iter *it = NULL;
  hash_table_entry *entry = NULL;

  printf("****************************************\n");
  printf("*******ARGPARSER************************\n");
  printf("****************************************\n");

  argparser_create(&parser);
  argparser_add_name_to_argparser(&parser, "test");
  argparser_add_usage_to_argparser(&parser, "best usage");
  argparser_add_desc_to_argparser(&parser, "best description");
  argparser_add_epilogue_to_argparser(&parser, "best epilogue");
  argparser_add_prechars_to_argparser(&parser, "-+");
  argparser_add_help_to_argparser(&parser, false);
  argparser_add_abbrev_to_argparser(&parser, false);

  argparser_add_argument(parser, "-h", "--help");   // valid
  argparser_add_argument(parser, NULL, "version");  // valid
  argparser_add_argument(parser, "-A", NULL);       // valid
  argparser_add_argument(parser, NULL, "--force");  // valid
  argparser_add_argument(parser, NULL, NULL);       // not valid
  argparser_add_argument(parser, NULL, "--force");  // not valid
  argparser_add_argument(parser, NULL, "-export");    // not valid
  argparser_add_argument(parser, "n", "name");        // not valid
  argparser_add_argument(parser, "-t", "terminate");  // not valid
  argparser_add_argument(parser, "-ww", "www");       // not valid
  argparser_add_argument(parser, "!E", "--extra");    // not valid

  printf("***************************************\n");
  printf("*******TESTING INT**********************\n");
  printf("****************************************\n");

  if ((hash_table_create(&ht, sizeof(int), NULL, NULL)) != 0) {
    LOG_ERROR("failure");
    RETURN_DEFER(STATUS_FAILURE);
  }

  char str[20];
  for (int c = 65; c < 69; c++) {
    sprintf(str, "i:%i", c);
    hash_table_insert(ht, str, (void *)&c);
  }

  hash_table_iter_create(&it, ht);

  char *key = NULL;
  int *int_value = NULL;
  while (hash_table_iter_next(it, &entry) == 0) {
    hash_table_get_entry_key(entry, &key);
    hash_table_get_entry_value(entry, (void **)&int_value);
    LOG_DEBUG("key: %s, value: %i", key, *int_value);
  }

  hash_table_delete(ht, "i:66");
  hash_table_delete(ht, "i:67");

  int *in = NULL;
  hash_table_search(ht, "i:66", (void **)&in);
  if (in != NULL) {
    LOG_DEBUG("i:66: %i", *in);
  } else {
    LOG_DEBUG("'i:66' key not found");
  }

  int a = 2222;

  hash_table_insert_and_replace(ht, "i:80", (void *)&a);
  hash_table_insert_and_replace(ht, "i:65", (void *)&a);
  hash_table_search(ht, "i:80", (void **)&int_value);
  LOG_DEBUG("i:80: %i", *int_value);

  hash_table_iter_destroy(&it);
  hash_table_destroy(&ht);

  printf("****************************************\n");
  printf("*******TESTING CHAR*********************\n");
  printf("****************************************\n");

  if ((hash_table_create(&ht, sizeof(char), NULL, NULL)) != 0) {
    LOG_ERROR("failure");
    RETURN_DEFER(STATUS_FAILURE);
  }

  for (char c = 65; c <= 69; c++) {
    sprintf(str, "c:%c", c);
    hash_table_insert(ht, str, &c);
  }

  hash_table_iter_create(&it, ht);

  key = NULL;
  char *char_value = NULL;
  while (hash_table_iter_next(it, &entry) == 0) {
    hash_table_get_entry_key(entry, &key);
    hash_table_get_entry_value(entry, (void **)&char_value);
    LOG_DEBUG("key: %s, value: %c'", key, *char_value);
  }

  hash_table_delete(ht, "c:A");
  hash_table_delete(ht, "c:B");

  hash_table_search(ht, "c:A", (void **)&char_value);
  if (in != NULL) {
    LOG_DEBUG("c:L: %c", *char_value);
  } else {
    LOG_DEBUG("c:L: key not found");
  }

  char c = 'A';
  hash_table_insert_and_replace(ht, "c:L", (void **)&c);
  hash_table_insert_and_replace(ht, "c:B", (void **)&c);
  hash_table_search(ht, "c:L", (void **)&char_value);
  if (char_value != NULL) {
    LOG_DEBUG("c:L: %c", *char_value);
  } else {
    LOG_DEBUG("c:L: key not found");
  }

  hash_table_iter_destroy(&it);
  hash_table_destroy(&ht);

  printf("****************************************\n");
  printf("*******TESTING FLOAT********************\n");
  printf("****************************************\n");

  if ((hash_table_create(&ht, sizeof(float), NULL, NULL)) != 0) {
    LOG_ERROR("failure");
    RETURN_DEFER(STATUS_FAILURE);
  }

  for (float c = 65.0; c <= 69.0; c += 0.823) {
    sprintf(str, "f:%f", c);
    hash_table_insert(ht, str, &c);
  }

  hash_table_iter_create(&it, ht);

  key = NULL;
  float *float_value = NULL;
  while (hash_table_iter_next(it, &entry) == 0) {
    hash_table_get_entry_key(entry, &key);
    hash_table_get_entry_value(entry, (void **)&float_value);
    LOG_DEBUG("key: %s, value: %f'", key, *float_value);
  }

  hash_table_delete(ht, "f:A");
  hash_table_delete(ht, "f:B");

  hash_table_search(ht, "f:A", (void **)&float_value);
  if (float_value != NULL) {
    LOG_DEBUG("f:L: %f", *float_value);
  } else {
    LOG_DEBUG("f:L: key not found");
  }

  float f = 3.14;
  hash_table_insert_and_replace(ht, "f:L", (void **)&f);
  hash_table_insert_and_replace(ht, "f:B", (void **)&f);
  hash_table_search(ht, "f:L", (void **)&float_value);
  if (float_value != NULL) {
    LOG_DEBUG("f:L: %f", *float_value);
  } else {
    LOG_DEBUG("f:L: key not found");
  }

defer:
  if (parser != NULL) {
    argparser_destroy(&parser);
  }
  if (ht != NULL) {
    hash_table_destroy(&ht);
  }
  if (it != NULL) {
    hash_table_iter_destroy(&it);
  }
  return result;
}
