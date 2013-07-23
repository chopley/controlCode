#-----------------------------------------------------------------------
# Targets
#-----------------------------------------------------------------------

# If the directives don't exist, create the directives file from the template

make_make_directives:
	echo "Inside generic make_make_directives"
	@if [ ! -f Makefile.directives ] ; then cp Makefile.directives.template Makefile.directives ; fi ;

# If any directives have changed since the last compile, touch relevant files

make_directives:
	echo "Inside generic make_directives"
	$(MAKE) make_make_directives
	echo "generic have changed = " $(GENERIC_DIR_HAVE_CHANGED)

# If directives have changed, touch the generic directives file

ifeq ($(GENERIC_DIR_HAVE_CHANGED), y)

	@echo ""
	@echo "Generic directives have changed since the last compile: touching gcp/util/common/Directives.h"
	@echo ""

	@touch $(GCPTOP)/util/common/Directives.h

	@echo "OLD_COMPILE_WITH_DEBUG = " $(COMPILE_WITH_DEBUG)  > Makefile.directives.last
	@echo "OLD_IS_STABLE          = " $(IS_STABLE)          >> Makefile.directives.last
	@echo "OLD_SPECIFIC           = " $(SPECIFIC)           >> Makefile.directives.last

endif
