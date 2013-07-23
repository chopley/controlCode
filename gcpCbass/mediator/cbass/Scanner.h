#ifndef GCP_MEDIATOR_SCANNER_H
#define GCP_MEDIATOR_SCANNER_H

/**
 * @file Scanner.h
 * 
 * Tagged: Thu Nov 13 16:54:00 UTC 2003
 * 
 * @author Erik Leitch
 */
#include "gcp/util/specific/Directives.h"

#include "gcp/util/common/AntennaDataFrameManager.h"
#include "gcp/util/common/AntennaFrameBuffer.h"
#include "gcp/util/common/ArrayFrameBuffer.h"
#include "gcp/util/common/DataFrameManager.h"
#include "gcp/util/common/GenericTask.h"
#include "gcp/util/common/NetStr.h"
#include "gcp/util/common/TcpClient.h"

#include "gcp/util/specific/Directives.h"

#include "gcp/mediator/specific/ScannerMsg.h"

// Don't move this around: GCC3.2.2 is easily confused!

#include "gcp/control/code/unix/libunix_src/common/scanner.h"

namespace gcp {

  namespace util {
    class ArrayDataFrameManager;
    class BoardDataFrameManager;
  }

#if DIR_HAVE_MUX

  namespace receiver {
    class DioConsumer;
    class XMLConsumer;
  }

#endif

  namespace mediator {
    
    /**
     * Forward declarations let us use class pointers without defining
     * them.
     */
    class Master;
    class XMLConsumer;
    class AntennaConsumer;

