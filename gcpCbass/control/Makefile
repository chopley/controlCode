GCPDIR := $(shell pwd)

GCPBIN  = $(GCPTOP)/bin
GCPLIB  = $(GCPTOP)/lib
GCPUTIL = $(GCPTOP)/util/common

export GCPDIR

export GCPBIN
export GCPLIB
export GCPUTIL

PGINC = $(TOOLSDIR)/include
PGLIB = $(TOOLSDIR)/lib

export PGINC
export PGLIB

ifeq ($(strip $(IS_STABLE)),)
 $(error IS_STABLE (1/0) has not been defined.  Set up your environment)
endif

all: libs bins

clean: clean_code clean_util clean_tools clean_depend

#-----------------------------------------------------------------------
# Recursive Make rules
#-----------------------------------------------------------------------

.PHONY : code

code:
	@cd $(GCPDIR)/code; make "GCPDIR=$(GCPDIR)"

libs:
	@cd $(GCPDIR)/code; make libs "GCPDIR=$(GCPDIR)"

bins:
	@cd $(GCPDIR)/code; make bins "GCPDIR=$(GCPDIR)"

test:
	@cd $(GCPDIR)/code; make test

clean_code:
	@cd $(GCPDIR)/code; make clean

clean_depend:
	@cd $(GCPDIR)/util; make clean_depend
	@cd $(GCPDIR)/code; make clean_depend

clean_util:
	@cd $(GCPDIR)/util; make clean

.PHONY : tools

tools:	
	cd $(GCPDIR)/tools; make	

clean_tools:	
	cd $(GCPDIR)/tools; make clean


