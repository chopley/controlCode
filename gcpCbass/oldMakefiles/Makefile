#-----------------------------------------------------------------------
# Include GCP-specific variables
#-----------------------------------------------------------------------

include Makefile.defs

#-----------------------------------------------------------------------
# Targets
#-----------------------------------------------------------------------

all:   links make_directives make_dirs libs make_control mods bins scripts tags

clean: clean_subdirs make_control_clean clean_depend clean_dirs clean_links clean_tags

# If any directives have changed since the last compile, touch relevant files

make_directives:
	cd directives/common;$(MAKE) make_directives
	@if [ -d directives/$(SPECIFIC) ]; then \
	  cd directives/$(SPECIFIC);$(MAKE) make_directives; \
	fi ;

# Make directories needed for compilation

make_dirs:
	@if [ ! -d $(LIBDIR) ] ; then mkdir $(LIBDIR) ; fi ;
	@if [ ! -d $(BINDIR) ] ; then  mkdir -p $(BINDIR);  fi; 
	@if [ ! -d $(HELPDIR) ] ; then  mkdir -p $(HELPDIR);  fi; 
	@if [ ! -d $(LOGDIR) ] ; then  mkdir -p $(LOGDIR);  fi; 
	@if [ ! -d gcp ] ; then  ln -s . gcp;  fi; 

.PHONY : modules

# Control library and binary creation

make_control:
	cd control;$(MAKE)

make_control_clean:
	cd control;$(MAKE) clean

#-----------------------------------------------------------------------
# Define lib targets
#-----------------------------------------------------------------------

MAKE_OBJS = antenna/control pgutil mediator program util grabber cfitsio

ifeq ($(HAVE_FFTW),1)
  MAKE_OBS += fftw_util
endif

ifeq ($(HAVE_MATLAB), 1)
  MAKE_OBJS += matlab
endif

ifeq ($(HAVE_IDL), 1)
  MAKE_OBJS += idl
endif

libs:
	@for dir in $(MAKE_OBJS) ; do \
	  echo "" ; \
	  echo "-----------------------------------------------------------------------" ; \
	  echo "      Making libs in directory:" $$dir ; \
	  echo "-----------------------------------------------------------------------" ; \
	  echo "" ; \
	  if [ -d $$dir/common ] ; then (cd $$dir/common; $(MAKE) libs); fi ; \
	  if [ -d $$dir/$(SPECIFIC) ] ; then (cd $$dir/$(SPECIFIC); $(MAKE) libs); fi ; \
	  pwd ;\
	done

#-----------------------------------------------------------------------
# Binary creation
#-----------------------------------------------------------------------

bins:
	@for dir in $(MAKE_OBJS) ; do \
	  if [ -d $$dir/common ] ; then (cd $$dir/common; $(MAKE) bins); fi ; \
	  if [ -d $$dir/$(SPECIFIC) ] ; then (cd $$dir/$(SPECIFIC); $(MAKE) bins); fi ; \
	done

#-----------------------------------------------------------------------
# Test binary creation
#-----------------------------------------------------------------------

test:
	@for dir in $(MAKE_OBJS) ; do \
	  echo 'Making test programs in: ' $$dir ; \
	  if [ -d $$dir/common/Test ] ; then (cd $$dir/common/Test; $(MAKE) test); fi ; \
	  if [ -d $$dir/$(SPECIFIC)/Test ] ; then (cd $$dir/$(SPECIFIC)/Test; $(MAKE) test); fi ; \
	done

#-----------------------------------------------------------------------
# Tags creation
#-----------------------------------------------------------------------

