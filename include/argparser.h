#ifndef ARGPARSER_H
#define ARGPARSER_H

#include <stdbool.h>

// Optional single input value.
#define AP_ARG_NARGS_OPTIONAL "?"
// Takes zero or more inputs.
// Values are stored in an array.
#define AP_ARG_NARGS_ZERO_OR_MORE "*"
// Takes one or more inputs.
// Values are stored in an array.
#define AP_ARG_NARGS_ONE_OR_MORE "+"
// Takes all remaining values from the command line.
// Values are stored in an array.
#define AP_ARG_NARGS_REMAINDER "!"

// Parser argument type to convert to
typedef enum argparser_arg_type {
  AP_ARG_TYPE_FLOAT,   // Parser convert to float.
  AP_ARG_TYPE_INT,     // Parser convert to integer.
  AP_ARG_TYPE_STRING,  // Same as not setting the type arg.
  AP_ARG_TYPE_BOOL,    // Parser convert to 0 false 1 true.
} argparser_arg_type;

typedef struct argparser argparser;
// Describes how to store the argument's value(s)
// NOTE: 'store', 'append' or extend' can ONLY be used with positional args.
typedef enum argparser_arg_action {
  AP_ARG_ACTION_STORE,  // Store the argument's value. default action
  AP_ARG_ACTION_STORE_CONST,   // Store value stored in const parameter.
  AP_ARG_ACTION_STORE_TRUE,    // Store true if optional arg is passed.
  AP_ARG_ACTION_STORE_FALSE,   // Store true if optional arg is passed.
  AP_ARG_ACTION_STORE_APPEND,  // Stores values in an array.
  AP_ARG_ACTION_STORE_APPEND_CONST,  // Store value in const parameter.
  AAP_ARG_ACTION_STORE_EXTEND,  // Store values from same arg passed.
  AP_ARG_ACTION_STORE_COUNT,  // Store the amount of times arg is passed.
  AP_ARG_ACTION_STORE_VERSION,  // Print version of program and exit.
} argparser_arg_action;

/**
 * Allocate necessary resources and setup.
 *
 * @param parser argparser to create.
 * @param name Program name.
 * @param usage Text that informs about how to use the program.
 * @param description Text that informs about the purpose of the program.
 * @param epilogue Text to add after help messsage.
 * @param prefix_chars Change the symbol that precede optional argument.
 * @param add_help Add -h/--help optional argument to the parser.
 * @param allow_abbrev Allow abbreviations of long args name.
 *
 * @return 0 on success, 2 indicates memory allocation failed.
 */
int argparser_create(argparser **parser);

/**
 * Add name to argparser.
 *
 * @param parser argparser to modify.
 * @param name The name for the program.
 *
 * @return 0 on success,
 *         1 indicates name is empty,
 *         2 indicates memory allocation failed.
 */
int argparser_add_name_to_argparser(argparser **parser, char *name);

/**
 * Add usage to argparser.
 *
 * @param parser argparser to modify.
 * @param usage Text descibing how to use the program.
 *
 * @return 0 on success,
 *         1 indicates usage is empty,
 *         2 indicates memory allocation failed.
 */
int argparser_add_usage_to_argparser(argparser **parser, char *usage);

/**
 * Add description to argparser.
 *
 * @param parser argparser to modify.
 * @param desc Text descibing the program.
 *
 * @return 0 on success,
 *         1 indicates usage is empty,
 *         2 indicates memory allocation failed.
 */
int argparser_add_desc_to_argparser(argparser **parser, char *usage);

/**
 * Add epilogue to argparser.
 *
 * @param parser argparser to modify.
 * @param epilogue Text after the help message.
 *
 * @return 0 on success,
 *         1 indicates usage is empty,
 *         2 indicates memory allocation failed.
 */
int argparser_add_epilogue_to_argparser(argparser **parser, char *epilogue);

/**
 * Add prefix characters to argparser.
 *
 * Customize how optional arguments should be passed in.
 * Set prechars to "+" to pass optinal arguments
 * in the form of +h/+help instread of -h/--help.
 *
 * @param parser argparser to modify.
 * @param prechars characters that prefix optinal arguments. Default(-)
 *
 * @return 0 on success, 2 indicates memory allocation failed.
 */
int argparser_add_prechars_to_argparser(argparser **parser, char *prefix_chars);

