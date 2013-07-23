// $Id: ImageReader.h,v 1.1.1.1 2009/07/06 23:57:23 eml Exp $

#ifndef GCP_GRABBER_IMAGEREADER_H
#define GCP_GRABBER_IMAGEREADER_H

/**
 * @file ImageReader.h
 * 
 * Tagged: Mon Jun 11 13:07:31 PDT 2007
 * 
 * @version: $Revision: 1.1.1.1 $, $Date: 2009/07/06 23:57:23 $
 * 
 * @author username: Command not found.
 */

#include <iostream>

#include "gcp/pgutil/common/ImagePlotter.h"

#include "gcp/control/code/unix/libmonitor_src/im_monitor_stream.h"

namespace gcp {
  namespace grabber {

    class ImageReader : public ImagePlotter {
    public:

      /**
       * Constructor.
       */
      ImageReader();

      /**
       * Destructor.
       */
      virtual ~ImageReader();

      // Read an image from the stream

      virtual ImsReadState read();

      void changeStream(ImMonitorStream* ims);
      void openStream(std::string host);

      int fd() {
	return ims_select_fd(ims_);
      }

    private:

      ImMonitorStream* ims_;  // An image monitor-data stream or NULL
			      // if not assigned

    }; // End class ImageReader

  } // End namespace grabber
} // End namespace gcp



#endif // End #ifndef GCP_GRABBER_IMAGEREADER_H