    /**
     * A class to encapsulate data transmission from the AC to the ACC.
     */
    class Scanner :
      public gcp::util::GenericTask<ScannerMsg> {
      
      public:
      
      //------------------------------------------------------------
      // Scanner public members
      //------------------------------------------------------------
      
      /**
       * Destructor.
       */
      ~Scanner();
      
      /**
       * Pack a fake data frame for sending to the control
       * computer.  Automatically deals with the mutex lock on the
       * frame buffer.
       *
       * @throws Exception
       */
      void packAntennaFrame(gcp::util::AntennaDataFrameManager* frame);
      
#if DIR_HAVE_MUX
      /**
       * Pack a data frame from the receiver computer.  Automatically
       * deals with the mutex lock on the frame buffer.
       *
       * @throws Exception
       */
      void packBolometerFrame();
#endif

      /**
       * Pack a fake data frame for sending to the control
       * computer.  Automatically deals with the mutex lock on the
       * frame buffer.
       *
       * @throws Exception
       */
      void packDataFrame(gcp::util::RegMapDataFrameManager* frame, 
			 std::string regMapName);      
      
      /**.......................................................................
       * Pack a board into the shared array register map
       */
      void packBoard(gcp::util::BoardDataFrameManager* dataFrame);

      /**.......................................................................
       * Pack a board into the array register map of the frame
       * corresponding to the passed integer id
       */
      void packBoardSynch(gcp::util::BoardDataFrameManager* dataFrame, unsigned int id);

      /**
       * Method by which the scanner thread can be notified that a
       * bolometer data frame is ready to be packed.
       */
      void sendPackBoloDataFrameMsg();

      /**
       * Methods by which tasks can query std::strings pertinent to
       * the connection.
       */
      std::string dioHost();
      std::string hwHost();
      unsigned dioPort();
      unsigned hwPort();

      //------------------------------------------------------------
      // Scanner private members
      //------------------------------------------------------------
      
      private:

      // Needed so that  alone can instantiate this class
      
      friend class Master; 
      
      // Needed because AntennaConsumer will call our packFrame method.
      
      friend class AntennaConsumer;
      
      // Needed because DioConsumer will call our packFrame method.

      friend class DioConsumer;
      
      // Needed because XMLConsumer will call our packFrame method.
      
      friend class XMLConsumer;
      
      /**
       * An object for managing feature bits received from the control
       * program
       */
      struct {
	unsigned transient;
	unsigned persistent;
	unsigned seq;
      } features_;

      /**
       * Initialize antenna resources.
       */
      void initAntennaResources();
      
      //------------------------------------------------------------
      // Declare startup functions for threads managed by this class.
      //------------------------------------------------------------
      
      static THREAD_START(startAntennaConsumer);  
      static THREAD_START(startReceiverConfigConsumer);
      static THREAD_START(startPointingTelConsumer);
      static THREAD_START(startHoloConsumer);
      static THREAD_START(startSecondaryConsumer);
      static THREAD_START(startFridgeConsumer);
      static THREAD_START(startDeicingConsumer);

#if DIR_HAVE_MUX
      static THREAD_START(startBolometerConsumer);
      static THREAD_START(startSquidConsumer);
#endif
      
      //------------------------------------------------------------
      // Declare cleanup handlers for threads managed by this class.
      //------------------------------------------------------------
      
      static THREAD_CLEAN(cleanAntennaConsumer);
      static THREAD_CLEAN(cleanReceiverConfigConsumer);
      static THREAD_CLEAN(cleanPointingTelConsumer);
      static THREAD_CLEAN(cleanHoloConsumer);
      static THREAD_CLEAN(cleanSecondaryConsumer);
      static THREAD_CLEAN(cleanFridgeConsumer);
      static THREAD_CLEAN(cleanDeicingConsumer);

#if DIR_HAVE_MUX
      static THREAD_CLEAN(cleanBolometerConsumer);
      static THREAD_CLEAN(cleanSquidConsumer);
#endif

      /** 
       * A pointer to the parent.
       */
      Master* parent_;
      
      /**
       * The object which captures data frames from the antenna
       * computer
       */
      AntennaConsumer* antennaConsumer_;     
      
#if DIR_HAVE_MUX

      /**
       * The object which captures data frames from the bolometers.
       */
      gcp::receiver::DioConsumer* bolometerConsumer_;
      
      /**
       * The object which captures data frames from the squids.
       */
      gcp::receiver::DioConsumer* squidConsumer_;

      /**
       * The object which captures data frames from the receiver
       * hardware manager
       */
      gcp::receiver::XMLConsumer* receiverConfigConsumer_;
      
#endif

      /**
       * The object which captures data frames from the pointing
       * telescope computer hardware manager
       */
      XMLConsumer* deicingConsumer_;
      
      /**
       * The object which captures data frames from the pointing
       * telescope computer hardware manager
       */
      XMLConsumer* pointingTelConsumer_;
      
      /**
       * The object which captures data from the holography receiver
       * hardware manager
       */
      XMLConsumer* holoConsumer_;

      /**
       * The object which captures data from the secondary cryostat
       * monitoring hardware manager
       */
      XMLConsumer* secondaryConsumer_;
      
      /**
       * The object which captures data from the receiver cryostat
       * fridge monitoring hardware manager
       */
      XMLConsumer* fridgeConsumer_;
      
      /**
       * The array map
       */
      ArrayMap* arraymap_;     
      
      /**
       * The IP address of the control host
       */
      std::string host_;           
      
      /**
       * An object to manage our connection to the control program
       */
      gcp::util::TcpClient client_;
      
      /**
       * The TCP/IP network stream we will use to communicate with
       * the array control program.
       */
      gcp::util::NetStr* arrayStr_;       
      
      static const unsigned int NUM_BUFFER_FRAMES   = 10;
      static const unsigned int MIN_FRAMES_IN_QUEUE = 4;
      /**
       * The output frame buffer
       */
      gcp::util::ArrayFrameBuffer fb_;
      unsigned boloId_;
      /**
       * True when a frame is waiting to be dispatched.
       */
      bool dispatchPending_;

      //------------------------------------------------------------
      // Antenna resources
      //------------------------------------------------------------
      
      /**
       * The number of antennas we may have to receive data from
       */
      unsigned nAntenna_;
      
      /**
       * The number of archived registers in a single Antenna map
       */
      std::vector<unsigned> nArchiveAntenna_;     
      
      /**
       * A std::vector of locations corresponding to the beginning of
       * each antenna register map in the array map.
       */
      std::vector<int> antStartSlots_;
      
      /**
       * A std::vector of locations corresponding to the frame.received
       * register of each register map in the array map.
       */
      std::vector<int> antRecSlots_;
      
      /**
       * A bitmask of antennas from which we received data frames
       * in the last half second.
       */
      unsigned antReceivedCurrent_;
      unsigned antReceivedLast_;
      
      /**
       * Set the received status for all antenna register maps.
       */
      void setAntReceived(bool received, 
			  gcp::util::ArrayDataFrameManager* frame=0);
      
      /**
       * Set the received status for a single register map.
       */
      void setAntReceived(unsigned iant, bool received, 
			  gcp::util::ArrayDataFrameManager* frame=0);
      
      /**
       * True if the connection status of any antenna has changed.
       */
      bool antennasChangedState();
      
      /**
       * Method to report an atenna state change to the master thread.
       */
      void reportStateChange();
      
      /**
       * Private constructor insures that this class can only be
       * instantiated by .
       */
      Scanner(Master* master);
      
      /**
       * True when connected to the archiver port
       */
      bool isConnected(); 
      
      /**
       * Connect to the scanner port
       */
      bool connect();        
      
      /**
       * Disconnect from the scanner port
       */
      void disconnect();     
      
      /**
       * Attempt to connect to the host
       */
      void connectScanner(bool reEnable);

      /**
       * Disconnect from the host.
       */
      void disconnectScanner();

      /**
       * Process a message received on our message queue
       *
       * @throws Exception
       */
      void processMsg(ScannerMsg* taskMsg);
      
      /**
       * Start a new data frame 
       */
      gcp::util::ArrayDataFrameManager* 
	startNewArrayFrame();
      
      /**
       * Reinitialize the shared register map
       */
      void initializeShare(gcp::util::TimeVal& timeVal);

      /**
       * Fill register time axes with default values
       */
      void initializeDefaultRegisters(gcp::util::ArrayDataFrameManager* frame, 
				      gcp::util::TimeVal& mjd);

      /**
       * Check if there are data frames waiting to be sent.
       */
      void checkDataFrames();  
      
      /**
       * Send messages to consumer threads telling them to get a frame of data
       */
      void pollConsumers();

      /**
       * Install the frame buffer as the network buffer and pre-format
       * the register frame output message.
       */
      void packFrame(gcp::util::DataFrame* frame);

      /**
       * Dispatch the next unsent frame  
       */
      void dispatchNextFrame();
	
      /**
       * Return true if the scanner is connected to the ACC.
       */
      bool connected();
      
      //------------------------------------------------------------
      // Messaging methods
      //------------------------------------------------------------

      /**
       * Method to tell this task to dispatch the next data frame in
       * the queue.
       */
      void sendDispatchDataFrameMsg();
      
      /**
       * Method by which we can tell the control task to reconnect
       * to an antenna.
       *
       * @throws Exception (via MsgQ::sendMsg) 
       */
      void sendFlagAntennaMsg(unsigned antenna, bool flag);
      
      /**
       * A method to send a connection status message to the master.
       */
      void sendScannerConnectedMsg(bool connected);
      
      /**
       * A method to send a connection status message to the master.
       */
      void sendControlConnectedMsg(bool connected);

      /**
       * Override GenericTask::respondToHeartBeat()
       */
      void respondToHeartBeat();
      
      /**
       * Parse the greeting message.
       */
      void parseGreeting();
      
      /**
       * A handler to be called when a frame has been completely sent.
       */
      static NET_READ_HANDLER(readHandler);
      
      /**
       * A handler to be called when a frame has been completely sent.
       */
      static NET_SEND_HANDLER(sendHandler);

      /**
       * A handler to be called when an error occurs in communication
       */
      static NET_ERROR_HANDLER(networkError);

      /**
       * Service our message queue.
       */
      void serviceMsgQ();
      void serviceMsgQSave();

      /**
       * Change the features
       */
      void changeFeatures(unsigned seq, unsigned mode, unsigned mask);

      Master* parent() {return parent_;};

      /**
       * Forward a message to the dio control task.
       */
      void forwardDioMsg(ScannerMsg* msg);

    }; // End class Scanner
    
  } // End namespace mediator
} // End namespace gcp

#endif // End #ifndef GCP_MEDIATOR_SCANNER_H
