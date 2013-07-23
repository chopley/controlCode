
UNIX_BIN  = $(GCPDIR)/code/unix/bin
UNIX_LIB  = $(GCPDIR)/code/unix/lib
UNIX_HELP = $(GCPDIR)/code/unix/help
VXW_BIN   = $(GCPDIR)/code/vxworks/bin

#all: bin lib help bin/dasiviewer bin/tkviewer bin/szacontrol \
#     lib/page.tcl lib/cmd.tcl bin/rtc bin/probe bin/fixSio \
#     bin/tjp_pointing bin/azel bin/pmacterm install-help

all: bin lib help bin/szaviewer bin/tkviewer bin/szacontrol \
     lib/page.tcl lib/cmd.tcl \
     install-help

clean:
	/bin/rm -rf bin
	/bin/rm -rf lib
	/bin/rm -rf help

bin:
	mkdir bin

lib:
	mkdir lib

help:
	mkdir help

bin/tkviewer: $(UNIX_BIN)/tkviewer
	if [ -f $@ ] ; then mv -f $@ $(@D)/old_$(@F); fi
	cp -p $(UNIX_BIN)/tkviewer $@

bin/szacontrol: $(UNIX_BIN)/szacontrol
	if [ -f $@ ] ; then mv -f $@ $(@D)/old_$(@F); fi
	cp -p $(UNIX_BIN)/szacontrol $@

lib/page.tcl: $(UNIX_LIB)/page.tcl
	if [ -f $@ ] ; then mv -f $@ $(@D)/old_$(@F); fi
	cp -p $(UNIX_LIB)/page.tcl $@

lib/cmd.tcl: $(UNIX_LIB)/cmd.tcl
	if [ -f $@ ] ; then mv -f $@ $(@D)/old_$(@F); fi
	cp -p $(UNIX_LIB)/cmd.tcl $@

bin/rtc: $(VXW_BIN)/rtc
	if [ -f $@ ] ; then mv -f $@ $(@D)/old_$(@F); fi
	cp -p $(VXW_BIN)/rtc $@

bin/probe: $(VXW_BIN)/probe
	if [ -f $@ ] ; then mv -f $@ $(@D)/old_$(@F); fi
	cp -p $(VXW_BIN)/probe $@

bin/fixSio: $(VXW_BIN)/fixSio
	if [ -f $@ ] ; then mv -f $@ $(@D)/old_$(@F); fi
	cp -p $(VXW_BIN)/fixSio $@

bin/szaviewer: $(UNIX_BIN)/szaviewer
	if [ -f $@ ] ; then mv -f $@ $(@D)/old_$(@F); fi
	sed < $(UNIX_BIN)/szaviewer "s|$(GCPDIR)/code/unix|`pwd`|g" > $@
	chmod ug+x $@

bin/tjp_pointing: $(UNIX_BIN)/tjp_pointing
	if [ -f $@ ] ; then mv -f $@ $(@D)/old_$(@F); fi
	cp -p $(UNIX_BIN)/tjp_pointing $@

bin/azel: $(UNIX_BIN)/azel
	if [ -f $@ ] ; then mv -f $@ $(@D)/old_$(@F); fi
	cp -p $(UNIX_BIN)/azel $@

bin/pmacterm: $(UNIX_BIN)/pmacterm
	if [ -f $@ ] ; then mv -f $@ $(@D)/old_$(@F); fi
	cp -p $(UNIX_BIN)/pmacterm $@

install-help:
	rsync -tr --delete --exclude CVS $(UNIX_HELP)/* help
