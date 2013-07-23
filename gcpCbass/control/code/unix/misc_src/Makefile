# This file is designed to b invoked from a recursive call to make
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
#  FC        - The FORTRAN compiler to use.
#  CFLAGS    - Flags for the C compiler.
#  RANLIB    - The archiver randomizer command (use : if not needed).
#  PGLIB     - The directory that contains libpgplot.
#  PGINC     - The directory that contains pgplot.h
#-----------------------------------------------------------------------

#-----------------------------------------------------------------------
# List the default targets.
#-----------------------------------------------------------------------

all: $(GCPBIN)/azel $(GCPBIN)/logfile

#all: $(BIN)/read_archive $(BIN)/check_encoder $(BIN)/check_az_encoder \
# $(BIN)/backup $(BIN)/clocksync $(BIN)/tjp_pointing $(BIN)/azel $(BIN)/pmacterm

clean: clean_o clean_prog

#-----------------------------------------------------------------------
# Source file dependencies.
#-----------------------------------------------------------------------

read_archive.o: $(INC)/monitor_stream.h

check_encoder.o: $(INC)/monitor_stream.h $(SHARE)/lprintf.h \
 $(PGINC)/cpgplot.h $(SHARE)/szaconst.h

check_az_encoder.o: $(INC)/monitor_stream.h $(SHARE)/lprintf.h \
 $(PGINC)/cpgplot.h $(SHARE)/szaconst.h

backup.o: $(SHARE)/hash.h $(SHARE)/input.h $(SHARE)/pathname.h

tjp_pointing.o: $(INC)/monitor_stream.h $(PGINC)/cpgplot.h date.h color_tab.h

date.o: date.h $(SHARE)/astrom.h

command_interp.o: readline.h history.h

clocksync.o: $(SHARE)/netbuf.h $(SHARE)/tcpip.h \
 $(SHARE)/clocksync.h $(SHARE)/astrom.h $(SHARE)/szaconst.h

clean_o:
	#/bin/rm -f *.o

#-----------------------------------------------------------------------
# Program generation.
#-----------------------------------------------------------------------

$(GCPBIN)/azel: azel.o 	
	$(CC) -o $@ azel.o -lm

$(GCPBIN)/logfile: logfile.o coproc.o
	$(CC) -o $@ logfile.o coproc.o

$(GCPBIN)/pmacterm: pmacterm.o 	
	$(CC) -o $@ pmacterm.o -lreadline -ltermcap

ARC_OBJ = read_archive.o

$(BIN)/read_archive: $(ARC_OBJ) $(LIB)/libunix.a $(LIB)/libmonitor.a \
 $(GCPLIB)/libSla.a
	$(CC) -o $@ $(ARC_OBJ) -L$(GCPLIB) -lSla -L$(LIB) -lmonitor -lunix $(SYSLIBS)

ENC_OBJ = check_encoder.o

$(BIN)/check_encoder: $(ENC_OBJ) $(LIB)/libunix.a $(LIB)/libmonitor.a \
 $(GCPLIB)/libSla.a $(PGLIB)/libcpgplot.a $(PGLIB)/libpgplot.so
	$(FC) -o $@ $(ENC_OBJ) -L$(GCPLIB) -lSla -L$(LIB) -lmonitor -lunix \
$(PGLD) $(X11LD) $(SYSLIBS)

AZ_ENC_OBJ = check_az_encoder.o

$(BIN)/check_az_encoder: $(AZ_ENC_OBJ) $(LIB)/libunix.a $(LIB)/libmonitor.a \
 $(GCPLIB)/libSla.a $(PGLIB)/libcpgplot.a $(PGLIB)/libpgplot.so
	$(FC) -o $@ $(AZ_ENC_OBJ) -L$(GCPLIB) -lSla -L$(LIB) -lmonitor -lunix \
$(PGLD) $(X11LD) $(SYSLIBS)

BACKUP_OBJ = backup.o

$(BIN)/backup: $(BACKUP_OBJ) $(LIB)/libunix.a
	$(CC) -o $@ $(BACKUP_OBJ) -L$(LIB) -lunix $(SYSLIBS)

TJP_POINT_OBJ = tjp_pointing.o date.o random.o command_interp.o filestuff.o

$(BIN)/tjp_pointing: $(TJP_POINT_OBJ) $(LIB)/libunix.a $(LIB)/libmonitor.a \
 $(GCPLIB)/libSla.a $(PGLIB)/libcpgplot.a $(PGLIB)/libpgplot.so
	$(FC) -o $@ $(TJP_POINT_OBJ) -L$(GCPLIB)-lSla -L$(LIB) -lmonitor -lunix \
$(PGLD) $(X11LD) $(SYSLIBS) -lreadline -ltermcap

CLOCK_OBJ = clocksync.o

$(BIN)/clocksync: $(CLOCK_OBJ) $(LIB)/libunix.a $(GCPLIB)/libSla.a
	$(CC) -o $@ $(CLOCK_OBJ) -L$(GCPLIB) -lSla -L$(LIB) -lunix $(SYSLIBS)

clean_prog: 
	/bin/rm -f $(BIN)/read_archive 
	/bin/rm -f $(BIN)/check_encoder 
	/bin/rm -f $(BIN)/check_az_encoder 
	/bin/rm -f $(BIN)/backup 
	/bin/rm -f $(BIN)/clocksync 
	/bin/rm -f $(BIN)/tjp_pointing 
	/bin/rm -f $(BIN)/azel 
	/bin/rm -f $(BIN)/pmacterm

#-----------------------------------------------------------------------
# Local include file dependencies.
#-----------------------------------------------------------------------

