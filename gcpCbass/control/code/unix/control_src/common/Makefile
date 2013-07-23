# This file is designed to be invoked from a recursive call to make
# from the makefile in SZA/code/unix/.
#
# The makefile that includes it is expected to define the following
# macros:
#
#  BIN       - The full name of the SZA/code/unix/bin directory.
#  INC       - The full name of the SZA/code/unix/include directory.
#  LIB       - The full name of the SZA/code/unix/lib directory.
#  SHARE     - The full name of the SZA/code/share/include directory.
#  CC        - The C compiler to be used.
#  CFLAGS    - Flags for the C compiler.
#  RANLIB    - The archiver randomizer command (use : if not needed).
#-----------------------------------------------------------------------

CONF     = $(GCPTOP)/control/conf
SCRIPT   = $(GCPTOP)/scripts

#-----------------------------------------------------------------------
# Generic rule for making .o files
#-----------------------------------------------------------------------

%.o: %.c
	$(CC) $(CFLAGS) $(INCFLAGS) -c -o $@ $<

#-----------------------------------------------------------------------
# List the default targets.
#-----------------------------------------------------------------------

GCPCONTROL = $(BINDIR)/$(BIN_EXE_PREFIX)Control
GCPCONTROL_INIT = $(CONF)/$(SPECIFIC)/control.init
GCPMEDIATOR_INIT = $(CONF)/$(SPECIFIC)/mediator.init

all: libs $(GCPCONTROL) $(GCPCONTROL_INIT) $(GCPMEDIATOR_INIT)

#-----------------------------------------------------------------------
# Library generation
#-----------------------------------------------------------------------

# List the dependencies by substituting from the source directory.
# This will cause the .o files to be made if they do not already
# exist.  Otherwise if .o files don't already exist, the library will
# be made without complaint with no targets

MAINS = control.o

OBJS =  $(patsubst %.c,%.o,$(wildcard [a-z,A-Z]*.c)) 

LIB_OBJ = $(filter-out $(MAINS),$(OBJS))

# The name of the library to create

LIBSO = $(LIBDIR)/libGcpControlCommon$(LIBSO_SUFFIX)

$(LIBSO): $(LIB_OBJ)
	echo ' ';echo 'Compiling library: ' $(LIBSO); echo ' '
	g++ $(LIBSO_FLAGS) -o $(LIBSO) $(LIB_OBJ)

libs: 	depend $(LIB_OBJ)
	@if [ ! -d $(LIBDIR) ] ; then mkdir $(LIBDIR) ; fi ;
	$(MAKE) $(LIBSO); 

test:
	@cd Test; $(MAKE)

#-----------------------------------------------------------------------
# Program generation.
#-----------------------------------------------------------------------

CP_OBJ = genericcontrol.o

$(GCPCONTROL): $(CP_OBJ)
	@if [ ! -d $(GCPBIN) ] ; then mkdir $(GCPBIN) ; fi ;
	$(CC) -o $@ $(CP_OBJ) $(LDPATH) $(RPATH) \
	-lGcpControlCommon -lGcpControlSpecific -lGcpProgramCommon \
	-lGcpShareCommon -lGcpShareSpecific \
	-lGcpUtilCommon -lGcpUtilSpecific \
	-lGcpScript -lGcpSla -lGcpSrc -lGcpScan -lGcpMonitor -lGcpTransaction \
	-lGcpGrabberCommon \
	-lcfitsio \
	-lreadline -ltermcap $(SYSLIBS) $(MUXLIBS)

# According to the GNU Make manual, prefacing a command line with '-'
# causes errors to be ignored for that command.  This is the desired
# behavior for the replace commands, since the file will contain on or
# the other of (GCPDIR|GCPUDIR), but not both, and this rule is
# designed to cover either case.

$(GCPCONTROL_INIT): $(GCPCONTROL)
	-$(SCRIPT)/common/replace GCP_DIR GCP_GCPTOP_LEVEL $@
	-$(SCRIPT)/common/replace GCP_UDIR GCP_GCPTOP_LEVEL $@
ifeq ($(IS_STABLE), 1)
	-$(SCRIPT)/common/replace GCP_GCPTOP_LEVEL GCP_DIR $@
else
	-$(SCRIPT)/common/replace GCP_GCPTOP_LEVEL GCP_UDIR $@
endif

$(GCPMEDIATOR_INIT): $(GCPCONTROL)
	-$(SCRIPT)/common/replace GCP_DIR GCP_GCPTOP_LEVEL $@
	-$(SCRIPT)/common/replace GCP_UDIR GCP_GCPTOP_LEVEL $@
ifeq ($(IS_STABLE), 1)
	-$(SCRIPT)/common/replace GCP_GCPTOP_LEVEL GCP_DIR $@
else
	-$(SCRIPT)/common/replace GCP_GCPTOP_LEVEL GCP_UDIR $@
endif

clean_depend:
	\rm -f Makefile.rules

clean_files:
	/bin/rm -f *.o
clean: clean_files clean_depend

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
