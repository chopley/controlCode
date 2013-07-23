#-----------------------------------------------------------------------
# Run the makefiles in the unix and vxworks subdirectories.
#-----------------------------------------------------------------------

all: libs bins

clean: clean_unix clean_depend

#-----------------------------------------------------------------------
# Recursive make targets.
#-----------------------------------------------------------------------

make_unix:
	@echo ' ';echo '       **** Building UNIX utilities ****'; echo ' '
	@cd unix; $(MAKE) "CC=g++" "GCPDIR=$(GCPDIR)"

libs:	
	@echo ' ';echo '       **** Building UNIX utility libs ****'; echo ' '
	@cd unix; $(MAKE) libs "CC=g++" "GCPDIR=$(GCPDIR)"

bins:	
	@echo ' ';echo '       **** Building UNIX utility bins ****'; echo ' '
	@cd unix; $(MAKE) bins "CC=g++" "GCPDIR=$(GCPDIR)"

clean_unix:
	@cd unix; $(MAKE) clean

clean_depend:
	@cd unix; $(MAKE) clean_depend

test:
	@cd unix; $(MAKE) test
