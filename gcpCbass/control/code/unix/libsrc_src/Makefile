# This file is designed to be invoked from a recursive call to make
# from the makefile in SZA/code/unix/.
#
# The makefile that includes it is expected to define the following
# macros:
#
#  INC       - The full name of the SZA/code/unix/include directory.
#  LIB       - The full name of the SZA/code/unix/lib directory.
#  SHARE     - The full name of the SZA/code/share/include directory.
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

LIBSO = $(GCPLIB)/libGcpSrc$(LIBSO_SUFFIX)
all:  $(LIBSO) $(INC)/source.h 

clean: clean_o clean_readcat clean_lib clean_depend

clean_readcat:
	@ if [ -f readcat.o ] ; then /bin/rm -f readcat.o ; fi
	@ if [ -f readcat ] ; then /bin/rm -f readcat ; fi

#-----------------------------------------------------------------------
# Library generation.
#-----------------------------------------------------------------------

SRC_OBJ = source.o

$(LIBSO): depend $(SRC_OBJ)
	$(CC) $(LIBSO_FLAGS) -o $(LIBSO) $(SRC_OBJ)
	$(RANLIB) $@

clean_depend:
	\rm -f Makefile.rules

clean_o:
	@ if [ -f source.o ] ; then /bin/rm -f source.o ; fi

clean_lib:
	/bin/rm -f $(LIBSO)

#-----------------------------------------------------------------------
# Arrange for pertinent include files to be visible in the shared
# include directory.
#-----------------------------------------------------------------------

$(INC)/source.h: source.h
	cp -p source.h $@


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

