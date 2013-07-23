# The parent directory.

CODE = $(GCPDIR)/code

# The unix html help directory.

HELP = $(CODE)/unix/help

# The unix bitmap directory.

BIT = $(CODE)/unix/bit

# The unix binary directory.

BIN = $(CODE)/unix/bin

# The unix library directory.

LIB = $(CODE)/unix/lib

# The unix include directory.

INC = $(CODE)/unix/libunix_src/common

TCL = $(CODE)/unix/libtclcontrol_src

# The shared include directory.

SHARE = $(CODE)/share/include

# The directory that contains the slalib source code.

SLALIB = $(CODE)/share/slalib

# The rules file for shared code.

SHARE_RULES = $(CODE)/share/Makefile.rules

# The PGPLOT library and include-file directory.

# Optimisation flag.

OPT = -O

# The C compiler and its run-time library.

CC     = g++
CCLIB  = $(shell gcc -print-libgcc-file-name)

# We have to kludge the C++ run0-time library because gcc provides no
# option to return the name
# Removed kludge because it was causing a conflict between different
# revisions of libstdc++ when compiling under Red Hat Enterprise Linux 4

#CPPLIB = -lstdc++
CPPLIB = 

# Set CLIB to the sum of the C and C++ runtime libs

CLIB   = $(CCLIB)
CLIB  += $(CPPLIB)

# C compiler flags.

#CFLAGS = $(OPT) -Wall -Wmissing-prototypes -Wmissing-declarations -D_POSIX_C_SOURCE=199506L -D__EXTENSIONS__ -I$(INC) -I$(SHARE)

# Uncomment the following  for debugging purposes:
#
# Solaris
#CFLAGS = -g -Wall -Wmissing-prototypes -Wmissing-declarations -D_POSIX_C_SOURCE=199506L -D__EXTENSIONS__ -I$(INC) -I$(SHARE)
# Linux
#CFLAGS = -Wall -D_POSIX_C_SOURCE=199506L -D_REENTRANT -D_BSD_SOURCE -I$(INC) -I$#(SHARE) -I$(GCPTOP) $(GENERIC_FLAGS)
CFLAGS = $(CCFLAGS) -I/usr/include/readline -D_BSD_SOURCE

LDPATH += -L$(LIB)

ifneq ($(MAC_OSX),1)
  RPATH    += -Xlinker -R -Xlinker $(LIBDIR)
endif

INCFLAGS += -I$(GCPTOP)
INCFLAGS += -I$(SHARE)
INCFLAGS += -I$(INC)
INCFLAGS += -I$(TCL)

export INCFLAGS

# The following flags specify which Fortran compiler is to be used,
# and the flags that it needs when being used to link a program.
# The meanings of the flag are:
#
# FC          - The command name of the FORTRAN compiler.
# FC_STATIC   - The flags (if available) that tell the fortran compiler to
#               link the remaining libraries on its command line statically.
# FC_DYNAMIC  - The flags (if available) that tell the fortran compiler to
#               link the remaining libraries on its command line dynamically.
# FC_PG_RPATH - The flags (if any) that are needed to tell the run-time
#               linker, where to look for the shared pgplot libraries.
#-----------------------------------------------------------------------

# Uncomment the following for use with sun f77.

#FC = f77
#FC_STATIC = -Bstatic
#FC_DYNAMIC = -Bdynamic
#FC_PG_RPATH = -R$(PGLIB)

# Uncomment the following for use with gnu g77.

ifeq ($(MAC_OSX),1)
  FC = g++
  FC_FLAGS = -bind_at_load
  FC_STATIC = -Xlinker
  FC_DYNAMIC = -Xlinker
  FC_PG_RPATH = 
else
#  FC = gfortran
  FC = g77
  FC_FLAGS = 
  FC_STATIC = -Xlinker -Bstatic
  FC_DYNAMIC = -Xlinker -Bdynamic
  FC_PG_RPATH = -Xlinker -R$(PGLIB)
endif

export FC_FLAGS

# The library randomizer (not required under Solaris).

RANLIB = @:

# What's needed to link with X11?

# Solaris
# X11LD = -L/usr/openwin/lib -R/usr/openwin/lib -lX11
# Linux