/**
 * Add help to argparser.
 *
 * @param parser argparser to modify.
 * @param help -h/--help message if true by default.
 */
void argparser_add_help_to_argparser(argparser **parser, bool epilogue);

/**
 * Add allow abbrev to argparser.
 *
 * @param parser argparser to modify.
 * @param allow_abbrev Allow abbreviations of long optional arguments.
 *                     true by default.
 */
void argparser_add_abbrev_to_argparser(argparser **parser, bool allow_abbrev);

/**
 * Add argument to the parser.
 *
 * Determines whether to add a positional or optional argument.
 * To add Positional argument:
 *   'short_name' MUST be set to NULL,
 *   'long_name' MUST exclude a double dash(--).
 * To add Optional argument:
 *   'short_name' MUST start with a single dash(-) followed by single
 *                alphanumeric character.
 *   'long_name' MUST start with double dashes(--).
 *
 * @param parser argparser to modify.
 * @param short_name character(after '-') name for optional argument.
 * @param long_name name for positional argument.
 *
 * @return 0 on success,
 *         1 indicates argument formatting error,
 *         2 indicates failed to allocate memory for argument,
 *         5 indicates parser is NULL.
 *         6 indicates argument already exists.
 */
int argparser_add_argument(argparser *parser, char short_name[2],
                           char *long_name);

/**
 * Add action to parser argument.
 *
 * @param parser argparser to modify.
 * @param name_or_flag For positional arguments use 'long_name'.
 *                     For optional arguments, use 'short_name' IF 'long_name'
 *                     is NULL, and use 'long_name' if both 'short_name' and
 *                     'long_name' are defined.
 * @param action argparser_arg_action that determines how args are handled.
 *        available action are:
 *
 * AP_ARG_ACTION_STORE,  // Store the argument's value. default action
 * AP_ARG_ACTION_STORE_CONST,   // Store value stored in const parameter.
 * AP_ARG_ACTION_STORE_TRUE,    // Store true if optional arg is passed.
 * AP_ARG_ACTION_STORE_FALSE,   // Store true if optional arg is passed.
 * AP_ARG_ACTION_STORE_APPEND,  // Stores values in an array.
 * AP_ARG_ACTION_STORE_APPEND_CONST,  // Store value in const parameter.
 * AP_ARG_ACTION_STORE_EXTEND,  // Store values from same arg passed.
 * AP_ARG_ACTION_STORE_COUNT,  // Store the amount of times arg is
 *                                       passed.
 * ARGPARSER_ARG_ACTION_STORE_VERSION,  // Print version of program and exit.
 *
 ** @return 0 on success,
            positive number otherwise.
 */
int argparser_add_action_to_arg(argparser *parser, char *name_or_flag,
                                argparser_arg_action action);

/**
 * Add type to parser argument.
 *
 * @param parser argparser to modify.
 * @param name_or_flag For positional arguments use 'long_name'.
 *                     For optional arguments, use 'short_name' IF 'long_name'
 *                     is NULL, and use 'long_name' if both 'short_name' and
 *                     'long_name' are defined.
 * @param type argparser_arg_type that determines what type to convert to.
 *
 * @return 0 on success, positive number otherwise.
 */
int argparser_add_type_to_arg(argparser *parser, char *name_or_flag,
                              argparser_arg_type type);

/**
 * Add help to parser argument.
 *
 * @param parser argparser to modify.
 * @param name_or_flag For positional arguments use 'long_name'.
 *                     For optional arguments, use 'short_name' IF 'long_name'
 *                     is NULL, and use 'long_name' if both 'short_name' and
 *                     'long_name' are defined.
 * @param help Text that describes the argument.
 *
 * @return 0 on success, positive number otherwise.
 */
int argparser_add_help_to_arg(argparser *parser, char *name_or_flag,
                              char *help);

/**
 * Add required parameter to parser argument.
 *
 * @param parser argparser to modify.
 * @param name_or_flag For positional arguments use 'long_name'.
 *                     For optional arguments, use 'short_name' IF 'long_name'
 *                     is NULL, and use 'long_name' if both 'short_name' and
 *                     'long_name' are defined.
 * @param required indicates whether the argument is required.
 *
 * @return 0 on success, positive number otherwise.
 */
int argparser_add_required_to_arg(argparser *parser, char *name_or_flag,
                                  bool required);

