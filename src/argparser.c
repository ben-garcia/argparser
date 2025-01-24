#include "argparser.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dynamic_array.h"
#include "hash_table.h"
#include "utils.h"

/**
 * Add property to argparser.
 * If prop is not empty, then allocate memory.
 *
 * return 1 if prop is empty.
 */
#define ADD_PROP_TO_PARSER(parser, prop) \
  do {                                   \
    if (strlen((prop)) <= 0) {           \
      RETURN_DEFER(STATUS_FAILURE);      \
    }                                    \
    /*  TODO: malloc here */             \
    (*(parser))->prop = prop;            \
  } while (0)

/**
 * Retrieve argument from argparser.
 *
 * return 1 key was not found
 *        3 args is empty
 *        6 name is empty
 */
#define GET_ARG_FROM_PARSER(args, name, arg)                           \
  do {                                                                 \
    if (strlen((name))) {                                              \
      RETURN_DEFER(6);                                                 \
    }                                                                  \
    result = hash_table_search(args, name_or_flag, ((void **)(&arg))); \
    if (result == 1) {                                                 \
      /* indicates key was not found, */                               \
      RETURN_DEFER(STATUS_FAILURE);                                    \
    } else if (result == 3) {                                          \
      /* indicates parser->arguments is empty, */                      \
      RETURN_DEFER(STATUS_IS_EMPTY);                                   \
    }                                                                  \
  } while (0)

// typedef enum argparser_arg_type {
//   ARGPARSER_ARG_TYPE_POSITIONAL,  // always required.
//   ARGPARSER_ARG_TYPE_OPTIONAL,    // optional by default. Can be changed
// } argparser_arg_type;

typedef struct argparser_argument {
  argparser_arg_action action;  // how command line args should be handled.
  char *choices;        // Comma seperated string of acceptable arg values.
  char *const_value;    // Values not consumed by command line but required for
  char *default_value;  // Value to be used if arg is not present.
  bool deprecated;      // Indicates the argument is deprecated.
  char *dest;           // Set a custom name. Overrides long_name.
  char *help;           // Brief description of the argument.
  char *long_name;      // indicates whether arg is positional or optional(--).
  char *metavar;     // Change the arg input value name used in the help message
  char *nargs;       // number of values to accept. Works with action
                     // '?' single value which can be optional.
                     // '+' one or more values to store in array.
                     // '*' zero of more values to stor in array.
                     // if omitted, args consumed depends on action.
                     // Various actions('store_const', 'append_const').
  bool required;     // Used to make option args required.
  char *short_name;  // '-' + letter indicating optional argument.
  argparser_arg_type type;  // Type to convert to from string.
  // char *version;  // version of the program.
  union {
    char *str_value;         // Action 'store', 'store_const'
    dynamic_array *values;   // Actions 'append', 'append_const', 'extend'
    float float_value;       // Action 'store', 'store_const'
    unsigned int int_value;  // Action 'store', 'store_const', 'store_true',
                             // 'store_false', 'count'
  } type_value;
  // argparser_arg_type arg_type;  // positional or optional
} argparser_argument;

struct argparser {
  hash_table *arguments;  // Entries of argparser_argument.
  char *name;             // Program name(default is argv[0])
  char *usage;            // Describing program usage.
  char *description;      // Text to display before argument help message.
  char *epilogue;         // text to display after argument help message
  char *prefix_chars;     // Chars that prefix optional arguments('-')
  bool add_help;          // Add -h/--help option to the parser.
  bool allow_abbrev;      // Allow abbreviations of long args name.
};

/**
 * Allocate necessary resources and setup.
 *
 * @param arg argparser argument to create.
 * @param short_name 2 character flag.
 * @param long_name long form name.
 *
 * @return 0 on success, 2 indicates memory allocation failed.
 */
