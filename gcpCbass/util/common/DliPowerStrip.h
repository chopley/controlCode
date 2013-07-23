// $Id: DliPowerStrip.h,v 1.2 2010/11/24 00:46:01 sjcm Exp $

#ifndef GCP_UTIL_DLIPOWERSTRIP_H
#define GCP_UTIL_DLIPOWERSTRIP_H

/**
 * @file DliPowerStrip.h
 * 
 * Tagged: Tue Oct 27 14:19:56 PDT 2009
 * 
 * @version: $Revision: 1.2 $, $Date: 2010/11/24 00:46:01 $
 * 
 * @author tcsh: username: Command not found.
 */
#include <vector>

namespace gcp {
  namespace util {

    class DliPowerStrip {
    public:

      enum State {
	UNKNOWN = 0x0,
	OFF     = 0x1,
	ON      = 0x2,
      };

      enum Outlet {
	OUTLET_NONE =  0x0,
	OUTLET_1    =  0x1,
	OUTLET_2    =  0x2,
	OUTLET_3    =  0x4,
	OUTLET_4    =  0x8,
	OUTLET_5    = 0x10,
	OUTLET_6    = 0x20,
	OUTLET_7    = 0x40,
	OUTLET_8    = 0x80,
      };

      static const unsigned MAX_OUTLETS;

      /**
       * Constructor.
       */
      DliPowerStrip(std::string host);

      /**
       * Destructor.
       */
      virtual ~DliPowerStrip();

      // Turn an outlet on

      void on(unsigned outlet);

      // Turn a set of outlets on

      void on(Outlet outletMask);

      // Turn an outlet off

      void off(unsigned outlet);

      // Turn a set of outlets off

      void off(Outlet outletMask);

      // Cycle power to an outlet

      void cycle(unsigned outlet);

      // Cycle power to a set of outlets

      void cycle(Outlet outletMask);

      // Turn all outlets on

      void allOn();

      // Turn all outlets off

      void allOff();

      // Cycle power to all outlets

      void cycleAll();
      
      // Query the status of all outlets

      std::vector<State> queryStatus();

    private:

      std::vector<State> state_;
      std::string host_;

      void checkOutlet(unsigned outlet);
      std::string sendRequest(std::string str);

    }; // End class DliPowerStrip

  } // End namespace util
} // End namespace gcp



#endif // End #ifndef GCP_UTIL_DLIPOWERSTRIP_H