ifeq ($(COMPILE_FOR_64BIT), 1)
 X11LD = -L/usr/X11R6/lib64 -Xlinker -R/usr/X11R6/lib64 -lX11
else
 ifneq ($(MAC_OSX),1)
   X11LD = -L/usr/X11R6/lib -Xlinker -R/usr/X11R6/lib -lX11
 else
   X11LD = -L/usr/X11R6/lib -lX11
 endif
endif

# What's needed to link pgplot?
# Solaris:
# PGLD = -L$(PGLIB) -R$(PGLIB) -lcpgplot -lpgplot
# Linux

ifneq ($(MAC_OSX),1)
 PGLD = -L$(PGLIB) -Xlinker -R$(PGLIB) -lcpgplot -lpgplot -lg2c
# PGLD = -L$(PGLIB) -Xlinker -R$(PGLIB) -lcpgplot -lpgplot -lgfortran
else
 PGLD = -L$(PGLIB) -lcpgplot -lpgplot -lg2c
endif

# The loader flags needed to link with Tcl/Tk.
# Linux

# The flags for including Tcl/Tk files.

TK_INC = -I/usr/local/include

# Flags needed by ld when creating a shared library.

# Solaris:
# DLFLAGS = -G -i
# Linux

ifneq ($(MAC_OSX),1)
 DLFLAGS = -shared
else
 DLFLAGS = -dynamiclib -flat_namespace -undefined suppress 
endif

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $< 

#-----------------------------------------------------------------------
# Default to making all targets.
#-----------------------------------------------------------------------

all: bin lib include make_libsla make_libunix make_libsrc make_libscan \
	make_libtransaction make_libscript make_libmonitor make_control \
	make_libtclcontrol \
	make_viewer make_tcl_lib

libs: lib include make_libsla make_libunix make_libsrc make_libscan \
	make_libtransaction make_libscript make_libmonitor make_libtclcontrol \
	make_tcl_lib

bins: bin make_control make_viewer

clean: clean_code clean_dirs

#-----------------------------------------------------------------------
# Directory creation.
#-----------------------------------------------------------------------

bin:
	mkdir bin

lib:
	mkdir lib

include:
	mkdir include

clean_dirs:
	/bin/rm -rf bin	
	/bin/rm -rf lib
	/bin/rm -rf include	

#-----------------------------------------------------------------------
# Recursive make targets.
#-----------------------------------------------------------------------

make_libsrc:
	@echo ' ';echo 'Building lib/libGcpSrc.so'; echo ' '
	@cd libsrc_src; $(MAKE) "INC=$(INC)" "LIB=$(LIB)" "SHARE=$(SHARE)" \
	"CC=$(CC)" "CFLAGS=$(CFLAGS)" "RANLIB=$(RANLIB)" "SYSLIBS=$(SYSLIBS)"

clean_libsrc:
	@cd libsrc_src; $(MAKE) clean "INC=$(INC)" "LIB=$(LIB)" \
	"SHARE=$(SHARE)" "CC=$(CC)" "CFLAGS=$(CFLAGS)" "RANLIB=$(RANLIB)" \
	"SYSLIBS=$(SYSLIBS)"

make_libscan:
	@echo ' ';echo 'Building lib/libGcpScan.so'; echo ' '
	@cd libscan_src; $(MAKE) "INC=$(INC)" "LIB=$(LIB)" "SHARE=$(SHARE)" \
	"CC=$(CC)" "CFLAGS=$(CFLAGS)" "RANLIB=$(RANLIB)" "SYSLIBS=$(SYSLIBS)"

clean_libscan:
	@cd libscan_src; $(MAKE) clean "INC=$(INC)" "LIB=$(LIB)" \
	"SHARE=$(SHARE)" "CC=$(CC)" "CFLAGS=$(CFLAGS)" "RANLIB=$(RANLIB)" \
	"SYSLIBS=$(SYSLIBS)"

make_libtransaction:
	@echo ' ';echo 'Building lib/libGcpTransaction.so'; echo ' '
	@cd libtransaction_src; $(MAKE) 

clean_libtransaction:
	@cd libtransaction_src; $(MAKE) clean

