#-----------------------------------------------------------------------
# Makefile for common util library
#
#  @author Erik Leitch
#  @version $Revision: 1.1.1.1 $, $Date: 2009/07/06 23:57:07 $
#
# This file is intended to be called recursively from a parent makefile.
#-----------------------------------------------------------------------

#-----------------------------------------------------------------------
# Generic rule for making .o files
#-----------------------------------------------------------------------

%.o: %.cc
	$(CC) $(CCFLAGS) $(INCFLAGS) -c -o $@ $<

#-----------------------------------------------------------------------
# Targets
#-----------------------------------------------------------------------

all: libs

#-----------------------------------------------------------------------
# Doxygen documentation
#-----------------------------------------------------------------------

# We have to make a phony target so that make will proceed even if
# there is a file/dir under the current directory with the same name

.PHONY : dox

dox:
	@echo ' ';echo 'Making Doxygen documentation'; echo ' '
	doxygen Doxyfile.cfg

clean_dox:
	@echo ' ';echo 'Removing dox directory'; echo ' '
	/bin/rm -rf dox

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

OBJ =  $(patsubst %.cc,%.o,$(wildcard [A-Z]*.cc)) 

# The name of the library to create

LIBSO = $(LIBDIR)/libGcpTransaction$(LIBSO_SUFFIX)

$(LIBSO): $(OBJ)
	echo ' ';echo 'Compiling library: ' $(LIBSO); echo ' '
	g++ $(LIBSO_FLAGS) -o $(LIBSO) $(OBJ)

libs: 	depend $(OBJ)
	@if [ ! -d $(LIBDIR) ] ; then mkdir $(LIBDIR) ; fi ;
	$(MAKE) $(LIBSO); 

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

RULESFILES =  $(wildcard *.cc)

# If Makefile.rules doesn't exist (see above), executing this rule
# will cause it to be made and sourced.

depend:
	@echo 'Executing depend rule'
ifeq ($(RULES_EXISTS),n)
	$(CC) $(CCFLAGS) $(OPTIONS) $(INCFLAGS) -MM $(RULESFILES) > Makefile.rules
	@set -e; [ -s Makefile.rules ] ;
endif

#-----------------------------------------------------------------------
# Clean directives
#-----------------------------------------------------------------------

clean_depend:
	\rm -f Makefile.rules

clean_files:
	\rm -f *.o

clean_libs:
	\rm -f $(LIBSO)

clean: clean_depend clean_files clean_libs