/**
 * Add deprecated parameter to parser argument.
 *
 * @param parser argparser to modify.
 * @param name_or_flag For positional arguments use 'long_name'.
 *                     For optional arguments, use 'short_name' IF 'long_name'
 *                     is NULL, and use 'long_name' if both 'short_name' and
 *                     'long_name' are defined.
 * @param deprecated indicates the argument is deprecated in the help message.
 *
 * @return 0 on success, positive number otherwise.
 */
int argparser_add_deprecated_to_arg(argparser *parser, char *name_or_flag,
                                    char deprecated);

/**
 * Add dest parameter to parser argument.
 *
 * @param parser argparser to modify.
 * @param name_or_flag For positional arguments use 'long_name'.
 *                     For optional arguments, use 'short_name' IF 'long_name'
 *                     is NULL, and use 'long_name' if both 'short_name' and
 *                     'long_name' are defined.
 * @param dest Custom name to appear on help message.
 *
 * @return 0 on success, positive number otherwise.
 */
int argparser_add_dest_to_arg(argparser *parser, char *name_or_flag,
                              char *dest);

/**
 * Add nargs parameter to parser argument.
 *
 * @param parser argparser to modify.
 * @param name_or_flag For positional arguments use 'long_name'.
 *                     For optional arguments, use 'short_name' IF 'long_name'
 *                     is NULL, and use 'long_name' if both 'short_name' and
 *                     'long_name' are defined.
 * @param nargs Number of args to accept. Works with action.
 *                 '?' single value which can be optional.
 *                 '+' one or more values to store in array.
 *                 '*' zero of more values to stor in array.
 *
 * @return 0 on success, positive number otherwise.
 */
int argparser_add_nargs_to_arg(argparser *parser, char *name_or_flag,
                               char *nargs);

/**
 * Add metavar parameter to parser argument.
 *
 * @param parser argparser to modify.
 * @param name_or_flag For positional arguments use 'long_name'.
 *                     For optional arguments, use 'short_name' IF 'long_name'
 *                     is NULL, and use 'long_name' if both 'short_name' and
 *                     'long_name' are defined.
 * @param metavar Change input value name used in the hlep message.
 *
 * @return 0 on success, positive number otherwise.
 */
int argparser_add_metavar_to_arg(argparser *parser, char *name_or_flag,
                                 char *metavar);

/**
 * Add default_value parameter to parser argument.
 *
 * @param parser argparser to modify.
 * @param name_or_flag For positional arguments use 'long_name'.
 *                     For optional arguments, use 'short_name' IF 'long_name'
 *                     is NULL, and use 'long_name' if both 'short_name' and
 *                     'long_name' are defined.
 * @param default_value Value for argument.
 *
 * @return 0 on success, positive number otherwise.
 */
int argparser_add_default_value_to_arg(argparser *parser, char *name_or_flag,
                                       char *default_value);

/**
 * Add const_value parameter to parser argument.
 *
 * @param parser argparser to modify.
 * @param name_or_flag For positional arguments use 'long_name'.
 *                     For optional arguments, use 'short_name' IF 'long_name'
 *                     is NULL, and use 'long_name' if both 'short_name' and
 *                     'long_name' are defined.
 * @param const_value Value for argument.
 *
 * @return 0 on success, positive number otherwise.
 */
int argparser_add_const_value_to_arg(argparser *parser, char *name_or_flag,
                                     char *const_value);

/**
 * Add choices parameter to parser argument.
 *
 * @param parser argparser to modify.
 * @param name_or_flag For positional arguments use 'long_name'.
 *                     For optional arguments, use 'short_name' IF 'long_name'
 *                     is NULL, and use 'long_name' if both 'short_name' and
 *                     'long_name' are defined.
 * @param choices Valid options for argument.
 *
 * @return 0 on success, positive number otherwise.
 */
int argparser_add_choices_to_arg(argparser *parser, char *name_or_flag,
                                 char *choices);

/**
 * Parse parser arguments.
 *
 * @param parser argparser to parse.
 * @param argc argument count.
 * @param argv array of arguments as strings.
 *
 * @return 0 on success, positive number otherwise.
 */
int argpaser_parse_args(argparser *parser, int argc, char *argv[]);
/**
 * Deallocate and set to NULL.
 *
 * @parser argparser to deallocate.
 */
void argparser_destroy(argparser **parser);

#endif  // ARGPARSER_Hl
