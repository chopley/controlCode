#-----------------------------------------------------------------------
# Force make to use sh, so that we can write shell commands uniformly
# on different systems
#-----------------------------------------------------------------------

#SHELL     = /bin/sh

#-----------------------------------------------------------------------
# Get variables we can determine from the system
#-----------------------------------------------------------------------

TOOLSDIR := $(shell pwd)
OS       := $(shell uname -s)
SCRIPTDIR = $(TOOLSDIR)/scripts

ifeq ($(OS),Linux)
  PROC := $(shell uname -i)
else
  PROC    := $(shell uname -p)
  RELEASE := $(shell uname -r)
endif

RELEASELIST = $(subst ., ,$(RELEASE))
MAJVERS     = $(firstword $(RELEASELIST))

#-----------------------------------------------------------------------
# Check the processor type for bitness
#-----------------------------------------------------------------------

# Ordinary Linux systems report the architecture in uname

ifeq ($(strip $(PROC)),i386)
  COMPILE_FOR_64BIT = 0
else
  COMPILE_FOR_64BIT = 1
endif

# However, later versions of Darwin can have 32-bit processor, with
# 64-bit architecture.  I don't know how to determine this generally
# at this point, but Darwin 10+ systems I've played with require
# 64-bit libraries even though uname reports the architecture as i386

ifeq ($(OS),Darwin)
  ifneq ($(MAJVERS),8)
    COMPILE_FOR_64BIT = 1
  endif
endif

#-----------------------------------------------------------------------
# Determine what we can about fortran compilers present on the system
#-----------------------------------------------------------------------

FCOMPG77  := $(shell which g77)
FCOMPGFOR := $(shell which gfortran)

FCOMP =
FFLAG =
ifneq (,$(strip $(FCOMPG77)))
  FCOMP = g77
  FFLAG = -fPIC -O -Wno-globals
endif

#ifneq (,$(strip $(FCOMPGFOR)))
#  FCOMP = gfortran
#  FFLAG = -fPIC -O
#endif

#-----------------------------------------------------------------------
# All targets
#-----------------------------------------------------------------------

ifneq (,$(FCOMP))
all: dirs tcl tk pgplot fftw
else
all: dirs tcl tk fftw
endif

#-----------------------------------------------------------------------
# Dirs
#-----------------------------------------------------------------------

dirs:
	@if [ ! -d lib ] ; then mkdir lib ; fi ;
	@if [ ! -d include ] ; then mkdir include ; fi ;
	@if [ ! -d src ] ; then mkdir src ; fi ;

dirs_clean:
	@if [ -d lib ] ; then \rm -rf lib ; fi ;
	@if [ -d include ] ; then \rm -rf include ; fi ;
	@if [ -d share ] ; then \rm -rf share ; fi ;
	@if [ -d man ] ; then \rm -rf man ; fi ;
	@if [ -d bin ] ; then \rm -rf bin ; fi ;
	@if [ -d src ] ; then \rm -rf src ; fi ;

#-----------------------------------------------------------------------
# PGPLOT 
#-----------------------------------------------------------------------

PGPLOTVERS   = 5.2
PGPLOTPREFIX = archive/pgplot$(PGPLOTVERS)
PGPLOTDIR    = pgplot

PGX11LDPATH    = /usr/X11R6/lib # 32-bit Linux, all Darwin compiles
ifeq ($(OS),Linux)
  ifeq ($(COMPILE_FOR_64BIT),1)
    PGX11LDPATH    = /usr/X11R6/lib64 # 64-bit Linux compile
  endif
endif

PGGCCCOMPATLIB =  # Linux compile
ifeq ($(OS),Darwin)
  ifeq ($(COMPILE_FOR_64BIT),0)
    PGGCCCOMPATLIB = -L/usr/lib -lgcc # 32-bit Darwin compile
  endif
endif

