#ifndef GCP_UTIL_DLPUSBTHERMAL_H
#define GCP_UTIL_DLPUSBTHERMAL_H

/**
 * @file DlpUsbtTermal.h
 *
 * Tagged: Tue Oct 16 13:01:01 PDT 2007
 *
 * @version: $Revision: 1.1 $, $Date: 2009/09/29 21:09:56 $
 *
 * @author username: Command not found.
 */
#include "gcp/util/common/CondVar.h"
#include "gcp/util/common/SerialClient.h"
#include "gcp/util/common/SpawnableTask.h"
#include "gcp/util/common/GenericTaskMsg.h"
#include "gcp/util/common/TimeOut.h"

#include "gcp/util/common/DlpUsbThermalMsg.h"

#include <sstream>

namespace gcp {
  namespace util {

    class DlpUsbThermal {
    public:

      /**
       * Constructor
       */
      DlpUsbThermal(); //done

      /**
       * Destructor
       */
      ~DlpUsbThermal();  //done


      /**
       *  Connect to the port
       *
       *  Returns true on success
       */ 
      bool connect(); //done

      /**
       *  Disconnect 
       */ 
      void disconnect(); //done

      /**
       *  returns true if connected
       */ 
      bool isConnected(); //done
      
      /**
       *  write message to the port
       */ 
      int writeString(std::string message); //done

      /** 
       *  Packs our request
       */ 
      DlpUsbThermalMsg packCommand(DlpUsbThermalMsg::Request req, int input); // done

      /**
       * Sends the command
       */ 
      void sendCommand(DlpUsbThermalMsg& msg);  // done

      /**
       *  Sends a command and reads the response if necessary
       */ 
      DlpUsbThermalMsg issueCommand(DlpUsbThermalMsg::Request req, int input); //done
      DlpUsbThermalMsg issueCommand(DlpUsbThermalMsg::Request req, int input, bool withQ); //done

      /**
       *  Command set
       */ 
      void setupDefault();
      void setOutputType(int outType=1);   // 0 for binary, all else Ascii
      void setOutputUnits(int unitType=1); // 0 for F, all else C.
      float queryTemperature(int channel);  // channels 1 through 8
      float queryVoltage(int channel);  // channels 1 through 8
      std::vector<float> queryAllTemps();
      std::vector<float> queryAllVoltages();

    private:

      /**
       * The file descriptor associated with the Usb device
       */
      int fd_;

      /**
       * The set of file descriptors to be watched for readability.
       */
      gcp::util::FdSet fdSet_;

      /**
       * True when we have a usb connection to the temp monitor
       */
      bool connected_;

    public:
      /** 
       * Read bytes from the port.
       */ 
      int readPort(DlpUsbThermalMsg& msg); //done
      int readPort(DlpUsbThermalMsg& msg, bool withQ); //done

    private:
      /**
       * Block, waiting for input from the device.
       */
      void waitForResponse(); //done

      /**
       * Parse Response
       */ 
      void parseResponse(DlpUsbThermalMsg& msg);  //done
      void parseResponse(DlpUsbThermalMsg& msg, bool withQ);  //done

      /**
       * Original serial terminal settings
       */
      struct termios termioSave_;

      void print_bits(unsigned char feature);


    }; //End class DlpUsbThermal

  }; //End namespace util
}; // End namesapce gcp

#endif // End #ifndef

