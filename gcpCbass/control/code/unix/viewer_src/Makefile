# This file is designed to be invoked from a recursive call to make
# from the makefile in GCP/code/unix/.
#
# The makefile that includes it is expected to define the following
# macros:
#
#  BIT         - The full name of the GCP/code/unix/bit directory.
#  BIN         - The full name of the GCP/code/unix/bin directory.
#  INC         - The full name of the GCP/code/unix/include directory.
#  LIB         - The full name of the GCP/code/unix/lib directory.
#  INC         - The full name of the GCP/code/share/include directory.
#  CC          - The C compiler to be used.
#  FC          - The FORTRAN compiler to use.
#  CFLAGS      - Flags for the C compiler.
#  RANLIB      - The archiver randomizer command (use : if not needed).
#  PGLIB       - The directory that contains libpgplot.
#  PGINC       - The directory that contains pgplot.h.
#  PGLD        - Loader flags for linking with the pgplot libraries.
#  TKLD        - Loader flags for linking with the Tcl/Tk libraries.
#  X11LD       - Loader flags for linking with the X-window library.
#  CLIB        - The full path name of the run-time C library of $(CC).
#  FC_STATIC   - The flags (if available) that tell the fortran compiler to
#                link the remaining libraries on its command line statically.
#  FC_DYNAMIC  - The flags (if available) that tell the fortran compiler to
#                link the remaining libraries on its command line dynamically.
#  FC_PG_RPATH - The flags (if any) that are needed to tell the run-time
#                linker, where to look for the shared pgplot libraries.
#-----------------------------------------------------------------------

GCPHELP   = $(GCPTOP)/help
GCPSCRIPT = $(GCPTOP)/scripts/common

#-----------------------------------------------------------------------
# Generic rule for making .o files
#-----------------------------------------------------------------------

%.o: %.c
	$(CC) $(CFLAGS) $(INCFLAGS) -c -o $@ $<

ifeq ($(strip $(IS_STABLE)),)
 $(error IS_STABLE (1/0) has not been defined.  Set up your environment)
endif

#-----------------------------------------------------------------------
# List the default targets.
#-----------------------------------------------------------------------

GCPVIEWER = $(GCPBIN)/$(BIN_EXE_PREFIX)Viewer

all: directives $(GCPBIN)/tkviewer $(LIB)/page.tcl $(LIB)/grabber.tcl $(LIB)/starplot.tcl $(LIB)/libtkpanel.so \
  $(LIB)/libtkpanel.a $(GCPVIEWER) $(GCPBIN)/convert_cnf

directives:
ifeq ($(DIR_HAVE_CHANGED), y)
	@touch viewer.tcl
endif

clean: clean_script clean_lib clean_prog clean_depend

#-----------------------------------------------------------------------
# Source file dependencies.
#-----------------------------------------------------------------------

monitor_viewer.o: $(PGINC)/cpgplot.h monitor_viewer.h $(INC)/freelist.h \
  $(INC)/regset.h $(INC)/regcal.h $(INC)/const.h $(INC)/astrom.h \
  $(INC)/lprintf.h axes.h $(INC)/optcam.h 

tclmonitor.o: tclmonitor.h monitor_viewer.h $(INC)/hash.h $(INC)/astrom.h \
  $(INC)/lprintf.h 

tclstarplot.o: tclstarplot.h $(INC)/hash.h $(INC)/astrom.h \
  $(INC)/lprintf.h

tclgrabber.o: tclgrabber.h $(INC)/hash.h $(INC)/astrom.h \
  $(INC)/lprintf.h 

tkviewer.o: tclmonitor.h netscape_remote.h $(PGINC)/tkpgplot.h \
  $(PGINC)/cpgplot.h

starplot.o: mathconst.h $(INC)/astrom.h

netscape_remote.o: netscape_remote.h

axes.o: axes.h $(PGINC)/cpgplot.h

panel.o: $(INC)/lprintf.h $(INC)/freelist.h

#-----------------------------------------------------------------------
# Script-file dependencies.
#-----------------------------------------------------------------------

SPECIFIC_CAPS := $(shell echo $(SPECIFIC) | tr "[:lower:]" "[:upper:]")

$(LIB)/page.tcl: page.tcl
	cp page.tcl $@
	-$(GCPSCRIPT)/replace gcpViewer $(SPECIFIC)Viewer $@
ifeq ($(IS_STABLE), 1)
	-$(GCPSCRIPT)/replace TextLabelColor SeaGreen2 $@
else
	-$(GCPSCRIPT)/replace TextLabelColor lightgreen $@
endif
	-$(GCPSCRIPT)/replace GCP_DIR $(SPECIFIC_CAPS)_DIR $@

$(LIB)/starplot.tcl: starplot.tcl
	cp starplot.tcl $@
ifeq ($(IS_STABLE), 1)
	-$(GCPSCRIPT)/replace TextLabelColor black $@
	-$(GCPSCRIPT)/replace BackgroundWidgetColor SeaGreen3 $@
else
	-$(GCPSCRIPT)/replace TextLabelColor lightgreen $@
	-$(GCPSCRIPT)/replace BackgroundWidgetColor ForestGreen $@
endif
	-$(GCPSCRIPT)/replace GCP_DIR $(SPECIFIC_CAPS)_DIR $@

$(LIB)/grabber.tcl: grabber.tcl
	cp grabber.tcl $@
	-$(GCPSCRIPT)/replace gcpViewer $(SPECIFIC)Viewer $@
