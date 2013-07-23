# Exclude szaRegDoc from the generic binary list.  We will make this
# separately below

DFT=tDft
$(DFT): tDft.o 
	$(FC) -I$(CBASS_TOOLS)/include $(FC_FLAGS) -o $@ tDft.o \
	$(LIBS) $(PGLD) -ltkpgplot $(TKLD) $(X11LD) -lXmu

CROSSCORR=tCrossCorrelation
$(CROSSCORR): tCrossCorrelation.o 
	$(FC) $(FC_FLAGS) -o $@ tCrossCorrelation.o \
	$(LIBS) $(PGLD) -ltkpgplot $(TKLD) $(X11LD) -lXmu

BINEXC = $(CROSSCORR)

# And redefine bins to make both the generic binaries and RegDoc

test: generic_test $(BINEXC)

include $(GCPTOP)/Makefile_generic