PGSHAREDFLAGS  = -shared # linux compile
ifeq ($(OS),Darwin)
  ifeq ($(COMPILE_FOR_64BIT),0)
    PGSHAREDFLAGS  = -bundle -flat_namespace -undefined suppress # Darwin 32-bit compile
  else
    PGSHAREDFLAGS  = -dynamiclib -flat_namespace -undefined suppress # Darwin 64-bit compile
  endif
endif

PGFFLAGC       = 'FFLAGC=-u -Wall -fPIC -O' # default linux
ifeq ($(OS),Linux)
  ifeq ($(COMPILE_FOR_64BIT),1)
    PGFFLAGC = 'FFLAGC=-fPIC -O'          # 64-bit darwin
  endif
endif

PGCCOMPL = gcc
PGFCOMPL = $(FCOMP)
ifeq ($(OS),Darwin)
  ifeq ($(COMPILE_FOR_64BIT),1)
    PGCCOMPL = gcc -m64
    PGFCOMPL = $(FCOMP) -m64
  endif
endif

PGLIBSUFFIX = so
ifeq ($(OS),Darwin)
  ifeq ($(COMPILE_FOR_64BIT),1)
    PGLIBSUFFIX = dylib
  endif
endif

pgplot_compl:
	echo $(PGCCOMPL)
	echo $(PGFCOMPL)

pgplot_unpack:
	@if [ -f $(PGPLOTPREFIX).tar.gz ] ; then cp $(PGPLOTPREFIX).tar.gz $(PGPLOTPREFIX)Copy.tar.gz ; fi ;
	@if [ -f $(PGPLOTPREFIX)Copy.tar.gz ] ; then gunzip $(PGPLOTPREFIX)Copy.tar.gz ; fi ;
	@if [ -f $(PGPLOTPREFIX)Copy.tar ] ; then tar xvf $(PGPLOTPREFIX)Copy.tar ; rm $(PGPLOTPREFIX)Copy.tar ; mv $(PGPLOTDIR) src ; fi ;

pgplot_create_makefile:
	@if [ -d src/$(PGPLOTDIR) ] ; then \
	  mv src/$(PGPLOTDIR) src/pgplot_src ; \
	  mkdir src/$(PGPLOTDIR) ; cp src/pgplot_src/drivers.list src/$(PGPLOTDIR); \
	  $(SCRIPTDIR)/replace '! XWDRIV' '  XWDRIV' src/$(PGPLOTDIR)/drivers.list ; \
	  $(SCRIPTDIR)/replace '! TKDRIV' '  TKDRIV' src/$(PGPLOTDIR)/drivers.list ; \
	  $(SCRIPTDIR)/replace '! PSDRIV' '  PSDRIV' src/$(PGPLOTDIR)/drivers.list ; \
	  cd src/$(PGPLOTDIR); ../pgplot_src/makemake ../pgplot_src linux g77_gcc ; \
	fi ;

pgplot_create_cpgrule:
	@if [ -d src/$(PGPLOTDIR) ] ; then \
	  cd src/$(PGPLOTDIR); \
	  echo -e '\012libcpgplot.so: \044(PG_SOURCE) pgbind' >> cpgMakefile ; \
	  echo -e '\011./pgbind \044(PGBIND_FLAGS) -h -w \044(PG_SOURCE)' >> cpgMakefile ; \
	  echo -e '\011\044(CCOMPL) -c \044(CFLAGC) cpg*.c' >> cpgMakefile ; \
	  echo -e '\011rm -f cpg*.c' >> cpgMakefile ; \
	  echo -e '\011gcc -shared  -o libcpgplot.so cpg*.o' >> cpgMakefile ; \
	  echo -e '\011rm -f cpg*.o' >> cpgMakefile ; \
	fi ;

pgplot_edit_cpgrule:
	-$(SCRIPTDIR)/replace '-e ' '' src/$(PGPLOTDIR)/cpgMakefile