static int arg_create(argparser_argument **arg, char short_name[2],
                      char *long_name) {
  int result = STATUS_SUCCESS;

  if ((*arg = MALLOC(sizeof(argparser_argument))) == NULL) {
    RETURN_DEFER(STATUS_MEMORY_FAILURE);
  }

  // TODO: malloc here
  (*arg)->action = AP_ARG_ACTION_STORE;
  (*arg)->choices = NULL;
  (*arg)->const_value = NULL;
  (*arg)->default_value = NULL;
  (*arg)->deprecated = false;
  (*arg)->dest = NULL;
  (*arg)->help = NULL;
  (*arg)->long_name = long_name;
  (*arg)->metavar = NULL;
  (*arg)->nargs = NULL;
  (*arg)->required = false;
  (*arg)->short_name = short_name;
  (*arg)->type = AP_ARG_TYPE_STRING;

defer:
  return result;
}

static void arg_destroy(void **arg) {
  if (*arg != NULL) {
    // TODO: deallocate memory
    FREE(*arg);
    *arg = NULL;
  }
}

/**
 * Determine positional or optional argument.
 *
 * @param short_name flag that indicates optional argument.
 * @param long_name indicates positional argument.
 *
 * @return 1 for positional argument,
 *         2 for optional argument with short_name as key,
 *         3 for optional argument with long_name as key,
 *         -1 for error, short_name and long_name are NULL,
 *         -2 for error, short_name MUST contain dash(-) followed by alpha char.
 *         -3 for error, long_name cannot start with '--' for positional arg.
 *         -4 for error, long_name cannot start with '-' for positional arg.
 *         -5 for error, cannot mix positional and optional arguments.
 *         -6 SHOULD NOT RETURN THIS VALUE.
 */
static int determine_argument(char short_name[2], char *long_name) {
  if (short_name == NULL && long_name == NULL) {
    LOG_ERROR("both are NULL (%s:%s)", short_name, long_name);
    return -1;
  } else if (short_name == NULL && (strncmp(long_name, "--", 2) != 0) &&
             long_name[0] != '-') {
    // For positional argument.
    return 1;
  } else if (short_name == NULL && (strncmp(long_name, "--", 2) == 0)) {
    return 3;
  } else if (short_name == NULL && long_name[0] == '-') {
    LOG_ERROR("positional arg cannot begin with dash (-) (%s:%s)", short_name,
              long_name);
    return -4;
  }

  int short_length = strlen(short_name);

  if (short_length != 2 || short_name[0] != '-' || !isalpha(short_name[1])) {
    LOG_ERROR("invalid optional arg(%s:%s)", short_name, long_name);
    return -2;
  } else if (short_length == 2 && long_name == NULL) {
    // For optional argument with short_name as key.
    return 2;
  } else if (short_length == 2 && strncmp(long_name, "--", 2) == 0) {
    // For optional argument with long_name as key.
    return 3;
  } else if (short_name[0] == '-' && isalpha(short_name[1]) &&
             long_name[0] != '-' && strncmp(long_name, "--", 2) != 0) {
    LOG_ERROR("cannot mix positional and optional args (%s:%s)", short_name, long_name);
    return -5;
  }
  LOG_ERROR("UNKNOWN arg error (%s:%s)", short_name, long_name);
  return -6;

  // // Default prefix character
  // if (parser->prefix_chars == NULL) {
  //   // For positional argument.
  //   if (short_name == NULL && (strncmp(long_name, "--", 2) != 0)) {
  //     // For optional argument.
  //   } else if (short_length == 2 && (strncmp(short_name, "-", 1) == 0) &&
  //              isalpha(short_name[1]) && (strncmp(long_name, "--", 2) !=
  //              0))
  //              {
  //   }
  // } else {
  //   // Custom prefix character(s)
  // }
}

int argparser_create(argparser **parser) {
  int result = STATUS_SUCCESS;

  if ((*parser = MALLOC(sizeof(argparser))) == NULL) {
    LOG_ERROR("failed to allocated memory for argparser");
    RETURN_DEFER(STATUS_MEMORY_FAILURE);
  }

  result = hash_table_create(&(*parser)->arguments, sizeof(argparser_argument),
                             NULL, arg_destroy);
  if (result == 2) {
    RETURN_DEFER(STATUS_MEMORY_FAILURE);
  }

  (*parser)->name = NULL;
  (*parser)->usage = NULL;
  (*parser)->description = NULL;
  (*parser)->epilogue = NULL;
  (*parser)->prefix_chars = NULL;
  (*parser)->add_help = true;
  (*parser)->allow_abbrev = true;

defer:
  return result;
}

