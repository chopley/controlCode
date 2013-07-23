# Makefile for antenna common code.
#
#  Currently makes the shared CORBA antenna library
#  And a simple server/client application that uses it
#
#  @author Erik Leitch
#  @version $Revision: 1.1.1.1 $, $Date: 2009/07/06 23:57:05 $
#
# This file is intended to be called recursively from a parent makefile.
#
# The environment must define:
#
#  CARMA    -- The root of the common directory
#  CARMALIB -- The carma lib directory
#  ORBHOME  -- Root of the orbacus tree (e.g., /usr/local/orbacus)

OS := $(shell uname)

SZALIB   = $(CARMA)/sza/lib
SZABIN   = $(CARMA)/sza/bin
CARMALIB = $(CARMA)/lib
CARMATOOLSLIB = $(CARMA_TOOLS)/lib

HOSTNAME  = $(HOST)

# Include directories for the common library

THRDINC   = -I$(CARMA)
THRDINC  += -I$(CARMA)/include
THRDINC  += -I$(CARMA_TOOLS)/include

THRDINC  += -I$(ORBHOME)/include
THRDINC  += -I$(ORBHOME)/include/JTC
THRDINC  += -I$(ORBHOME)/include/OB

# Needed for building a shared library

LDPATH   += -L $(CARMALIB)
LDPATH   += -L $(CARMATOOLSLIB)
LDPATH   += -L $(SZALIB)

RPATH    += -Xlinker -R -Xlinker $(CARMALIB)
RPATH    += -Xlinker -R -Xlinker $(CARMATOOLSLIB)
RPATH    += -Xlinker -R -Xlinker $(SZALIB)

# Libs needed for building binaries

ifeq ($(HAVE_CORBA), 1)
  LIBS     = -lOB
  LIBS    += -lOBNaming
  LIBS    += -lOBEvent
  LIBS    += -lCosNotify
  LIBS    += -lJTC
  LIBS    += -lIMR
endif

# Only include CARMA libraries if we are compiling within the CARMA
# environment

ifeq ($(HAVE_CARMA), 1)

  LIBS    += -lcarmautil
  LIBS    += -llog4cpp
  LIBS    += -lcarmacanbus
  LIBS    += -ljanz

# Only include correlator libraries if we are compiling the correlator
# controls

  ifeq ($(HAVE_CORR), 1)
    LIBS    += -lcarmapipeline	
    LIBS    += -lcarmacorbapipeline
    LIBS    += -lcarmacorr
    LIBS    += -lcarmacorrelator
  endif

  ifeq ($(HAVE_CORBA), 1)
    LIBS    += -lcarmaAntApiCommon
  endif

endif

# If CARMA environment is not present, we must define a Program class
# for transparent compilation of binaries

ifeq ($(HAVE_CARMA), 0)
  LIBS    += -lSzaProgram
endif

# Canbus library can only be compiled and linked with if the CARMA
# environment is present

ifeq ($(HAVE_CARMA), 1)
  LIBS    += -lSzaAntennaCanbus
endif

# The Antenna CORBA library is only compiled and linked with if we are
# using CORBA for antenna communications

ifeq ($(ANT_CORBA), 1)
  LIBS    += -lSzaAntennaCorba
endif

# SZA libs we will always link with

LIBS    += -lSzaArrayShare
LIBS    += -lSzaSla
LIBS    += -lSzaSrc
LIBS    += -lSzaMonitor
LIBS    += -lSzaUtil
LIBS    += -lSzaAntennaControl

# System libs we will always link with

LIBS    += -lrt
LIBS    += -ldl
LIBS    += -lreadline
LIBS    += -ltermcap

# SunOS requires different libraries that linux doesn't need

ifeq ($(OS), SunOS)
  LIBS += -lsocket
endif

# C++ standard library is supposed to be threadsafe with -D_REENTRANT

CCFLAGS += -D_REENTRANT

# And make sure any included C-header files from carma/ics/sza have
# _GPP defined

CCFLAGS += -D_GPP

CCFLAGS += $(CARMA_FLAGS)

CC = g++
#-----------------------------------------------------------------------
# Targets
#-----------------------------------------------------------------------

all: bins

%.o: %.cc
	$(CC) $(CCFLAGS) $(OPTIONS) $(THRDINC) -g -c -o $@ $<

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
# SZA Controls lib
#-----------------------------------------------------------------------

#-----------------------------------------------------------------------
# Make dependencies
#-----------------------------------------------------------------------

THRDDEP =  $(wildcard [A-Z]*.cc)

depend: 
	$(CC) $(CCFLAGS) $(OPTIONS) $(THRDINC) -MM $(THRDDEP) > Makefile.rules

#-----------------------------------------------------------------------
# Make the control test programs
#-----------------------------------------------------------------------

bins: tAntennaDataFrameManager

tAntennaDataFrameManager: tAntennaDataFrameManager.o 
	g++ -g -o tAntennaDataFrameManager $(LDPATH) $(RPATH) $(LIBS) tAntennaDataFrameManager.o 

tCanMaster: tCanMaster.o 
	g++ -g -o tCanMaster $(LDPATH) $(RPATH) $(LIBS) tCanMaster.o 

tAntNum: tAntNum.o 
	g++ -g -o tAntNum $(LDPATH) $(RPATH) $(LIBS) tAntNum.o 

tSignalTask: tSignalTask.o 
	g++ -g -o tSignalTask $(LDPATH) $(RPATH) $(LIBS) tSignalTask.o 

tTimeVal: tTimeVal.o 
	g++ -g -o tTimeVal $(LDPATH) $(RPATH) $(LIBS) tTimeVal.o 

tAbsTimer: tAbsTimer.o 
	g++ -g -o tAbsTimer $(LDPATH) $(RPATH) $(LIBS) tAbsTimer.o 

tSubArray: tSubArray.o 
	g++ -g -o tSubArray $(LDPATH) $(RPATH) $(LIBS) tSubArray.o 


#-----------------------------------------------------------------------
# Clean directive
#-----------------------------------------------------------------------

clean:
	\rm -f Makefile.rules
	\rm -f *.o
	\rm -f tTimeVal	
	\rm -f tAbsTimer
	\rm -f tSubArray

#-----------------------------------------------------------------------
# Include the file in which automatic dependencies are deposited by
# make depend, above.  But only if it exists.
#-----------------------------------------------------------------------

RULESFILE := $(wildcard Makefile.rules)

ifeq ($(RULESFILE), Makefile.rules)
  include Makefile.rules
endif





