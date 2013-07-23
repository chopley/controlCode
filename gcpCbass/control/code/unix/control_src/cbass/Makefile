# This file is designed to be invoked from a recursive call to make
# from the makefile in gcp/code/unix/.
#
# The makefile that includes it is expected to define the following
# macros:
#
#  BIN       - The full name of the gcp/array/control//code/unix/bin directory.
#  INC       - The full name of the gcp/array/control//code/unix/include directory.
#  LIB       - The full name of the gcp/array/control//code/unix/lib directory.
#  SHARE     - The full name of the gcp/array/control//code/share/include directory.
#  CC        - The C compiler to be used.
#  CFLAGS    - Flags for the C compiler.
#  RANLIB    - The archiver randomizer command (use : if not needed).
#-----------------------------------------------------------------------

CONF      = $(GCPTOP)/control/conf
SCRIPT    = $(GCPTOP)/scripts
UNIXDIR   = $(GCPTOP)/control/code/unix/lib
COMMON    = $(GCPTOP)/control/code/unix/control_src/common

INCFLAGS += -I$(COMMON)

#-----------------------------------------------------------------------
# Generic rule for making .o files
#-----------------------------------------------------------------------

%.o: %.c
	$(CC) $(CFLAGS) $(INCFLAGS) -c -o $@ $<

#-----------------------------------------------------------------------
# List the default targets.
#-----------------------------------------------------------------------

all: libs

#-----------------------------------------------------------------------
# Library generation
#-----------------------------------------------------------------------

# List the dependencies by substituting from the source directory.
# This will cause the .o files to be made if they do not already
# exist.  Otherwise if .o files don't already exist, the library will
# be made without complaint with no targets

# In wildcard, we use only files beginning with capitals, since these
# are the class definitions.  Otherwise, threadserver.o and
# threadclient.o would get built into the library

LIB_OBJ =  $(patsubst %.c,%.o,$(wildcard [a-z,A-Z]*.c)) 

# The name of the library to create

LIBSO = $(LIBDIR)/libGcpControlSpecific$(LIBSO_SUFFIX)

$(LIBSO): $(LIB_OBJ)
	echo ' ';echo 'Compiling library: ' $(LIBSO); echo ' '
	g++ $(LIBSO_FLAGS) -o $(LIBSO) $(LIB_OBJ)

libs: 	depend $(LIB_OBJ)
	@if [ ! -d $(LIBDIR) ] ; then mkdir $(LIBDIR) ; fi ;
	$(MAKE) $(LIBSO); 

test:
	@cd Test; $(MAKE)

docs:
	$(SCRIPT)/code2html.py specificscript.c

clean:
	/bin/rm -f *.o
	/bin/rm -f $(GCPCONTROL)

clean_depend:
	\rm -f Makefile.rules

#-----------------------------------------------------------------------
# Include the file in which automatic dependencies are deposited by
# make depend, above.  But only if it exists.  If not, the rule for
# depend, below, will make it.  Don't include Makefile.rules until
# after the desired default target is defined, or Makefile.rules will
# determine the default target for you!
#-----------------------------------------------------------------------

MAKERULES := $(wildcard Makefile.rules)

ifeq ($(MAKERULES), Makefile.rules)
	RULES_EXISTS = y
	include Makefile.rules
else
	RULES_EXISTS = n
endif

#-----------------------------------------------------------------------
# Make dependencies, if they don't already exist
#-----------------------------------------------------------------------

RULESFILES =  $(wildcard *.c)

# If Makefile.rules doesn't exist (see above), executing this rule
# will cause it to be made and sourced.

depend:
	@echo 'Executing depend rule'
ifeq ($(RULES_EXISTS),n)
	$(CC) $(CFLAGS) $(OPTIONS) $(INCFLAGS) -MM $(RULESFILES) > Makefile.rules
	@set -e; [ -s Makefile.rules ] ;
endif