pgplot_make:
	@if [ -d src/$(PGPLOTDIR) ] ; then \
	  cd src/$(PGPLOTDIR); \
	  cat cpgMakefile >> makefile ; \
	  \rm cpgMakefile ; \
	  $(SCRIPTDIR)/replace 'libcpgplot.a cpgplot.h cpgdemo' 'libcpgplot.a libcpgplot.so cpgplot.h cpgdemo' makefile ; \
	  $(SCRIPTDIR)/replace 'TK_INCL=-I/usr/include' 'TK_INCL=-I$(TOOLSDIR)/include' makefile ; \
	  $(SCRIPTDIR)/replace 'TK_LIBS=-L/usr/lib -ltk -ltcl -L/usr/X11R6/lib -lX11 -ldl' 'TK_LIBS=-L$(TOOLSDIR)/lib -ltk -ltcl -L$(PGX11LDPATH) -lX11 $(PGGCCCOMPATLIB) -ldl' makefile ; \
	  $(SCRIPTDIR)/replace 'gcc -shared' 'gcc $(PGSHAREDFLAGS)' makefile ; \
	  $(SCRIPTDIR)/replace 'FFLAGC=-u -Wall -fPIC -O' 'FFLAGC=-fPIC -O' makefile ; \
	  $(SCRIPTDIR)/replace 'CCOMPL=gcc' 'CCOMPL=$(PGCCOMPL)' makefile ; \
	  $(SCRIPTDIR)/replace 'FCOMPL=g77' 'FCOMPL=$(PGFCOMPL)' makefile ; \
	  $(SCRIPTDIR)/replace 'libpgplot.so' 'libpgplot.$(PGLIBSUFFIX)' makefile ; \
	  $(SCRIPTDIR)/replace 'libcpgplot.so' 'libcpgplot.$(PGLIBSUFFIX)' makefile ; \
	fi ;

pgplot_make_make:
	@if [ -d src/$(PGPLOTDIR) ] ; then \
	  cd src/$(PGPLOTDIR); \
	  make ; make cpg ; cp lib* ../../lib ; cp *.h ../../include ; cp grfont.dat ../../lib ; cd ../../ \
; \
	fi ;

pgplot: dirs pgplot_unpack pgplot_create_makefile pgplot_create_cpgrule pgplot_edit_cpgrule pgplot_make pgplot_make_make

pgplot_clean:
	@if [ -d src/pgplot ] ; then \rm -rf src/pgplot ; fi ;
	@if [ -d src/pgplot_src ] ; then \rm -rf src/pgplot_src ; fi ;

#-----------------------------------------------------------------------
# TCL
#-----------------------------------------------------------------------

#TCLVERS_MAJ = 8.4
#TCLVERS_MIN = 19                                                                                
TCLVERS_MAJ = 8.5
TCLVERS_MIN = 10
TCLVERS     = $(TCLVERS_MAJ).$(TCLVERS_MIN)
TCLPREFIX   = archive/tcl$(TCLVERS)-src
TCLDIR      = tcl$(TCLVERS)

TCLCONF =
ifeq ($(OS),Darwin)
  TCLCONF = --enable-frameworks
  LIBSUFFIX = 'dylib'
else
  LIBSUFFIX = 'so'
endif

test:
	echo $(TCLDIR)

tcl: dirs
	@if [ -f $(TCLPREFIX).tar.gz ] ; then cp $(TCLPREFIX).tar.gz $(TCLPREFIX)Copy.tar.gz ; fi ;
	@if [ -f $(TCLPREFIX)Copy.tar.gz ] ; then gunzip $(TCLPREFIX)Copy.tar.gz ; fi ;
	@if [ -f $(TCLPREFIX)Copy.tar ] ; then tar xvf $(TCLPREFIX)Copy.tar ; rm $(TCLPREFIX)Copy.tar ; mv $(TCLDIR) src ; fi ;
	@if [ -d src/$(TCLDIR) ] ; then \
          cd src/$(TCLDIR)/unix ; ./configure --prefix=$(TOOLSDIR) --exec_prefix=$(TOOLSDIR) --enable-shared $(TCLCONF) ; \
          make ; make install ; cd ../../../ ; ln -s $(TOOLSDIR)/lib/libtcl$(TCLVERS_MAJ).$(LIBSUFFIX) $(TOOLSDIR)/lib/libtcl.$(LIBSUFFIX) ; \
        fi ;