TAGSOBJ  = $(GCPTOP)/control/code/unix/control_src/common/*.c
TAGSOBJ += $(GCPTOP)/control/code/unix/control_src/specific/*.c
TAGSOBJ += $(GCPTOP)/control/code/unix/libunix_src/common/*.c
TAGSOBJ += $(GCPTOP)/control/code/unix/libunix_src/specific/*.c
TAGSOBJ += $(GCPTOP)/control/code/unix/libmonitor_src/*.c
TAGSOBJ += $(GCPTOP)/control/code/unix/libscan_src/*.c
TAGSOBJ += $(GCPTOP)/control/code/unix/libscript_src/*.c
TAGSOBJ += $(GCPTOP)/control/code/unix/libtclcontrol_src/*.c
TAGSOBJ += $(GCPTOP)/control/code/unix/libtransaction_src/*.cc
TAGSOBJ += $(GCPTOP)/control/code/unix/viewer_src/*.c

export MAKE_OBJS
for dir in $(MAKE_OBJS) ; do \
  echo "Making tags" ; \
  if [ -d $$dir/common ] ; then TAGSOBJ += $$dir/common/*.[c]*; fi ;\
  if [ -d $$dir/specific ] ; then TAGSOBJ += $$dir/specific/*.[c]*; fi ; \
done

tags:
	echo $(MAKE_OBJS)
	etags -o TAGS $(TAGSOBJ)

#-----------------------------------------------------------------------
# Clean directives
#-----------------------------------------------------------------------

clean_subdirs:
	@for dir in $(MAKE_OBJS) ; do \
	  if [ -d $$dir/common ] ; then (cd $$dir/common; $(MAKE) clean); fi ; \
	  if [ -d $$dir/common/Test ] ; then (cd $$dir/common/Test; $(MAKE) clean_test); fi ; \
	  if [ -d $$dir/$(SPECIFIC) ] ; then (cd $$dir/$(SPECIFIC); $(MAKE) clean); fi ; \
	  if [ -d $$dir/$(SPECIFIC)/Test ] ; then (cd $$dir/$(SPECIFIC)/Test; $(MAKE) clean_test); fi ; \
	done

#-----------------------------------------------------------------------
# Clean_Depend directives
#-----------------------------------------------------------------------

make_control_clean_depend: 
	cd control; $(MAKE) clean_depend

clean_depend_subdirs:
	@for dir in $(MAKE_OBJS) ; do \
	  if [ -d $$dir/common ] ; then (cd $$dir/common; $(MAKE) clean_depend); fi ; \
	  if [ -d $$dir/common/Test ] ; then (cd $$dir/common/Test; $(MAKE) clean_depend); fi ; \
	  if [ -d $$dir/$(SPECIFIC) ] ; then (cd $$dir/$(SPECIFIC); $(MAKE) clean_depend); fi ; \
	  if [ -d $$dir/$(SPECIFIC)/Test ] ; then (cd $$dir/$(SPECIFIC)/Test; $(MAKE) clean_depend); fi ; \
	done

clean_depend: make_control_clean_depend clean_depend_subdirs

#-----------------------------------------------------------------------
# Secific link creation
#-----------------------------------------------------------------------

LINK_OBJS = directives help control/conf control/code/unix/control_src \
	control/code/unix/libunix_src grabber mediator util scripts \
	antenna/control muxutil receiver scripts

links:
	@for dir in $(LINK_OBJS) ; do \
	   if [ -d $$dir/specific ] ; then \rm -f $$dir/specific; fi ; \
	   if [ -d $$dir/$(SPECIFIC) ] ; then ln -s $(SPECIFIC) $$dir/specific; fi ; \
	 done

#-----------------------------------------------------------------------
# Secific link removal
#-----------------------------------------------------------------------

unlinks:
	@for dir in $(LINK_OBJS) ; do \
	   if [ -d $$dir/specific ] ; then \rm -f $$dir/specific; fi ; \
	 done

#-----------------------------------------------------------------------
# Script installation
#-----------------------------------------------------------------------

SCRIPT_COMMON_OBJS   = $(notdir $(wildcard $(GCPTOP)/scripts/common/*))
SCRIPT_SPECIFIC_OBJS = $(notdir $(wildcard $(GCPTOP)/scripts/specific/*))

.PHONY : scripts

scripts: scripts_common scripts_specific

scripts_common:
  ifneq ($(SCRIPT_COMMON_OBJS),) 
	@for scr in $(SCRIPT_COMMON_OBJS) ; do \
	if [ -d $(BINDIR) ]; then \
	  if [ -L $(BINDIR)/$$scr ]; then \
               \rm $(BINDIR)/$$scr; \
	  fi ;\
          ln -s $(GCPTOP)/scripts/common/$$scr $(BINDIR)/$$scr; \
	fi ; \
	done 
  endif

scripts_specific:
  ifneq ($(SCRIPT_SPECIFIC_OBJS),) 
	@for scr in $(SCRIPT_SPECIFIC_OBJS) ; do \
	if [ -d $(BINDIR) ]; then \
	  if [ -L $(BINDIR)/$$scr ]; then \
               \rm $(BINDIR)/$$scr; \
	  fi ;\
          ln -s $(GCPTOP)/scripts/specific/$$scr $(BINDIR)/$$scr; \
	fi ; \
	done 
  endif

#-----------------------------------------------------------------------
# Module creation
#-----------------------------------------------------------------------

mods:
	$(make_module)

#-----------------------------------------------------------------------
# Other test directives
#-----------------------------------------------------------------------

clean_bins:
	cd bin; \rm -f *

clean_dirs:
	\rm -rf bin
	\rm -rf lib
	\rm -f gcp

clean_links: unlinks

clean_tags:
	\rm -f TAGS
