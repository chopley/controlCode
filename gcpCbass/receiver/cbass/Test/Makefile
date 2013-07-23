CCINC += -I $(GCPTOP)/receiver/specific

DIOCOLUMN=tDIOColumn
$(DIOCOLUMN): $(DIOCOLUMN).o
	$(CC) -o $@ $(CCFLAGS) $(LDPATH) $(RPATH) $(LIBS) $(MUXDIR)/Utilities/libDIODevel.so $<

BINEXC = $(DIOCOLUMN)

# And redefine bins to make both the generic binaries and the excluded targets

test: generic_test $(BINEXC)

include $(GCPTOP)/Makefile_generic
