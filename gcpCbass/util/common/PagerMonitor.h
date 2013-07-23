// $Id: PagerMonitor.h,v 1.2 2010/02/23 17:19:58 eml Exp $

#ifndef GCP_UTIL_PAGERMONITOR_H
#define GCP_UTIL_PAGERMONITOR_H

/**
 * @file PagerMonitor.h
 * 
 * Tagged: Thu Oct 26 13:35:18 PDT 2006
 * 
 * @version: $Revision: 1.2 $, $Date: 2010/02/23 17:19:58 $
 * 
 * @author username: Command not found.
 */
#include "gcp/util/common/ArrayDataFrameManager.h"
#include "gcp/util/common/MonitorCondition.h"
#include "gcp/util/common/MonitorPoint.h"
#include "gcp/util/common/MonitorPointManager.h"
#include "gcp/util/common/RegDescription.h"

#include <string>
#include <vector>

namespace gcp {
  namespace util {

    /**
     * A class for monitoring registers to activate the pager.  
     *
     * The idea is to re-use the MonitorPointManager infrastructure
     * that already exists for Can packet monitoring.  This allows the
     * user to register conditions to be checked when a register is
     * written to, and to register handlers to be called when those
     * conditions are met.  Because these conditions are checked when
     * registers are written, this is implemented here by copying the
     * corresponding registers from the latest data frame into a dummy
     * frame.  
     */
    class PagerMonitor {
    public:

      /**
       * A struct for handling information about a single register
       */
      struct RegSpec {

	RegSpec(std::string name, 
		double min=0, double max=0, bool isDelta=false, 
		unsigned nFrame=PagerMonitor::defaultNframe_, 
		bool isOutOfRange=false);

	RegSpec(ArrayDataFrameManager* fm, 
		RegDescription& desc, MonitorPoint* mp, bool outOfRange, MonitorPointManager& mpm);

	RegSpec(RegSpec& regSpec);

	RegSpec(const RegSpec& regSpec);

	void operator=(RegSpec& regSpec);
	void operator=(const RegSpec& regSpec);
	
	// Output operator

	// A pointer to the  block descriptor for this register

	RegMapBlock* block_;

	// The range of this register

	CoordRange range_;

	// A pointer to the start of the data for this register in the
	// source data frame

	void* sourcePtr_;

	// A monitor point associated with this register

	MonitorPoint* mp_;

	// Other members needed for external pusposes

	std::string name_;
	bool isDelta_;
	bool isOutOfRange_;
	unsigned nFrame_;
	double min_;
	double max_;

	// A method for copying this register into the destination
	// frame

	void copyRegister();
      };

      /**
       * Constructor.
       */
      PagerMonitor();
      PagerMonitor(ArrayDataFrameManager* source);

      /**
       * Destructor.
       */
      virtual ~PagerMonitor();

      void setHandler(MONITOR_CONDITION_HANDLER(*handler), void* arg=0);

      /**
       * Methods to add a register to be monitored
       */

      // Add a monitor point to activate the pager when a register
      // goes out of the specified range

      std::vector<PagerMonitor::RegSpec>
	addOutOfRangeMonitorPoint(std::string regSpec, 
				  double min, double max, 
				  bool delta=false,
				  unsigned nFrame=defaultNframe_,
				  std::string script="");
      
      // Add a monitor point to activate the pager when a register
      // goes into the specified range

      std::vector<PagerMonitor::RegSpec>
	addInRangeMonitorPoint(std::string regSpec, 
			       double min, double max,
			       bool delta=false,
			       unsigned nFrame=defaultNframe_,
			       std::string script="");

      // And add monitor points associated with a register
      // specification

      std::vector<PagerMonitor::RegSpec>
	addMonitorPoint(std::string regSpec, 
			DataTypeTruthFn fn,
			DataType& min,
			DataType& max,
			bool delta,
			unsigned nFrame,
			bool outOfRange,
			std::string script="");

      // Remove a monitor point

      void remMonitorPoint(std::string regSpec);

      // A method to call to check registers

      void checkRegisters();
      
      // Reset all monitor points

      void reset();

      // Clear this manager of all monitor points

      void clear();

      // List all monitor points

      void list();

      // Return a list of all monitor points

      std::vector<PagerMonitor::RegSpec> getRegs();

      // Return a list of all monitor points

      std::vector<std::string> getList(bool sort=false);

      // Return a formatted printout of a single monitor point

      std::vector<std::string> format(std::string regSpec);

      // Output operator

      friend std::ostream& operator<<(std::ostream& os, PagerMonitor& pm);
      friend std::ostream& operator<<(std::ostream& os, PagerMonitor::RegSpec& rs);

      // Lock/unlock this object

      inline void lock() {
	guard_.lock();
      }

      inline void unlock() {
	guard_.unlock();
      }

      // The default number of frames for which the condition must be
      // met before activating the pager

      static unsigned defaultNframe_;

    private:

      // A pointer to the data frame that is the source of the latest
      // data

      ArrayDataFrameManager* source_;

      // A destination frame, into which pager registers will be
      // copied.  This will be copy constructed from the input data
      // frame manager.

      ArrayDataFrameManager dest_;

      // The monitor point manager

      MonitorPointManager mpManager_;
      
      // A handler to be called when a monitor condition is met

      MONITOR_CONDITION_HANDLER(*handler_);

      void* arg_;

      // A vector of monitor points

      std::vector<PagerMonitor::RegSpec> registers_;

      // A mutex variable for protecting this object

      Mutex guard_;

    }; // End class PagerMonitor

  } // End namespace util
} // End namespace gcp



#endif // End #ifndef GCP_UTIL_PAGERMONITOR_H
