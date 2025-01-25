BINARY=bin
CODEDIR=src
INCDIR=include
BUILDDIR=build

CC=gcc
OPT=-O0
# generate files that encode make rules for the .h dependencies
DEPFLAGS=-MP -MD
CFLAGS=-Wall -Wextra -Werror -g -I$(INCDIR) $(OPT) $(DEPFLAGS)

# $(wildcard pattern…)
# get a list of files that match the pattern.
HFILES=$(wildcard $(INCDIR)/**/*.h $(INCDIR)/*.h)
CFILES=$(wildcard $(CODEDIR)/**/*.c $(CODEDIR)/*.c)

# $(patsubst pattern, replacement, text)
#  replace 'pattern' with replace 'replacement' in text
OBJECTS=$(patsubst $(CODEDIR)%.c,$(BUILDDIR)%.o,$(CFILES))
DEPFILES=$(patsubst $(CODEDIR)%.c,$(BUILDDIR)%.d,$(CFILES))

all: $(BUILDDIR)/$(BINARY)
	@echo "All Done"

# $@ represents the target ($(BUILDDIR)/$(BINARY)) here.
# $^ represents all the dependencies of a target, excluding duplicates.
# | represents order-only prerequisite. Those that are to the right of |
#   target can be out of date. 
$(BUILDDIR)/$(BINARY): $(OBJECTS) | $(BUILDDIR)
	@echo "Linking -> $@"
	@$(CC) $(CFLAGS) -o $@ $^

# $< represents the first prerequisite of a rule $(CODEDIR)/%.c
$(BUILDDIR)/%.o: $(CODEDIR)/%.c $(HFILES) | $(BUILDDIR)
	@echo "Compiling -> $<"
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c -o $@ $<

$(BUILDDIR):
	@mkdir -p $@

clean:
	@rm -rf $(BUILDDIR) # $(BINARY) $(OBJECTS) $(DEPFILES)
	@echo "All Clean"

# include the dependencies
-include $(DEPFILES)

# add .PHONY so that the non-targetfile - rules work even if a file with the same name exists.
.PHONY: all clean
