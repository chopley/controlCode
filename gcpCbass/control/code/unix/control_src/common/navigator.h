#ifndef navigator_h
#define navigator_h

#ifndef genericcontrol_h
typedef struct Navigator Navigator;
#endif

#include "source.h"
#include "scan.h"

#include "gcp/util/common/AntNum.h"
#include "gcp/util/common/Tracking.h"

#include <vector>

int nav_print_scan_info(Navigator *nav, char *name, int resolve, 
			gcp::control::ScanId *id);
int nav_get_scan_del(Navigator *nav, char *name, int resolve, 
		     gcp::control::ScanId *id, unsigned nreps, unsigned& ms);
int nav_lookup_source(Navigator *nav, char *name, int resolve, 
		      gcp::control::SourceId *id);

std::vector<std::pair<gcp::control::SourceId, gcp::util::AntNum::Id> > 
navLookupSourceExtended(Navigator *nav, char *name, 
			gcp::util::Tracking::Type type, 
			unsigned antennas, int resolve);

int nav_source_info(Navigator *nav, char *name, double utc,
		    double horizon, unsigned options, SourceInfo *info);

int nav_source_info(Navigator *nav, unsigned number, double utc,
		    double horizon, unsigned options, SourceInfo *info);

int nav_lookup_scan(Navigator *nav, char *name, int resolve, 
		      gcp::control::ScanId *id);

int nav_pmac_done(Navigator *nav);

int nav_track_source(Navigator *nav, char *name, 
		     gcp::util::Tracking::Type type, 
		     unsigned antennas, unsigned seq);

int nav_start_scan(Navigator *nav, char *name, unsigned nreps, unsigned seq,
		   bool add=false);

int nav_slew_telescope(Navigator *nav, unsigned mask,
		       double az, double el, double dk, 
		       unsigned antennas, unsigned seq);

int nav_halt_telescope(Navigator *nav, 
		       unsigned antennas, unsigned seq);

bool navIsCurrent(std::string name);

void readSourceCatalog(ControlProg* cp, std::string dir, std::string cat);
void readScanCatalog(ControlProg* cp, std::string dir, std::string cat);

#endif
