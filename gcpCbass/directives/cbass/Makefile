#-----------------------------------------------------------------------
# Targets
#-----------------------------------------------------------------------

# If the directives don't exist, create the directives file from the template

make_make_directives:
	@if [ ! -f Makefile.directives ] ; then cp Makefile.directives.template Makefile.directives ; fi ;

# If any directives have changed since the last compile, touch relevant files

make_directives:
	$(MAKE) make_make_directives
ifeq ($(SPECIFIC_DIR_HAVE_CHANGED), y)
	@echo ""
	@echo "Specific directives have changed since the last compile: touching Directives.h"
	@echo ""
	@touch $(GCPTOP)/util/$(SPECIFIC)/Directives.h
	@echo "OLD_HAVE_USB    = " $(HAVE_USB)   >> Makefile.directives.last
endif