ifeq ($(IS_STABLE), 1)
	-$(GCPSCRIPT)/replace TextLabelColor SeaGreen2 $@
else
	-$(GCPSCRIPT)/replace TextLabelColor lightgreen $@
endif
	-$(GCPSCRIPT)/replace GCP_DIR $(SPECIFIC_CAPS)_DIR $@

$(GCPVIEWER): viewer.tcl
	\rm -f $@
	echo '#!'$(GCPBIN)/tkviewer >> $@
	echo "set ::lib_dir $(LIB)" >> $@
	echo "set ::bit_dir $(BIT)" >> $@
	echo "set ::help_dir $(GCPHELP)" >> $@
	cat viewer.tcl >> $@
	chmod +x $@
ifeq ($(IS_STABLE), 1)
	-$(GCPSCRIPT)/replace BackgroundWidgetColor SeaGreen3 $@
	-$(GCPSCRIPT)/replace ActiveWidgetColor SeaGreen2 $@
	-$(GCPSCRIPT)/replace AlternateLabelColor SeaGreen2 $@
	-$(GCPSCRIPT)/replace DisabledColor SeaGreen2 $@
	-$(GCPSCRIPT)/replace TextLabelColor SeaGreen2 $@
else
	-$(GCPSCRIPT)/replace BackgroundWidgetColor ForestGreen $@
	-$(GCPSCRIPT)/replace ActiveWidgetColor lightgreen $@
	-$(GCPSCRIPT)/replace AlternateLabelColor lightgreen $@
	-$(GCPSCRIPT)/replace DisabledColor lightgreen $@
	-$(GCPSCRIPT)/replace TextLabelColor lightgreen $@
	-$(GCPSCRIPT)/replace GCPDIR GCP_UDIR $@
endif
	-$(GCPSCRIPT)/replace gcpViewer $(SPECIFIC)Viewer $@
	-$(GCPSCRIPT)/replace GCP_DIR $(SPECIFIC_CAPS)_DIR $@
	chmod +x $@
	chmod -w $@

clean_script:
	@if [ -f $(GCPVIEWER) ] ; then /bin/rm -f $(GCPVIEWER); fi

#-----------------------------------------------------------------------
# Library generation.
#-----------------------------------------------------------------------

PANEL_OBJ = panel.o

$(LIB)/libtkpanel.a: depend $(PANEL_OBJ)
	ar ru $@ $(PANEL_OBJ)
	$(RANLIB) $@

$(LIB)/libtkpanel.so: depend $(PANEL_OBJ) 
	$(CC) -o $@ $(DLFLAGS) $(PANEL_OBJ) -L$(CBASS_TOOLS)/lib $(TKLD) $(X11LD) \
 $(SYSLIBS)

clean_lib:
	@ if [ -f $(PANEL_OBJ) ] ; then /bin/rm -f $(PANEL_OBJ) ; fi
	@ if [ -f $(LIB)/libtkpanel.a ] ; then /bin/rm -f $(LIB)/libtkpanel.a ; fi
	@ if [ -f $(LIB)/libtkpanel.so ] ; then /bin/rm -f $(LIB)/libtkpanel.so ; fi

#-----------------------------------------------------------------------
# Program generation
#-----------------------------------------------------------------------

VIEW_OBJ = monitor_viewer.o tclmonitor.o tclgrabber.o tkviewer.o axes.o netscape_remote.o tclstarplot.o

$(GCPBIN)/tkviewer: depend $(VIEW_OBJ) $(PGLIB)/libtkpgplot.a $(LIB)/libtkpanel.a \
  $(PGLIB)/libcpgplot.a 
	$(FC) $(FC_FLAGS) -o $@ $(VIEW_OBJ) $(FC_DYNAMIC) $(GCPLIB)/libGcpMonitor$(LIBSO_SUFFIX) \
 $(FC_STATIC) -L$(LIB) -ltkpanel $(FC_DYNAMIC) -L$(GCPLIB) \
	$(GCPLIB)/libGcpTclControl$(LIBSO_SUFFIX) \
	$(GCPLIB)/libGcpPgUtilCommon$(LIBSO_SUFFIX) \
	$(GCPLIB)/libGcpShareCommon$(LIBSO_SUFFIX) $(GCPLIB)/libGcpShareSpecific$(LIBSO_SUFFIX) \
	$(GCPLIB)/libGcpSla$(LIBSO_SUFFIX) $(GCPLIB)/libGcpSrc$(LIBSO_SUFFIX) \
	$(GCPLIB)/libGcpUtilCommon$(LIBSO_SUFFIX) $(GCPLIB)/libGcpUtilSpecific$(LIBSO_SUFFIX) \
	$(GCPLIB)/libGcpGrabberCommon$(LIBSO_SUFFIX) \
	$(GCPLIB)/libcfitsio$(LIBSO_SUFFIX) \
 $(FC_PG_RPATH) -L$(CBASS_TOOLS)/lib $(PGLD) -ltkpgplot $(TKLD) $(X11LD) -lXmu \
 $(SYSLIBS) $(MUXLIBS)

$(GCPBIN)/convert_cnf: convert_cnf.tcl
	cp convert_cnf.tcl $@

clean_prog:
	/bin/rm -f *.o
	@ if [ -f $(GCPBIN)/tkviewer ] ; then /bin/rm -f $(GCPBIN)/tkviewer ; fi
	@ if [ -f $(GCPBIN)/convert_cnf ] ; then /bin/rm -f $(GCPBIN)/convert_cnf ; fi

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

clean_depend:
	/bin/rm -f Makefile.rules
