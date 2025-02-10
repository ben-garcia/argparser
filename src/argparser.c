#include "argparser.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dynamic_array.h"
#include "hash_table.h"
#include "string_builder.h"
#include "string_slice.h"
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
    if (strlen((name)) == 0) {                                         \
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

typedef enum arg_kind {
  ARG_KIND_OPT_FLAG,
  ARG_KIND_OPT_NAME,
} arg_kind;

// typedef enum argparser_arg_type {
//   ARGPARSER_ARG_TYPE_POSITIONAL,  // always required.
//   ARGPARSER_ARG_TYPE_OPTIONAL,    // optional by default. Can be changed
// } argparser_arg_type;

typedef struct argparser_argument {
  argparser_arg_action action;  // how command line args should be handled.
  char *choices;        // Comma seperated string of acceptable arg values.
  char *const_value;    // Values not consumed by command line but required for
  char *default_value;  // Value to be used if arg is not present.
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
  char *short_name;  // '-' + letter indicating optional argument.
  void *value;       // Action 'store', 'store_const'
                     // Actions 'append', 'append_const', 'extend'
                     // Action 'store', 'store_const'
                     // Action 'store', 'store_const', 'count'
                     // Actions 'store_true', and 'store_false'.

  char deprecated;          // Indicates the argument is deprecated.
  char required;            // Used to make option args required.
  argparser_arg_type type;  // Type to convert to from string.
  // char *version;  // version of the program.
  // argparser_arg_type arg_type;  // positional or optional
} argparser_argument;

struct argparser {
  hash_table *arguments;              // Entries of argparser_argument.
  string_builder *positional_args;    // Concatenated string of all positional
                                      // argument names.
  string_builder *optional_args;      // Concatenated string of all optional
                                      // argument short names.
  string_builder *req_opt_args;       // Optional arguments that must
                                      // be passed in.
  string_builder *unrecognized_args;  // Arguments that don't match the
                                      // parser arguments.
  dynamic_array *errors;              // Argument errors. Array of char*.
  char *name;                         // Program name(default is argv[0])
  char *usage;                        // Describing program usage.
  char *description;           // Text to display before argument help message.
  char *epilogue;              // text to display after argument help message
  char *prefix_chars;          // Chars that prefix optional arguments('-')
  char add_help;               // Add -h/--help option to the parser.
  char allow_abbrev;           // Allow abbreviations of long args name.
  unsigned int pos_args_size;  // Number of positional arguments.
  unsigned int req_opt_args_size;  // Number of required optional arguments.
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
  (*arg)->action = AP_ARG_STORE;
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
  (*arg)->type = AP_ARG_STRING;
  (*arg)->value = NULL;

defer:
  return result;
}

static void arg_destroy(void **arg) {
  if (*arg != NULL) {
    if (((argparser_argument *)(*arg))->value != NULL) {
      FREE(((argparser_argument *)(*arg))->value);
    }

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
    // LOG_ERROR("both are NULL (%s:%s)", short_name, long_name);
    return -1;
  } else if (short_name == NULL && (strncmp(long_name, "--", 2) != 0) &&
             long_name[0] != '-') {
    // For positional argument.
    return 1;
  } else if (short_name == NULL && (strncmp(long_name, "--", 2) == 0)) {
    return 3;
  } else if (short_name == NULL && long_name[0] == '-') {
    // LOG_ERROR("positional arg cannot begin with dash (-) (%s:%s)",
    // short_name,
    //          long_name);
    return -4;
  }

  int short_length = strlen(short_name);

  if (short_length != 2 || short_name[0] != '-' || !isalpha(short_name[1])) {
    // LOG_ERROR("invalid optional arg(%s:%s)", short_name, long_name);
    return -2;
  } else if (short_length == 2 && long_name == NULL) {
    // For optional argument with short_name as key.
    return 2;
  } else if (short_length == 2 && strncmp(long_name, "--", 2) == 0) {
    // For optional argument with long_name as key.
    return 3;
  } else if (short_name[0] == '-' && isalpha(short_name[1]) &&
             long_name[0] != '-' && strncmp(long_name, "--", 2) != 0) {
    /// LOG_ERROR("cannot mix positional and optional args (%s:%s)", short_name,
    //              long_name);
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

/**
 * Concatinate command line arguments into a string.
 *
 * @param argc argument count.
 * @param argv all command line arguments passed in.
 * @param str buffer used to store the final string.
 *
 * @return 0 on success,
 *         1 indicates failure to build.
 *         2 indicates memory allocation failed, str set to NULL.
 */
static int concat_argv(int argc, char *argv[], char **str) {
  int result = STATUS_SUCCESS;
  string_builder *sb;

  if ((string_builder_create(&sb)) != 0) {
    *str = NULL;
    RETURN_DEFER(STATUS_MEMORY_FAILURE);
  }

  for (int i = 1; i < argc; i++) {
    if ((string_builder_append(sb, argv[i], strlen(argv[i]))) != 0) {
      RETURN_DEFER(STATUS_FAILURE);
    }
    if (i != argc - 1) {
      // Add space after each arg except the last one.
      string_builder_append_char(sb, ' ');
    }
  }

  result = string_builder_build(sb, str);

defer:
  if (sb != NULL) {
    string_builder_destroy(&sb);
  }
  return result;
}

/**
 * Add argument flags to hash table.
 *
 * @param parser argparser
 * @param flags hash table to modify.
 *
 * @return 0 on success, 2 indicates memory allocation failed.
 */
static int separate_opt_args(argparser *parser, hash_table **flags) {
  int result = STATUS_SUCCESS;

  if (((result = hash_table_create(flags, sizeof(char *), NULL, NULL)) != 0)) {
    RETURN_DEFER(result);
  }

  string_slice *ss = NULL;
  string_slice *output = NULL;
  char *opt_args = NULL;

  string_builder_build(parser->optional_args, &opt_args);

  if (((result = string_slice_create(&ss, opt_args, strlen(opt_args))) != 0) ||
      (result = string_slice_create(&output, NULL, 0)) != 0) {
    RETURN_DEFER(result);
  }

  string_slice *flag_slice = NULL;
  string_slice *name_slice = NULL;
  char *flag = NULL;
  char *name = NULL;
  char *opt_str = NULL;

  string_slice_trim(ss);

  while (string_slice_split(ss, output, ' ') == 0) {
    string_slice_to_string(output, &opt_str);
    string_slice_create(&name_slice, opt_str, strlen(opt_str));
    string_slice_create(&flag_slice, NULL, 0);
    string_slice_split(name_slice, flag_slice, ',');
    string_slice_to_string(flag_slice, &flag);
    string_slice_to_string(name_slice, &name);

    if (strncmp(flag, "-0", 2) != 0 && strncmp(name, "--0", 3) == 0) {
      // flag found.
      hash_table_insert(*flags, flag, "--0");
    } else if (strncmp(name, "--0", 3) != 0 && strncmp(flag, "-0", 2) == 0) {
      // name found.
    } else {
      // flag and name are defined.
      hash_table_insert(*flags, flag, name);
    }

    string_slice_destroy(&flag_slice);
    string_slice_destroy(&name_slice);
    FREE(flag);
    flag = NULL;
    FREE(name);
    name = NULL;
    FREE(opt_str);
    opt_str = NULL;
  }

  string_slice_destroy(&ss);
  string_slice_destroy(&output);
  FREE(opt_args);

defer:
  return result;
}

/**
 * Add positional arguments to dynamic array.
 *
 * @param parser argparser
 * @param pos_args dynamic array to modify.
 *
 * @return 0 on success, 2 indicates memory allocation failed.
 */
static int separate_pos_args(argparser *parser, dynamic_array **pos_args) {
  int result = STATUS_SUCCESS;

  if ((result = dynamic_array_create(pos_args, sizeof(char *), NULL, NULL))) {
    RETURN_DEFER(result);
  }

  char *pos_args_str = NULL;
  string_slice *ss = NULL;
  string_slice *output = NULL;
  char *name = NULL;

  string_builder_build(parser->positional_args, &pos_args_str);

  if (((result = string_slice_create(&ss, pos_args_str,
                                     strlen(pos_args_str))) != 0) ||
      (result = string_slice_create(&output, NULL, 0)) != 0) {
    RETURN_DEFER(result);
  }

  string_slice_trim(ss);
  while (string_slice_split(ss, output, ' ') == 0) {
    string_slice_to_string(output, &name);

    dynamic_array_add(*pos_args, name);
    FREE(name);
    name = NULL;
  }

  string_slice_destroy(&ss);
  string_slice_destroy(&output);
  FREE(pos_args_str);

defer:
  return result;
}

/**
 * Deallocate memory for string.
 */
static void destroy_str(void **str) {
  FREE(str);
  str = NULL;
}

/**
 * Append error message to parser errors array.
 *
 * @param parser argparser
 * @param arg argparser_argument
 * @param flag_or_name
 * @param opt_type number indicating the argument type,
 *                 0 for no argument type,
 *                 1 for argument flag,
 *                 2 for argument name.
 *
 * @return 0 on success, 2 indicates memory allocation failed.
 */
static int add_error_to_parser(argparser *parser, argparser_argument *arg,
                               char *flag_or_name, unsigned int opt_type) {
  int result = STATUS_SUCCESS;
  string_builder *sb = NULL;
  char *message = NULL;

  if ((result = string_builder_create(&sb)) != 0) {
    RETURN_DEFER(result);
  }

  if (arg != NULL && flag_or_name == NULL) {
    // Use argument to format error message.
    if (arg->short_name != NULL) {
      string_builder_append_fmtstr(sb, "error: argument %s", arg->short_name);
    }

    if (arg->short_name != NULL && arg->long_name != NULL) {
      string_builder_append_char(sb, '/');
    }

    if (arg->long_name != NULL) {
      string_builder_append(sb, arg->long_name, strlen(arg->long_name));
    }

    string_builder_append(sb, " expected one argument", 21);

  } else if (arg == NULL && flag_or_name != NULL && opt_type == 1) {
    // Use argument flag to format error message.
    string_builder_append_fmtstr(
        sb, "error: argument -%c expected one argument", flag_or_name[0]);
  } else if (arg == NULL && flag_or_name != NULL && opt_type == 2) {
    // Use argument name to format error message.
    string_builder_append_fmtstr(
        sb, "error: argument %s expected one argument\n", flag_or_name);
  }

  string_builder_build(sb, &message);

  if (parser->errors == NULL) {
    if (dynamic_array_create(&parser->errors, sizeof(char *), destroy_str,
                             NULL) != 0) {
      RETURN_DEFER(result);
    }
  }

  if (dynamic_array_add_str(parser->errors, message) != 0) {
    RETURN_DEFER(result);
  }

defer:
  if (sb != NULL) {
    string_builder_destroy(&sb);
  }

  return result;
}

/**
 * Validate the argument.
 *
 * @param parg argparser_argument to validate.
 * @param args_str pointer to the start position of argument.
 * @param index pointer to the current position of args_str.
 *
 * @return how much to move forward.
 */
static int validate_argument(argparser *parser, argparser_argument *arg,
                             char *args_str, unsigned short index) {
  switch (arg->action) {
    case AP_ARG_STORE_APPEND:
      break;
    case AP_ARG_STORE: {
      string_slice *value_slice = NULL;
      int ss_length = 0;

      if (args_str[index] == ' ') {
        // Skip whitespace between argument flag and value.
        index++;
      }

      string_slice_create(&value_slice, args_str + index, 0);

      while (args_str[index] != ' ') {
        if (*(args_str + index) == '\0' && ss_length == 0) {
          add_error_to_parser(parser, arg, NULL, 0);
          break;
        }

        if (*(args_str + index) == '\0') {
          break;
        }

        // Construct value string.
        string_slice_advance(&value_slice);
        index++;
        ss_length++;
      }

      if (arg->value != NULL) {
        // Deallocate previous value
        FREE(arg->value);
      }
      string_slice_to_string(value_slice, (char **)&arg->value);
      string_slice_destroy(&value_slice);
      break;
    }
    case AP_ARG_STORE_CONST:
      break;
    case AP_ARG_STORE_APPEND_CONST:
      break;
    case AP_ARG_STORE_EXTEND:
      break;
    case AP_ARG_STORE_COUNT:
      break;
    case AP_ARG_STORE_TRUE:
      break;
    case AP_ARG_STORE_FALSE:
      break;
    case AP_ARG_STORE_VERSION:
      break;
  }

  return index;
}

/**
 * Loop through the current argument string position until a space is
 * encountered.
 *
 * @param args_str position of the current argument string.
 * @param index position of args_str.
 * @param name buffer to store the name of the argument.
 *
 * @return 0 on success,
 *         2 indicates failure to allocation memory for 'str',
 *         4 indicates index is out of bounds.
 */
static int get_arg_name(char *args_str, unsigned int index, char **name) {
  int result = STATUS_SUCCESS;
  string_slice *ss = NULL;

  if (((result = string_slice_create(&ss, args_str + index, 0)) != 0)) {
    RETURN_DEFER(result);
  }

  unsigned int args_str_length = strlen(args_str);
  while (args_str[index] != ' ') {
    if (index >= args_str_length) {
      string_slice_to_string(ss, name);
      RETURN_DEFER(STATUS_OUT_OF_BOUNDS);
    }
    string_slice_advance(&ss);
    index++;
  }

  if ((result = string_slice_to_string(ss, name) != 0)) {
    RETURN_DEFER(result);
  }

  string_slice_destroy(&ss);

defer:
  if (ss != NULL) {
    string_slice_destroy(&ss);
  }

  return result;
}

/**
 * Parse the positional argument.
 *
 * @param parser argparser
 * @param args_str pointer to the start position of argument.
 * @param index pointer to the arguments string.
 * @param args_num number of positional argument being processed.
 *
 * @return how much to move forward.
 */
static int parse_positional_argument(argparser *parser, char *args_str,
                                     unsigned short index,
                                     unsigned int args_num,
                                     dynamic_array *pos_args) {
  int result = 0;
  argparser_argument *arg = NULL;
  char *name = NULL;

  dynamic_array_find_ref(pos_args, args_num - 1, (void **)&name);

  if (name != NULL) {
    hash_table_search(parser->arguments, name, (void **)&arg);
    index = validate_argument(parser, arg, args_str, index);
    RETURN_DEFER(index);
  }

  get_arg_name(args_str, index, &name);

  index += strlen(name);

  // Only allocate memory if unrecognized argument has been detected.
  if (parser->unrecognized_args == NULL) {
    string_builder_create(&parser->unrecognized_args);
  }

  if (arg == NULL && name != NULL) {
    string_builder_append(parser->unrecognized_args, name, strlen(name));
    string_builder_append_char(parser->unrecognized_args, ' ');
    FREE(name);
    RETURN_DEFER(index);
  }

defer:
  return result;
}

/**
 * Verify that argument flag is a valid.
 *
 * @param parser argparser
 * @param flags hash table with flag/long name.
 * @param flag Argument flag to check.
 *
 * @return 0 on success, 1 otherwise.
 */
static int is_valid_arg_flag(argparser *parser, hash_table *flags, char flag) {
  int result = STATUS_FAILURE;
  char flag_str[3];
  sprintf(flag_str, "-%c", flag);
  void *long_name = NULL;

  if (hash_table_search(flags, flag_str, &long_name) == 0 &&
      strncmp(long_name, "--0", 3) != 0) {
    if (hash_table_search(parser->arguments, long_name, NULL) == 0) {
      RETURN_DEFER(STATUS_SUCCESS);
    }
  } else {
    if (hash_table_search(parser->arguments, flag_str, NULL) == 0) {
      RETURN_DEFER(STATUS_SUCCESS);
    }
  }

defer:
  return result;
}

/**
 * Verify that name is a valid argument name.
 *
 * @param parser argparser
 * @param name Argument name to check.
 *
 * @return 0 on success, 1 otherwise.
 */
static int is_valid_arg_name(argparser *parser, char *name) {
  int result = STATUS_FAILURE;

  if (hash_table_search(parser->arguments, name, NULL) == 0) {
    RETURN_DEFER(STATUS_SUCCESS);
  }

defer:
  return result;
}

/**
 * Parse the optional argument.
 *
 * @param parser argparser
 * @param args_str pointer to the start position of argument.
 * @param arg_kind type of argument,
 *                 1 is positional,
 *                 2 is optional with flag,
 *                 3 is optional with name.
 *
 * @return how much to move forward.
 */
static int parse_optional_argument(argparser *parser, char *args_str,
                                   arg_kind kind, unsigned int index,
                                   hash_table *flags,
                                   unsigned int args_str_length) {
  int result = 0;
  argparser_argument *arg = NULL;
  char *name = NULL;
  char *value = NULL;

  switch (kind) {
    case ARG_KIND_OPT_FLAG: {
      char concat_str[3];
      sprintf(concat_str, "-%c", args_str[index]);

      // Argument flag value cannot be an argument flag.
      if (index + 1 < args_str_length &&
          is_valid_arg_flag(parser, flags, args_str[index]) == 0 &&
          is_valid_arg_flag(parser, flags, args_str[index + 1]) == 0) {
        add_error_to_parser(parser, NULL, args_str + index++, 1);
        RETURN_DEFER(index);
      }

      int found = hash_table_search(flags, concat_str, (void **)&value);
      if (found == 0 && value != NULL && strncmp(value, "--0", 3) != 0) {
        // use name as key to access argument.
        hash_table_search(parser->arguments, value, (void **)&arg);
        index = validate_argument(parser, arg, args_str, index + 1);
        RETURN_DEFER(index);
      } else if (found == 0 && strncmp(value, "--0", 3) == 0) {
        // use flag as key to access argument.
        hash_table_search(parser->arguments, concat_str, (void **)&arg);
        index = validate_argument(parser, arg, args_str, index + 1);
        RETURN_DEFER(index);
      }

      // Only allocate memory if unrecognized argument has been detected.
      if (parser->unrecognized_args == NULL) {
        string_builder_create(&parser->unrecognized_args);
      }

      if (arg == NULL) {
        string_builder_append_char(parser->unrecognized_args, '-');
        string_builder_append_char(parser->unrecognized_args, args_str[index]);
        string_builder_append_char(parser->unrecognized_args, ' ');
      }

      RETURN_DEFER(++index);
      break;
    }
    case ARG_KIND_OPT_NAME: {
      if (get_arg_name(args_str, index, &name) == STATUS_OUT_OF_BOUNDS) {
        add_error_to_parser(parser, NULL, name, 2);
        RETURN_DEFER(strlen(args_str));
      }

      int name_length = strlen(name);

      if (hash_table_search(parser->arguments, name, (void **)&arg) == 0) {
        // use name as key to access argument.
        index = validate_argument(parser, arg, args_str, index + name_length);
        RETURN_DEFER(index);
      }

      // Only allocate memory if unrecognized argument has been detected.
      if (parser->unrecognized_args == NULL) {
        string_builder_create(&parser->unrecognized_args);
      }

      if (arg == NULL) {
        string_builder_append(parser->unrecognized_args, name, name_length);
        string_builder_append_char(parser->unrecognized_args, ' ');
        index += name_length - 2;  // exclude double dashes(--)
        RETURN_DEFER(index);
      }

      RETURN_DEFER(++index);
      break;
    }
    default:
      LOG_ERROR("Should NOT print this message.");
  }

defer:
  if (name != NULL) {
    FREE(name);
  }

  return result;
}

int argparser_create(argparser **parser) {
  int result = STATUS_SUCCESS;

  if ((*parser = MALLOC(sizeof(argparser))) == NULL) {
    LOG_ERROR("failed to allocated memory for argparser");
    RETURN_DEFER(STATUS_MEMORY_FAILURE);
  }

  if ((result =
           hash_table_create(&(*parser)->arguments, sizeof(argparser_argument),
                             NULL, arg_destroy)) != 0) {
    RETURN_DEFER(result);
  }

  if ((result = string_builder_create(&(*parser)->positional_args)) != 0) {
    RETURN_DEFER(result);
  }

  if ((result = string_builder_create(&(*parser)->optional_args)) != 0) {
    RETURN_DEFER(result);
  }

  if ((result = string_builder_create(&(*parser)->req_opt_args)) != 0) {
    RETURN_DEFER(result);
  }

  (*parser)->name = NULL;
  (*parser)->usage = NULL;
  (*parser)->description = NULL;
  (*parser)->epilogue = NULL;
  (*parser)->prefix_chars = NULL;
  (*parser)->add_help = true;
  (*parser)->allow_abbrev = true;
  (*parser)->pos_args_size = 0;
  (*parser)->req_opt_args_size = 0;
  (*parser)->unrecognized_args = NULL;
  (*parser)->errors = NULL;

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

int argparser_add_help_to_argparser(argparser **parser, bool add_help) {
  int result = STATUS_SUCCESS;

  if (add_help != 0 && add_help != 1) {
    RETURN_DEFER(STATUS_FAILURE);
  }

  (*parser)->add_help = add_help;

defer:
  return result;
}

int argparser_add_abbrev_to_argparser(argparser **parser, bool allow_abbrev) {
  int result = STATUS_SUCCESS;

  if (allow_abbrev != 0 && allow_abbrev != 1) {
    RETURN_DEFER(STATUS_FAILURE);
  }

  (*parser)->allow_abbrev = allow_abbrev;
defer:
  return result;
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

    string_builder_append(parser->positional_args, long_name,
                          strlen(long_name));
    string_builder_append_char(parser->positional_args, ' ');

    if ((arg_create(&arg, NULL, long_name) != 0)) {
      RETURN_DEFER(STATUS_MEMORY_FAILURE);
    }

    hash_table_insert(parser->arguments, long_name, arg);

    parser->pos_args_size++;
  } else if (arg_kind == 2) {
    // Optional argument with short_name as key.

    search_result = hash_table_search(parser->arguments, short_name, &value);
    if (search_result == 0 && (value == NULL || value != NULL)) {
      // There already exists an entry if value is not NULL.
      // hash table is empty if value is NULL.
      // either case exit.
      RETURN_DEFER(6);
    }

    string_builder_append(parser->optional_args, short_name, 2);
    string_builder_append_char(parser->optional_args, ',');
    if (long_name) {
      string_builder_append(parser->optional_args, long_name,
                            strlen(long_name));
    } else {
      string_builder_append(parser->optional_args, "--0", 3);
    }
    string_builder_append_char(parser->optional_args, ' ');

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

    if (short_name) {
      string_builder_append(parser->optional_args, short_name, 2);
      string_builder_append_char(parser->optional_args, ',');
    } else {
      string_builder_append(parser->optional_args, "-0,", 3);
    }

    string_builder_append(parser->optional_args, long_name, strlen(long_name));
    string_builder_append_char(parser->optional_args, ' ');

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
  argparser_arg_action action_value;

  GET_ARG_FROM_PARSER(parser->arguments, name_or_flag, arg);

  // TODO: malloc here

  switch (action) {
    case AP_ARG_STORE:
      action_value = AP_ARG_STORE;
      break;
    case AP_ARG_STORE_CONST:
      action_value = AP_ARG_STORE_CONST;
      break;
    case AP_ARG_STORE_TRUE:
      action_value = AP_ARG_STORE_TRUE;
      break;
    case AP_ARG_STORE_FALSE:
      action_value = AP_ARG_STORE_FALSE;
      break;
    case AP_ARG_STORE_APPEND:
      action_value = AP_ARG_STORE_APPEND;
      break;
    case AP_ARG_STORE_APPEND_CONST:
      action_value = AP_ARG_STORE_APPEND_CONST;
      break;
    case AP_ARG_STORE_EXTEND:
      action_value = AP_ARG_STORE_EXTEND;
      break;
    case AP_ARG_STORE_COUNT:
      action_value = AP_ARG_STORE_COUNT;
      break;
    case AP_ARG_STORE_VERSION:
      action_value = AP_ARG_STORE_VERSION;
      break;
    default:
      RETURN_DEFER(STATUS_FAILURE);
  }
  arg->action = action_value;

defer:
  return result;
}

int argparser_add_type_to_arg(argparser *parser, char *name_or_flag,
                              argparser_arg_type type) {
  int result = STATUS_SUCCESS;
  argparser_argument *arg = NULL;
  argparser_arg_type type_value;

  GET_ARG_FROM_PARSER(parser->arguments, name_or_flag, arg);

  switch (type) {
    case AP_ARG_FLOAT:
      type_value = AP_ARG_FLOAT;
      break;
    case AP_ARG_INT:
      type_value = AP_ARG_INT;
      break;
    case AP_ARG_STRING:
      type_value = AP_ARG_STRING;
      break;
    case AP_ARG_BOOL:
      type_value = AP_ARG_BOOL;
      break;
    default:
      RETURN_DEFER(STATUS_FAILURE);
  }

  // TODO: malloc here
  arg->type = type_value;

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

  if (required != 0 && required != 1) {
    RETURN_DEFER(6);
  }

  GET_ARG_FROM_PARSER(parser->arguments, name_or_flag, arg);

  int bytes = 0;
  char *flag;
  char *name;

  if (arg->short_name == NULL) {
    flag = "-0";
  } else {
    flag = arg->short_name;
  }
  bytes += 2;

  if (arg->long_name == NULL) {
    name = "--0\0";
    bytes += 3;
  } else {
    name = arg->long_name;
    bytes += strlen(arg->long_name);
  }

  char *concat_str = MALLOC(bytes);

  // TODO: make sure this isn't going to add null character.
  snprintf(concat_str, bytes, "%s,%s", flag, name);

  if ((result = string_builder_append(parser->req_opt_args, concat_str,
                                      strlen(concat_str))) != 0) {
    RETURN_DEFER(result);
  }

  parser->req_opt_args_size++;

  // TODO: malloc here
  arg->required = required;

defer:
  if (concat_str != NULL) {
    FREE(concat_str);
  }
  return result;
}

int argparser_add_deprecated_to_arg(argparser *parser, char *name_or_flag,
                                    bool deprecated) {
  int result = STATUS_SUCCESS;
  argparser_argument *arg = NULL;

  if (deprecated != 0 && deprecated != 1) {
    RETURN_DEFER(6);
  }

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

int argparser_parse_args(argparser *parser, int argc, char *argv[]) {
  int result = STATUS_SUCCESS;
  char *args_str = NULL;
  unsigned int current_pos_count = 0;
  hash_table *flags = NULL;
  dynamic_array *pos_args = NULL;

  if ((result = concat_argv(argc, argv, &args_str)) != 0) {
    RETURN_DEFER(result);
  }

  if ((result = separate_opt_args(parser, &flags)) != 0) {
    RETURN_DEFER(result);
  }

  if ((result = separate_pos_args(parser, &pos_args)) != 0) {
    RETURN_DEFER(result);
  }

  int args_length = strlen(args_str);
  for (int i = 0; i < args_length; i++) {
    if ((strncmp(args_str + i, "--", 2)) == 0) {
      // Optional name argument.
      char *name = NULL;
      get_arg_name(args_str, i, &name);
      int name_length = strlen(name);

      if (is_valid_arg_name(parser, name) == 0 &&
          i + name_length + 1 < args_length &&
          args_str[i + name_length + 1] == '-') {
        // Argument name value cannot conflict with another argument flag.
        add_error_to_parser(parser, NULL, name, 2);
        i += name_length;
        FREE(name);
        continue;
      }

      FREE(name);

      i = parse_optional_argument(parser, args_str, ARG_KIND_OPT_NAME, i, flags,
                                  args_length);
    } else if (args_str[i] == '-') {
      // Optional flag argument.
      while (args_str[i] != ' ' && i < args_length) {
        // Argument flag value cannot conflict with another argument's name.
        if (i + 2 < args_length && strncmp(args_str + i + 2, "--", 2) == 0) {
          add_error_to_parser(parser, NULL, args_str + i, 1);
          i++;
          break;
        }

        if (args_str[i] == '-') {
          i = parse_optional_argument(parser, args_str, ARG_KIND_OPT_FLAG,
                                      i + 1, flags, args_length);
        } else {
          i = parse_optional_argument(parser, args_str, ARG_KIND_OPT_FLAG, i,
                                      flags, args_length);
        }
      }
    } else {
      // Positional argument.
      i = parse_positional_argument(parser, args_str, i, ++current_pos_count,
                                    pos_args);
    }
  }

  if (parser->unrecognized_args != NULL) {
    char *args = NULL;

    string_builder_build(parser->unrecognized_args, &args);
    LOG_ERROR("unrecognized argument(s): %s", args);
    FREE(args);
  }

defer:
  if (flags != NULL) {
    hash_table_destroy(&flags);
  }

  if (pos_args != NULL) {
    dynamic_array_destroy(&pos_args);
  }

  if (args_str != NULL) {
    FREE(args_str);
  }

  return result;
}

void argparser_destroy(argparser **parser) {
  if (*parser != NULL) {
    hash_table_destroy(&(*parser)->arguments);
    string_builder_destroy(&(*parser)->positional_args);
    string_builder_destroy(&(*parser)->optional_args);
    string_builder_destroy(&(*parser)->req_opt_args);

    if ((*parser)->unrecognized_args != NULL) {
      string_builder_destroy(&(*parser)->unrecognized_args);
    }

    if ((*parser)->errors != NULL) {
      dynamic_array_destroy(&(*parser)->errors);
    }

    FREE(*parser);
    *parser = NULL;
  }
}