int argparser_add_name_to_argparser(argparser **parser, char *name) {
  int result = STATUS_SUCCESS;

  ADD_PROP_TO_PARSER(parser, name);

defer:
  return result;
}

int argparser_add_usage_to_argparser(argparser **parser, char *usage) {
  int result = STATUS_SUCCESS;

  ADD_PROP_TO_PARSER(parser, usage);

defer:
  return result;
}

int argparser_add_desc_to_argparser(argparser **parser, char *description) {
  int result = STATUS_SUCCESS;

  ADD_PROP_TO_PARSER(parser, description);

defer:
  return result;
}

int argparser_add_epilogue_to_argparser(argparser **parser, char *epilogue) {
  int result = STATUS_SUCCESS;

  ADD_PROP_TO_PARSER(parser, epilogue);

defer:
  return result;
}

int argparser_add_prechars_to_argparser(argparser **parser,
                                        char *prefix_chars) {
  int result = STATUS_SUCCESS;

  ADD_PROP_TO_PARSER(parser, prefix_chars);

defer:
  return result;
}

void argparser_add_help_to_argparser(argparser **parser, bool add_help) {
  (*parser)->add_help = add_help;
}

void argparser_add_abbrev_to_argparser(argparser **parser, bool allow_abbrev) {
  (*parser)->allow_abbrev = allow_abbrev;
}

int argparser_add_argument(argparser *parser, char short_name[2],
                           char *long_name) {
  int result = STATUS_SUCCESS;
  int arg_kind = 0;
  argparser_argument *arg = NULL;
  void *value = NULL;
  int search_result = -1;

  if (parser == NULL) {
    RETURN_DEFER(STATUS_IS_NULL);
  }

  arg_kind = determine_argument(short_name, long_name);
  if (arg_kind == 1) {
    // Positional argument

    search_result = hash_table_search(parser->arguments, long_name, &value);
    if (search_result == 0 && (value == NULL || value != NULL)) {
      // There already exists an entry if value is not NULL.
      // hash table is empty if value is NULL.
      // either case exit.
      RETURN_DEFER(6);
    }

    if ((arg_create(&arg, NULL, long_name) != 0)) {
      RETURN_DEFER(STATUS_MEMORY_FAILURE);
    }

    hash_table_insert(parser->arguments, long_name, arg);
  } else if (arg_kind == 2) {
    // Optional argument with short_name as key.

    search_result = hash_table_search(parser->arguments, short_name, &value);
    if (search_result == 0 && (value == NULL || value != NULL)) {
      // There already exists an entry if value is not NULL.
      // hash table is empty if value is NULL.
      // either case exit.
      RETURN_DEFER(6);
    }

    if ((arg_create(&arg, short_name, long_name) != 0)) {
      RETURN_DEFER(STATUS_MEMORY_FAILURE);
    }

    hash_table_insert(parser->arguments, short_name, arg);
  } else if (arg_kind == 3) {
    // Optional argument with long_name as key.

    search_result = hash_table_search(parser->arguments, long_name, &value);
    if (search_result == 0 && (value == NULL || value != NULL)) {
      // There already exists an entry if value is not NULL.
      // hash table is empty if value is NULL.
      // either case exit.
      RETURN_DEFER(6);
    }

    if ((arg_create(&arg, short_name, long_name) != 0)) {
      RETURN_DEFER(STATUS_MEMORY_FAILURE);
    }

    hash_table_insert(parser->arguments, long_name, arg);
  } else {
    // Argument formatting error.
    RETURN_DEFER(STATUS_FAILURE);
  }

defer:
  return result;
}

int argparser_add_action_to_arg(argparser *parser, char *name_or_flag,
                                argparser_arg_action action) {
  int result = STATUS_SUCCESS;
  argparser_argument *arg = NULL;

  GET_ARG_FROM_PARSER(parser->arguments, name_or_flag, arg);

  // TODO: malloc here
  arg->action = action;

defer:
  return result;
}

