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

GCPLIB = $(CARMA)/sza/lib

#-----------------------------------------------------------------------
# List the default targets.
#-----------------------------------------------------------------------

all: $(BIN)/supplier

#-----------------------------------------------------------------------
# Source file dependencies.
#-----------------------------------------------------------------------

supplier.o: $(SHARE)/netbuf.h $(SHARE)/tcpip.h $(SHARE)/szaregs.h \
 $(SHARE)/scanner.h $(SHARE)/szaconst.h $(SHARE)/rtcnetcoms.h \
 $(SHARE)/astrom.h

#-----------------------------------------------------------------------
# Program generation.
#-----------------------------------------------------------------------

SUPPLIER_OBJ = supplier.o

$(BIN)/supplier: $(SUPPLIER_OBJ)
	$(CC) -o $@ $(SUPPLIER_OBJ) $(SYSLIBS) -L$(GCPLIB) -lSzaSla -lSzaControlShare -lSzaUtil -lSzaMonitor -lrt -lreadline -ltermcap

clean:
	/bin/rm -f supplier.o $(BIN)/supplier