make_libsla:
	@echo ' ';echo 'Building lib/libSla.a'; echo ' '
	@cd libsla_src; $(MAKE) "INC=$(INC)" "LIB=$(LIB)" "SHARE=$(SHARE)" \
	"CC=$(CC)" "CFLAGS=$(CFLAGS)" "RANLIB=$(RANLIB)" "SLALIB=$(SLALIB)" \
	"SYSLIBS=$(SYSLIBS)"

clean_libsla:
	@cd libsla_src; $(MAKE) clean "INC=$(INC)" "LIB=$(LIB)" \
	"SHARE=$(SHARE)" "CC=$(CC)" "CFLAGS=$(CFLAGS)" "RANLIB=$(RANLIB)" \
	"SLALIB=$(SLALIB)" "SYSLIBS=$(SYSLIBS)"

make_libunix:
	@echo ' ';echo 'Building lib/libunix.a'; echo ' '
	@cd libunix_src/$(SPECIFIC); $(MAKE) "INC=$(INC)" "LIB=$(LIB)" "SHARE=$(SHARE)" \
	"CC=$(CC)" "CFLAGS=$(CFLAGS)" "RANLIB=$(RANLIB)" \
	"SHARE_RULES=$(SHARE_RULES)" "SYSLIBS=$(SYSLIBS)"

	@echo ' ';echo 'Building lib/libunix.a'; echo ' '
	@cd libunix_src/common; $(MAKE) "INC=$(INC)" "LIB=$(LIB)" "SHARE=$(SHARE)" \
	"CC=$(CC)" "CFLAGS=$(CFLAGS)" "RANLIB=$(RANLIB)" \
	"SHARE_RULES=$(SHARE_RULES)" "SYSLIBS=$(SYSLIBS)"

clean_libunix:
	@cd libunix_src/$(SPECIFIC); $(MAKE) clean "INC=$(INC)" "LIB=$(LIB)" \
	"SHARE=$(SHARE)" "CC=$(CC)" "CFLAGS=$(CFLAGS)" "RANLIB=$(RANLIB)" \
	"SHARE_RULES=$(SHARE_RULES)" "SYSLIBS=$(SYSLIBS)"
	@cd libunix_src/common; $(MAKE) clean "INC=$(INC)" "LIB=$(LIB)" \
	"SHARE=$(SHARE)" "CC=$(CC)" "CFLAGS=$(CFLAGS)" "RANLIB=$(RANLIB)" \
	"SHARE_RULES=$(SHARE_RULES)" "SYSLIBS=$(SYSLIBS)"

make_libscript:
	@echo ' ';echo 'Building lib/libscript.a'; echo ' '
	@cd libscript_src; $(MAKE) "INC=$(INC)" "LIB=$(LIB)" \
	"SHARE=$(SHARE)" "CC=$(CC)" "CFLAGS=$(CFLAGS)" "RANLIB=$(RANLIB)" \
	"SYSLIBS=$(SYSLIBS)"

clean_libscript:
	@cd libscript_src; $(MAKE) clean "INC=$(INC)" "LIB=$(LIB)" \
	"SHARE=$(SHARE)" "CC=$(CC)" "CFLAGS=$(CFLAGS)" "RANLIB=$(RANLIB)" \
	"SYSLIBS=$(SYSLIBS)"	

make_libmonitor:
	@echo ' ';echo 'Building lib/libmonitor.a'; echo ' '
	@cd libmonitor_src; $(MAKE) "INC=$(INC)" "LIB=$(LIB)" "SHARE=$(SHARE)" \
	"CC=$(CC)" "CFLAGS=$(CFLAGS)" "RANLIB=$(RANLIB)" \
	"SYSLIBS=$(SYSLIBS)"

clean_libmonitor:
	@cd libmonitor_src; $(MAKE) clean "INC=$(INC)" "LIB=$(LIB)" \
	"SHARE=$(SHARE)" "CC=$(CC)" "CFLAGS=$(CFLAGS)" "RANLIB=$(RANLIB)" \
	"SYSLIBS=$(SYSLIBS)"

make_control:
	@echo ' ';echo 'Building control specific'; echo ' '
	@cd control_src/$(SPECIFIC); $(MAKE) "BIN=$(BIN)" "INC=$(INC)" \
	"LIB=$(LIB)" "SHARE=$(SHARE)" "CC=$(CC)" "CFLAGS=$(CFLAGS)" \
	"RANLIB=$(RANLIB)" "SYSLIBS=$(SYSLIBS)"
	@echo ' ';echo 'Building control common'; echo ' '
	@cd control_src/common; $(MAKE) "BIN=$(BIN)" "INC=$(INC)" \
	"LIB=$(LIB)" "SHARE=$(SHARE)" "CC=$(CC)" "CFLAGS=$(CFLAGS)" \
	"RANLIB=$(RANLIB)" "SYSLIBS=$(SYSLIBS)"

