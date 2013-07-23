# This file is designed to be invoked from a recursive call to make
# from the makefile in SZA/code/unix/.
#
# The makefile that includes it is expected to define the following
# macros:
#
#  INC       - The full name of the SZA/code/unix/include directory.
#  LIB       - The full name of the SZA/code/unix/lib directory.
#  SHARE     - The full name of the SZA/code/share/include directory.
#  CC        - The C compiler to be used.
#  CFLAGS    - Flags for the C compiler.
#  RANLIB    - The archiver randomizer command (use : if not needed).
#  SLALIB    - The directory that contains the slalib source code.
#-----------------------------------------------------------------------

#-----------------------------------------------------------------------
# Generic rule for making .o files
#-----------------------------------------------------------------------

%.o: %.c
	$(CC) $(CFLAGS) $(INCFLAGS) -c -o $@ $<

#-----------------------------------------------------------------------
# List the default targets.
#-----------------------------------------------------------------------

LIBSO = $(GCPLIB)/libGcpSla$(LIBSO_SUFFIX)
all:  $(LIBSO) $(INC)/slalib.h

clean: clean_h clean_lib

#-----------------------------------------------------------------------
# List the slalib modules that we are interested in.
#-----------------------------------------------------------------------

SLA_OBJ = amp.o ampqk.o caldj.o cldj.o de2h.o djcl.o dat.o dt.o dmoon.o dtt.o \
  ecmat.o ecleq.o eqecl.o eqgal.o galeq.o gmst.o dimxv.o dranrm.o \
  eqeqx.o nutc.o geoc.o mapqkz.o map.o ranorm.o mappa.o prenut.o dvn.o \
  epj.o dcc2s.o evp.o dmxv.o dvdv.o prec.o dcs2c.o dmxm.o nut.o \
  deuler.o mapqk.o planet.o pvobs.o refco.o refro.o rdplan.o \
  drange.o fk45z.o fk54z.o pm.o fk524.o epb2d.o calyd.o clyd.o

#-----------------------------------------------------------------------
# Get the compilation rules.
#-----------------------------------------------------------------------

include $(SLALIB)/Makefile.rules

#-----------------------------------------------------------------------
# Library generation.
#-----------------------------------------------------------------------

$(LIBSO): $(SLA_OBJ)
	@if [ ! -d $(GCPLIB) ] ; then mkdir $(GCPLIB); fi;
	$(CC) $(LIBSO_FLAGS) -o $(LIBSO) $(SLA_OBJ)
	$(RANLIB) $@

clean_lib:
	/bin/rm -f $(SLA_OBJ) $(LIBSO)

#-----------------------------------------------------------------------
# Install the slalib include file in the shared include directory.
#-----------------------------------------------------------------------

$(INC)/slalib.h: $(SLALIB)/slalib.h
	cp $(SLALIB)/slalib.h $@

clean_h:
	/bin/rm -f $(INC)/slalib.h