tcl_clean:
	@if [ -d src/$(TCLDIR) ] ; then \rm -rf src/$(TCLDIR) ; fi ;

#-----------------------------------------------------------------------
# TK
#-----------------------------------------------------------------------

#TKVERS_MIN = 19                                                                                 
TKVERS_MAJ = 8.5
TKVERS_MIN = 10
TKVERS     = $(TKVERS_MAJ).$(TKVERS_MIN)
TKPREFIX   = archive/tk$(TKVERS)-src
TKDIR      = tk$(TKVERS)

TKCONF =
ifeq ($(OS),Darwin)
  TKCONF = --enable-frameworks
  LIBSUFFIX = 'dylib'
else
  LIBSUFFIX = 'so'
endif

tk: dirs
	@if [ -f $(TKPREFIX).tar.gz ] ; then cp $(TKPREFIX).tar.gz $(TKPREFIX)Copy.tar.gz ; fi ;
	@if [ -f $(TKPREFIX)Copy.tar.gz ] ; then gunzip $(TKPREFIX)Copy.tar.gz ; fi ;
	@if [ -f $(TKPREFIX)Copy.tar ] ; then tar xvf $(TKPREFIX)Copy.tar ; rm $(TKPREFIX)Copy.tar ; mv	$(TKDIR) src ; fi ;
	@if [ -d src/$(TKDIR) ] ; then \
          cd src/$(TKDIR)/unix ; ./configure --prefix=$(TOOLSDIR) --exec_prefix=$(TOOLSDIR) --enable-shared $(TKCONF) ; \
          make ; make install ; cd ../../../ ; ln -s $(TOOLSDIR)/lib/libtk$(TKVERS_MAJ).$(LIBSUFFIX) $(TOOLSDIR)/lib/libtk.$(LIBSUFFIX) ; \
        fi ;

tk_clean:
	@if [ -d src/$(TKDIR) ] ; then \rm -rf src/$(TKDIR) ; fi ;

#-----------------------------------------------------------------------
# FFTW
#-----------------------------------------------------------------------

FFTWVERS    = 3.2.1
FFTWPREFIX  = archive/fftw-$(FFTWVERS)
FFTWDIR     = fftw-$(FFTWVERS)

fftw: dirs
	@if [ -f $(FFTWPREFIX).tar.gz ] ; then cp $(FFTWPREFIX).tar.gz $(FFTWPREFIX)Copy.tar.gz ; fi ;
	@if [ -f $(FFTWPREFIX)Copy.tar.gz ] ; then gunzip $(FFTWPREFIX)Copy.tar.gz ; fi ;
	@if [ -f $(FFTWPREFIX)Copy.tar ] ; then tar xvf $(FFTWPREFIX)Copy.tar ; rm $(FFTWPREFIX)Copy.tar ; mv $(FFTWDIR) src ; fi ;
	@if [ -d src/$(FFTWDIR) ] ; then \
	  cd src/$(FFTWDIR) ; ./configure --prefix=$(TOOLSDIR) --exec_prefix=$(TOOLSDIR) --enable-shared ; \
	  make ; make install ; cd ../../ ; \
	fi ;

fftw_clean:
	@if [ -d src/$(FFTWDIR) ] ; then \rm -rf src/$(FFTWDIR) ; fi ;

#-----------------------------------------------------------------------
# Clean directive
#-----------------------------------------------------------------------

clean: dirs_clean tcl_clean tk_clean pgplot_clean fftw_clean