clean_control:
	@cd control_src/$(SPECIFIC); $(MAKE) clean "BIN=$(BIN)" "INC=$(INC)" \
	"LIB=$(LIB)" "SHARE=$(SHARE)" "CC=$(CC)" "CFLAGS=$(CFLAGS)" \
	"RANLIB=$(RANLIB)" "SYSLIBS=$(SYSLIBS)"
	if [ -d control_src/$(SPECIFIC)/Test ] ; then (cd control_src/$(SPECIFIC)/Test; $(MAKE) clean); fi ;
	@cd control_src/common; $(MAKE) clean "BIN=$(BIN)" "INC=$(INC)" \
	"LIB=$(LIB)" "SHARE=$(SHARE)" "CC=$(CC)" "CFLAGS=$(CFLAGS)" \
	"RANLIB=$(RANLIB)" "SYSLIBS=$(SYSLIBS)"
	if [ -d control_src/common/Test ] ; then (cd control_src/common/Test; $(MAKE) clean); fi ;

make_viewer:
	@cd viewer_src; $(MAKE) "HELP=$(HELP)" "BIT=$(BIT)" "BIN=$(BIN)" \
       "INC=$(INC)" "LIB=$(LIB)" "SHARE=$(SHARE)" "CC=$(CC)" \
	"CFLAGS=$(CFLAGS) $(TK_INC) -I$(PGINC)" "FC=$(FC)" "RANLIB=$(RANLIB)" \
	"PGLIB=$(PGLIB)" "PGINC=$(PGINC)" "PGLD=$(PGLD)" "TKLD=$(TKLD)" \
	"X11LD=$(X11LD)" "CLIB=$(CLIB)" "FC_STATIC=$(FC_STATIC)" \
	"FC_DYNAMIC=$(FC_DYNAMIC)" "FC_PG_RPATH=$(FC_PG_RPATH)" \
	"SYSLIBS=$(SYSLIBS)" "DLFLAGS=$(DLFLAGS)"

clean_viewer:
	@cd viewer_src; $(MAKE) clean "HELP=$(HELP)" "BIT=$(BIT)" \
	"BIN=$(BIN)" "INC=$(INC)" "LIB=$(LIB)" "SHARE=$(SHARE)" "CC=$(CC)" \
	"CFLAGS=$(CFLAGS) $(TK_INC) -I$(PGLIB)" "FC=$(FC)" "RANLIB=$(RANLIB)" \
	"PGLIB=$(PGLIB)" "PGINC=$(PGINC)" "PGLD=$(PGLD)" "TKLD=$(TKLD)" \
	"X11LD=$(X11LD)" "CLIB=$(CLIB)" "FC_STATIC=$(FC_STATIC)" \
	"FC_DYNAMIC=$(FC_DYNAMIC)" "FC_PG_RPATH=$(FC_PG_RPATH)" \
	"SYSLIBS=$(SYSLIBS)" "DLFLAGS=$(DLFLAGS)"

make_supplier:
	@echo ' ';echo 'Building bin/supplier'; echo ' '
	@cd supplier_src; $(MAKE) "BIN=$(BIN)" "INC=$(INC)" "LIB=$(LIB)" \
	"SHARE=$(SHARE)" "CC=$(CC)" "CFLAGS=$(CFLAGS)" "RANLIB=$(RANLIB)" \
	"SYSLIBS=$(SYSLIBS)"

clean_supplier:
	@cd supplier_src; $(MAKE) clean "BIN=$(BIN)" "INC=$(INC)" "LIB=$(LIB)" \
	"SHARE=$(SHARE)" "CC=$(CC)" "CFLAGS=$(CFLAGS)" "RANLIB=$(RANLIB)" \
	"SYSLIBS=$(SYSLIBS)"

