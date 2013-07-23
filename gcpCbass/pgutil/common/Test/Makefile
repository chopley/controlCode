IMAGEREADER=tVideoCapture
$(IMAGEREADER): tVideoCapture.o 
	g77 -o $@ tVideoCapture.o \
	$(LIBS) $(LIBDIR)/libGcpPgUtilCommon$(LIBSO_SUFFIX) $(PGLD) -ltkpgplot $(TKLD) $(X11LD) -lXmu


IMAGEREADER2=tVideoCaptureOrig
$(IMAGEREADER2): tVideoCaptureOrig.o 
	g77 -o $@ tVideoCaptureOrig.o \
	$(LIBS) $(LIBDIR)/libGcpPgUtilCommon$(LIBSO_SUFFIX) $(PGLD) -ltkpgplot $(TKLD) $(X11LD) -lXmu


VCE=tVideoCaptureEml
$(VCE): tVideoCaptureEml.o 
	g77 -o $@ tVideoCaptureEml.o \
	$(LIBS) $(LIBDIR)/libGcpPgUtilCommon$(LIBSO_SUFFIX) $(PGLD) -ltkpgplot $(TKLD) $(X11LD) -lXmu

VC=tVideoCapabilities
$(VC): tVideoCapabilities.o 
	g77 -o $@ tVideoCapabilities.o \
	$(LIBS) $(LIBDIR)/libGcpPgUtilCommon$(LIBSO_SUFFIX) $(PGLD) -ltkpgplot $(TKLD) $(X11LD) -lXmu

VC2=tVideoCapabilitiesOld
$(VC2): tVideoCapabilitiesOld.o 
	g77 -o $@ tVideoCapabilitiesOld.o \
	$(LIBS) $(LIBDIR)/libGcpPgUtilCommon$(LIBSO_SUFFIX) $(PGLD) -ltkpgplot $(TKLD) $(X11LD) -lXmu

VCE2=tVideoCapabilitiesEml
$(VCE2): tVideoCapabilitiesEml.o 
	g77 -o $@ tVideoCapabilitiesEml.o \
	$(LIBS) $(LIBDIR)/libGcpPgUtilCommon$(LIBSO_SUFFIX) $(PGLD) -ltkpgplot $(TKLD) $(X11LD) -lXmu

BINEXC = $(IMAGEREADER) $(VC) $(VCE) $(VCE2)

test: generic_test $(BINEXC)

include $(GCPTOP)/Makefile_generic