int argparser_add_type_to_arg(argparser *parser, char *name_or_flag,
                              argparser_arg_type type) {
  int result = STATUS_SUCCESS;
  argparser_argument *arg = NULL;

  GET_ARG_FROM_PARSER(parser->arguments, name_or_flag, arg);

  // TODO: malloc here
  arg->type = type;

defer:
  return result;
}

int argparser_add_help_to_arg(argparser *parser, char *name_or_flag,
                              char *help) {
  int result = STATUS_SUCCESS;
  argparser_argument *arg = NULL;

  GET_ARG_FROM_PARSER(parser->arguments, name_or_flag, arg);

  // TODO: malloc here
  arg->help = help;

defer:
  return result;
}

int argparser_add_required_to_arg(argparser *parser, char *name_or_flag,
                                  bool required) {
  int result = STATUS_SUCCESS;
  argparser_argument *arg = NULL;

  GET_ARG_FROM_PARSER(parser->arguments, name_or_flag, arg);

  // TODO: malloc here
  arg->required = required;

defer:
  return result;
}

int argparser_add_deprecated_to_arg(argparser *parser, char *name_or_flag,
                                    char deprecated) {
  int result = STATUS_SUCCESS;
  argparser_argument *arg = NULL;

  GET_ARG_FROM_PARSER(parser->arguments, name_or_flag, arg);

  // TODO: malloc here
  arg->deprecated = deprecated;

defer:
  return result;
}

int argparser_add_dest_to_arg(argparser *parser, char *name_or_flag,
                              char *dest) {
  int result = STATUS_SUCCESS;
  argparser_argument *arg = NULL;

  GET_ARG_FROM_PARSER(parser->arguments, name_or_flag, arg);

  // TODO: malloc here
  arg->dest = dest;

defer:
  return result;
}

int argparser_add_nargs_to_arg(argparser *parser, char *name_or_flag,
                               char *nargs) {
  int result = STATUS_SUCCESS;
  argparser_argument *arg = NULL;

  GET_ARG_FROM_PARSER(parser->arguments, name_or_flag, arg);

  // TODO: malloc here
  arg->nargs = nargs;

defer:
  return result;
}

int argparser_add_metavar_to_arg(argparser *parser, char *name_or_flag,
                                 char *metavar) {
  int result = STATUS_SUCCESS;
  argparser_argument *arg = NULL;

  GET_ARG_FROM_PARSER(parser->arguments, name_or_flag, arg);

  // TODO: malloc here
  arg->metavar = metavar;

defer:
  return result;
}

int argparser_add_default_value_to_arg(argparser *parser, char *name_or_flag,
                                       char *default_value) {
  int result = STATUS_SUCCESS;
  argparser_argument *arg = NULL;

  GET_ARG_FROM_PARSER(parser->arguments, name_or_flag, arg);

  // TODO: malloc here
  arg->default_value = default_value;

defer:
  return result;
}

int argparser_add_const_value_to_arg(argparser *parser, char *name_or_flag,
                                     char *const_value) {
  int result = STATUS_SUCCESS;
  argparser_argument *arg = NULL;

  GET_ARG_FROM_PARSER(parser->arguments, name_or_flag, arg);

  // TODO: malloc here
  arg->const_value = const_value;

defer:
  return result;
}

int argparser_add_choices_to_arg(argparser *parser, char *name_or_flag,
                                 char *choices) {
  int result = STATUS_SUCCESS;
  argparser_argument *arg = NULL;

  GET_ARG_FROM_PARSER(parser->arguments, name_or_flag, arg);

  // TODO: malloc here
  arg->choices = choices;

defer:
  return result;
}

// int argpaser_parse_args(argparser *parser, int argc, char *argv[]) {
//   int result = STATUS_SUCCESS;
//
//   RETURN_DEFER(STATUS_SUCCESS);
//
// defer:
//   return result;
// }

void argparser_destroy(argparser **parser) {
  if (*parser != NULL) {
    hash_table_destroy(&(*parser)->arguments);
    FREE(*parser);
    *parser = NULL;
  }
}
