#-----------------------------------------------------------------------
# SPT util lib
#-----------------------------------------------------------------------

# Name of the library we will create

LIBSO_PREFIX  = libGcpUtilCommon

# Exclude gcpRegDoc from the generic binary list.  We will make this
# separately below

REGDOC = $(BINDIR)/$(BIN_EXE_PREFIX)RegDoc
$(REGDOC): bin_RegDoc.o $(BINDEPEND) $(GCPTOP)/control/code/unix/libunix_src/specific/specificregs.c
	@echo "MAKING REGDOC RULE"
	@if [ ! -d $(BIN) ] ; then  mkdir -p bin;  fi; 
	g++ -o $@ $(LDPATH) $(RPATH) $(LIBS) $<
	$@ greeting=f > $(HELPDIR)/registers.html

AUTODOC = $(BINDIR)/$(BIN_EXE_PREFIX)AutoDoc
$(AUTODOC): bin_AutoDoc.o $(BINDEPEND) \
	$(GCPTOP)/control/code/unix/control_src/common/genericscript.c \
	$(GCPTOP)/control/code/unix/control_src/common/generictypes.c \
	$(GCPTOP)/control/code/unix/control_src/specific/specificscript.c \
	$(GCPTOP)/control/code/unix/control_src/specific/specifictypes.c \
	$(GCPTOP)/control/code/unix/libscript_src/script.c \
	$(GCPTOP)/control/code/unix/libscript_src/scope.c
	@echo "MAKING AUTODOC RULE"
	@if [ ! -d $(BIN) ] ; then  mkdir -p bin;  fi; 
	g++ -o $@ $(LDPATH) $(RPATH) $< -L$(TOOLSDIR) $(LIBS)
	$@ greeting=f dir=$(HELPDIR)

SCHEDDOC = $(BINDIR)/$(BIN_EXE_PREFIX)SchedDoc
$(SCHEDDOC): bin_SchedDoc.o $(BINDEPEND)
	@echo "MAKING SCHEDDOC RULE"
	@if [ ! -d $(BIN) ] ; then  mkdir -p bin;  fi; 
	g++ -o $@ $(LDPATH) $(RPATH) $(LIBS) $<
	$@ greeting=f schDir=$(SCHDIR) outDir=$(HELPDIR)

BINEXC = $(REGDOC) $(AUTODOC) $(SCHEDDOC)

# And redefine bins to make both the generic binaries and RegDoc

bins: generic_bins $(BINEXC)

# Finally, include the generic Makefile

include $(GCPTOP)/Makefile_generic


