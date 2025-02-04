#include <stdio.h>
#include <stdlib.h>

#include "argparser.h"
#include "utils.h"

#define ARG_SHORT_HELP "-h"
#define ARG_LONG_HELP "--help"
#define ARG_LONG_FORCE "--force"
#define ARG_SHORT_APPEND "-A"
#define ARG_POS_SRC "src"
#define ARG_POS_DEST "dest"
#define ARG_SHORT_COPY "-c"
#define ARG_LONG_COPY "--copy"

int main(int argc, char *argv[]) {
  int result = STATUS_SUCCESS;
  argparser *parser = NULL;

  argparser_create(&parser);
  argparser_add_name_to_argparser(&parser, "test");
  argparser_add_usage_to_argparser(&parser, "best usage");
  argparser_add_desc_to_argparser(&parser, "best description");
  argparser_add_epilogue_to_argparser(&parser, "best epilogue");
  argparser_add_prechars_to_argparser(&parser, "-+");
  argparser_add_help_to_argparser(&parser, false);
  argparser_add_abbrev_to_argparser(&parser, false);

  argparser_add_argument(parser, NULL, ARG_LONG_FORCE);  // valid
  // argparser_add_action_to_arg(parser, ARG_LONG_FORCE, AP_ARG_STORE_CONST);
  argparser_add_type_to_arg(parser, ARG_LONG_FORCE, AP_ARG_INT);
  argparser_add_help_to_arg(parser, ARG_LONG_FORCE, "This is the help message");
  argparser_add_required_to_arg(parser, ARG_LONG_FORCE, true);
  argparser_add_deprecated_to_arg(parser, ARG_LONG_FORCE, true);
  argparser_add_dest_to_arg(parser, ARG_LONG_FORCE, "FORCE");
  argparser_add_nargs_to_arg(parser, ARG_LONG_FORCE, "2");
  argparser_add_metavar_to_arg(parser, ARG_LONG_FORCE, "H");
  argparser_add_default_value_to_arg(parser, ARG_LONG_FORCE, "DV");
  argparser_add_const_value_to_arg(parser, ARG_LONG_FORCE, "CV");
  argparser_add_choices_to_arg(parser, ARG_LONG_FORCE, "0,1");

  argparser_add_argument(parser, ARG_SHORT_HELP, ARG_LONG_HELP);  // valid
  argparser_add_argument(parser, NULL, ARG_POS_SRC);              // valid
  argparser_add_argument(parser, NULL, ARG_POS_DEST);             // valid
  argparser_add_argument(parser, ARG_SHORT_APPEND, NULL);         // valid
  argparser_add_argument(parser, ARG_SHORT_COPY, ARG_LONG_COPY);  // valid

  argparser_add_argument(parser, NULL, NULL);         // not valid
  argparser_add_argument(parser, NULL, "--force");    // not valid (dublicate)
  argparser_add_argument(parser, NULL, "-export");    // not valid
  argparser_add_argument(parser, "n", "name");        // not valid
  argparser_add_argument(parser, "-t", "terminate");  // not valid
  argparser_add_argument(parser, "-ww", "www");       // not valid
  argparser_add_argument(parser, "!E", "--extra");    // not valid

  argparser_add_argument(parser, "-E", "--extend");  // valid
  argparser_add_argument(parser, "-Z", NULL);        // valid
  argparser_add_argument(parser, "-B", NULL);        // valid

  argparser_add_argument(parser, "-E", "--extend");  // not valid (dublicate)
  argparser_add_argument(parser, NULL, ARG_POS_SRC);  // not valid (dublicate)
  argparser_add_argument(parser, "-Z", NULL);        // not valid (dublicate)

  if ((argparser_parse_args(parser, argc, argv)) != 0) {
    RETURN_DEFER(STATUS_FAILURE);
  }

defer:
  if (parser != NULL) {
    argparser_destroy(&parser);
  }

  return result;
}
