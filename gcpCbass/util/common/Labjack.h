#ifndef GCP_UTIL_LABJACK_H
#define GCP_UTIL_LABJACK_H

/**
 * @file DlpUsbtTermal.h
 *
 * Tagged: Tue Oct 16 13:01:01 PDT 2007
 *
 * @version: $Revision: 1.2 $, $Date: 2013/01/17 19:17:10 $
 *
 * @author username: Command not found.
 */
#include "gcp/util/common/CondVar.h"
#include "gcp/util/common/SerialClient.h"
#include "gcp/util/common/SpawnableTask.h"
#include "gcp/util/common/GenericTaskMsg.h"
#include "gcp/util/common/TimeOut.h"

#include "gcp/util/common/LabjackMsg.h"
#include "gcp/util/common/LabjackU3.h"

#include <sstream>

namespace gcp {
  namespace util {

    class Labjack {
    public:

      /**
       * Constructor
       */
      Labjack(); //done

      /**
       * Destructor
       */
      ~Labjack();  //done


      /**
       *  Connect to the port
       *
       *  Returns true on success
       */ 
      bool connect(int serialNumber); //done

      /**
       *  Disconnect 
       */ 
      void disconnect(); //done

      /**
       *  returns true if connected
       */ 
      bool isConnected(); //done
     
	//function to update the calibration information of the U3
      long getCalibrationInfo(); 

      //and a function to configure the IO of the ADC
      int configAllIO();
      //function to read in the data from the Labjack
      int AllIO();
      /**
       *  write message to the port
       */ 
      int writeString(std::string message); //done

      /** 
       *  Packs our request
       */ 
      LabjackMsg packCommand(LabjackMsg::Request req, int input); // done

      /**
       *  Serial number
       */  
      int serial_;

      /**
       * Sends the command
       */ 
      void sendCommand(LabjackMsg& msg);  // done

      /**
       *  Sends a command and reads the response if necessary
       */ 
      LabjackMsg issueCommand(LabjackMsg::Request req, int input); //done
      LabjackMsg issueCommand(LabjackMsg::Request req, int input, bool withQ); //done

      std::vector<float> queryAllVoltages();
      
      bool connected_;
    private:
      
      /**
       * The file descriptor associated with the Usb device
       */
      int fd_;
      uint8 numChannels_;
      uint8 quickSample_;
      uint8 longSettling_;
      /*this is a handle to the labjack device*/
      HANDLE hDevice_;
      
      //calibration info pointer for the U3 device
      u3CalibrationInfo caliInfo_;	
      /**
       * The set of file descriptors to be watched for readability.
       */
      gcp::util::FdSet fdSet_;
      
      /**
       * True when we have a usb connection to the temp monitor
       */
      
    public:
      /** 
       * Read bytes from the port.
       */ 
      int readPort(LabjackMsg& msg); //done
      int readPort(LabjackMsg& msg, bool withQ); //done
      
    private:
      /**
       * Block, waiting for input from the device.
       */
      void waitForResponse(); //done

      /**
       * Parse Response
       */ 
      void parseResponse(LabjackMsg& msg);  //done
      void parseResponse(LabjackMsg& msg, bool withQ);  //done

      /**
       * Original serial terminal settings
       */
      struct termios termioSave_;

      void print_bits(unsigned char feature);


    }; //End class DlpUsbThermal

  }; //End namespace util
}; // End namesapce gcp

#endif // End #ifndef

