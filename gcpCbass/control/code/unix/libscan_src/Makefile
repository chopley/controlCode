# This file is designed to be invoked from a recursive call to make
# from the makefile in CBI/code/unix/.
#
# The makefile that includes it is expected to define the following
# macros:
#
#  INC       - The full name of the CBI/code/unix/include directory.
#  LIB       - The full name of the CBI/code/unix/lib directory.
#  SHARE     - The full name of the CBI/code/share/include directory.
#  CC        - The C compiler to be used.
#  CFLAGS    - Flags for the C compiler.
#  RANLIB    - The archiver randomizer command (use : if not needed).
#  SYSLIBS   - Default system libraries
#-----------------------------------------------------------------------

#-----------------------------------------------------------------------
# Generic rule for making .o files
#-----------------------------------------------------------------------

%.o: %.c
	$(CC) $(CFLAGS) $(INCFLAGS) -c -o $@ $<

#-----------------------------------------------------------------------
# List the default targets.
#-----------------------------------------------------------------------

LIBSO = $(GCPLIB)/libGcpScan$(LIBSO_SUFFIX)
all: $(LIBSO) $(INC)/scan.h 

clean: clean_o clean_lib clean_depend

#-----------------------------------------------------------------------
# Library generation.
#-----------------------------------------------------------------------

SCAN_OBJ = scan.o

$(LIBSO): depend $(SCAN_OBJ)
	$(CC) $(LIBSO_FLAGS) -o $(LIBSO) $(SCAN_OBJ)
	$(RANLIB) $@

clean_o:
	@ if [ -f scan.o ] ; then /bin/rm -f scan.o ; fi

clean_lib:
	/bin/rm -f $(GCPLIB)/libGcpScan$(LIBSO_SUFFIX)

clean_depend:
	/bin/rm -f Makefile.rules

#-----------------------------------------------------------------------
# Arrange for pertinent include files to be visible in the shared
# include directory.
#-----------------------------------------------------------------------

$(INC)/scan.h: scan.h
	cp scan.h $@


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

