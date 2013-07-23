# Makefile for antenna common code.
#
#  Currently makes the shared CORBA antenna library
#  And a simple server/client application that uses it
#
#  @author Erik Leitch
#  @version $Revision: 1.1.1.1 $, $Date: 2009/07/06 23:57:18 $
#
# This file is intended to be called recursively from a parent makefile.
#
# The environment must define:
#
#  CARMA    -- The root of the common directory
#  CARMALIB -- The carma lib directory
#  ORBHOME  -- Root of the orbacus tree (e.g., /usr/local/orbacus)


OS := $(shell uname)
HOSTNAME  = $(HOST)

GCPLIB =        $(CARMA)/sza/lib
ORBLIB =        $(ORBHOME)/lib
CARMATOOLSLIB = $(CARMA_TOOLS)/lib

GCPUTIL = $(CARMA)/sza/util

# Bin directory in which to install binary executables

GCPBIN = $(CARMA)/sza/bin

# Include directories for the common library

INC   = -I$(CARMA)
INC  += -I$(CARMA)/include
INC  += -I$(CARMA_TOOLS)/include

INC  += -I$(ORBHOME)/include
INC  += -I$(ORBHOME)/include/JTC
INC  += -I$(ORBHOME)/include/OB

# Needed for building a shared library

LDPATH   += -L$(ORBLIB)
LDPATH   += -L$(CARMALIB)
LDPATH   += -L$(CARMATOOLSLIB)
LDPATH   += -L$(GCPLIB)

RPATH    += -Xlinker -R -Xlinker $(ORBLIB)
RPATH    += -Xlinker -R -Xlinker $(CARMALIB)
RPATH    += -Xlinker -R -Xlinker $(CARMATOOLSLIB)
RPATH    += -Xlinker -R -Xlinker $(GCPLIB)

# Libs needed for building the shared SzaUtil library

LIBS  = -lSzaArrayUtil

# Libs for .cc files

LIBS += -lrt
LIBS += -ldl
LIBS += -lreadline
LIBS += -ltermcap
LIBS += -L/usr/local/lib/liblabjackusb.so

# If preprocessor directives have changed, we may have to re-link
# binaries

BINDEPEND = $(CARMA)/sza/util/Directives.h

# SunOS requires different libraries that linux doesn't need

ifeq ($(OS), SunOS)
  LIBS += -lsocket
endif

# C++ standard library is supposed to be threadsafe with -D_REENTRANT

CCFLAGS += -D_REENTRANT

# And make sure any included C-header files from carma/ics/sza have
# _GPP defined

CCFLAGS += -D_GPP
CCFLAGS += $(NON_CARMA_FLAGS)

CC = g++

#-----------------------------------------------------------------------
# Targets
#-----------------------------------------------------------------------

all: libs bins make_test

%.o: $(GCPUTIL)/%.cc
	$(CC) $(CCFLAGS) $(OPTIONS) $(INC) -c -o $@ $<

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
# SZA Util lib
#-----------------------------------------------------------------------

LIBSO = libGcpArrayUtil.so

# List the dependencies by substituting from the source directory.
# This will cause the .o files to be made if they do not already
# exist.  Otherwise if .o files don't already exist, the library will
# be made without complaint with no targets

# In wildcard, we use only files beginning with capitals, since these
# are the class definitions.  Otherwise, threadserver.o and
# threadclient.o would get built into the library

UTILOBJ =  $(patsubst %.cc,%.o,$(wildcard $(GCPUTIL)/[A-Z]*.cc)) 
OBJ =  $(subst $(GCPUTIL)/,,$(UTILOBJ))

#OBJ = AntNum.o Center.o	CenterManager.o Client.o Coordinates.o Debug.o Exception.o FdSet.o \
#	IoLock.o Logger.o LogFile.o LogStream.o Mutex.o NetHandler.o NetTermHandler.o SignalTask.o \
#	Source.o QuadPath.o TerminalServer.o TcpClient.o TcpListener.o

RULESFILES =  $(wildcard $(GCPUTIL)/[A-Z]*.cc) 

$(GCPLIB)/$(LIBSO): $(OBJ)
	echo $(OBJ)	
	echo ' ';echo 'Compiling SZA Array Util library'; echo ' '
	g++ -shared -g -o $(GCPLIB)/$(LIBSO) $(OBJ)

libs: 	depend $(OBJ)
	@if [ ! -d $(GCPLIB) ] ; then mkdir $(GCPLIB) ; fi ;
	$(MAKE) $(GCPLIB)/$(LIBSO); 

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

# If Makefile.rules doesn't exist (see above), executing this rule
# will cause it to be made and sourced.

depend:
ifeq ($(RULES_EXISTS),n)
	$(CC) $(CCFLAGS) $(OPTIONS) $(INC) -MM $(RULESFILES) > Makefile.rules
	@set -e; [ -s Makefile.rules ] ;
endif

#-----------------------------------------------------------------------
# Clean directive
#-----------------------------------------------------------------------

clean_depend:
	\rm -f Makefile.rules

clean_files:
	\rm -f *.o

clean_libs:
	\rm -f $(GCPLIB)/$(LIBSO)

clean: clean_depend clean_files clean_libs