make_libtclcontrol:
	@echo ' ';echo 'Building lib/libtclcontrol.a and lib/libtclcontrol.so';\
	echo ' '
	@cd libtclcontrol_src; $(MAKE) "INC=$(INC)" "LIB=$(LIB)" \
	"SHARE=$(SHARE)" "CC=$(CC)" "CFLAGS=$(CFLAGS)" "RANLIB=$(RANLIB)" \
	"TKLD=$(TKLD)" "X11LD=$(X11LD)" "DLFLAGS=$(DLFLAGS)" \
	"SYSLIBS=$(SYSLIBS)"

clean_libtclcontrol:
	@cd libtclcontrol_src; $(MAKE) clean "INC=$(INC)" "LIB=$(LIB)" \
	"SHARE=$(SHARE)" "CC=$(CC)" "CFLAGS=$(CFLAGS)" "RANLIB=$(RANLIB)" \
	"TKLD=$(TKLD)" "X11LD=$(X11LD)" "DLFLAGS=$(DLFLAGS)" \
	"SYSLIBS=$(SYSLIBS)"

make_misc:
	@echo ' ';echo 'Building programs in misc_src'; echo ' '
	@cd misc_src; $(MAKE) "BIN=$(BIN)" "INC=$(INC)" "LIB=$(LIB)" \
	"SHARE=$(SHARE)" "CC=$(CC)" "CFLAGS=$(CFLAGS) -I$(PGLIB)" "FC=$(FC)" \
	"RANLIB=$(RANLIB)" "PGLIB=$(PGLIB)" "PGINC=$(PGINC)" "PGLD=$(PGLD)" \
	"X11LD=$(X11LD)" "SYSLIBS=$(SYSLIBS)" "CODE=$(CODE)"

clean_misc:
	@cd misc_src; $(MAKE) clean "BIN=$(BIN)" "INC=$(INC)" "LIB=$(LIB)" \
	"SHARE=$(SHARE)" "CC=$(CC)" "CFLAGS=$(CFLAGS) -I$(PGLIB)" "FC=$(FC)" \
	"RANLIB=$(RANLIB)" "PGLIB=$(PGLIB)" "PGINC=$(PGINC)" "PGLD=$(PGLD)" \
	"X11LD=$(X11LD)" "SYSLIBS=$(SYSLIBS)" "CODE=$(CODE)"

make_tcl_lib:
	@echo ' ';echo 'Installing TCL scripts from tcl_lib_src'; echo ' '
	@cd tcl_lib_src; $(MAKE) "LIB=$(LIB)"


clean_tcl_lib:
	@cd tcl_lib_src; $(MAKE) clean "LIB=$(LIB)"

clean_depend:
	@cd libtransaction_src; $(MAKE) clean_depend
	@cd libsrc_src;         $(MAKE) clean_depend
	@cd libscan_src;        $(MAKE) clean_depend
	@cd libscript_src;      $(MAKE) clean_depend
	@cd libmonitor_src;     $(MAKE) clean_depend
	@cd libtclcontrol_src;  $(MAKE) clean_depend
	@cd libtransaction_src; $(MAKE) clean_depend
	@cd viewer_src;         $(MAKE) clean_depend
	@cd control_src/common; $(MAKE) clean_depend
	if [ -d control_src/common ] ; then (cd control_src/common; $(MAKE) clean_depend); fi ;
	if [ -d control_src/common/Test ] ; then (cd control_src/common/Test; $(MAKE) clean_depend); fi ;
	if [ -d control_src/$(SPECIFIC) ] ; then (cd control_src/$(SPECIFIC); $(MAKE) clean_depend); fi ;
	if [ -d control_src/$(SPECIFIC)/Test ] ; then (cd control_src/$(SPECIFIC)/Test; $(MAKE) clean_depend); fi ;
	if [ -d libunix_src/common ] ; then (cd libunix_src/common; $(MAKE) clean_depend); fi ;
	if [ -d libunix_src/$(SPECIFIC) ] ; then (cd libunix_src/$(SPECIFIC); $(MAKE) clean_depend); fi ;


clean_code: clean_libsla clean_libunix clean_libscan clean_libsrc \
	clean_libscript clean_libmonitor clean_control clean_libtclcontrol \
	clean_viewer clean_supplier clean_misc clean_tcl_lib \
	clean_libtransaction



test:
	@cd control_src/common; $(MAKE) test
