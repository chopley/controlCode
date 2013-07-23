#ifndef NETSENDSTR_H
#define NETSENDSTR_H

/**
 * @file NetSendStr.h
 * 
 * Started: Sun Feb 29 15:29:37 UTC 2004
 * 
 * @author Erik Leitch
 */
#include "gcp/control/code/unix/libunix_src/common/netbuf.h"
#include "gcp/control/code/unix/libunix_src/common/netobj.h"

#define NET_SEND_HANDLER(fn) void (fn)(void* arg)

#ifdef NET_ERROR_HANDLER
#undef NET_ERROR_HANDLER
#endif

#define NET_ERROR_HANDLER(fn) void (fn)(void* arg)

namespace gcp {

  namespace control {
    class NetSendStr;
  };

  namespace util {    
    
    class NetSendStr {
    public:
      
      enum NetSendId {   
	
	// The message body is being sent 
	
	NET_SEND_DATA   = gcp::control::NetSendStr::NET_SEND_DATA,
	
	// The message has been completely sent 
	
	NET_SEND_DONE   = gcp::control::NetSendStr::NET_SEND_DONE,
	
	// Closed connection detected at start of message 
	
	NET_SEND_CLOSED = gcp::control::NetSendStr::NET_SEND_CLOSED,
	
	// I/O error detected while sending 
	
	NET_SEND_ERROR  = gcp::control::NetSendStr::NET_SEND_ERROR
      };
      
      /**
       * Constructor.
       */
      NetSendStr();
      
      /**
       * Constructor with file descriptor and size.
       */
      NetSendStr(int fd, unsigned int size);
      
      /**
       * Destructor.
       */
      virtual ~NetSendStr();
      
      /**
       * Attach this network buffer to a file descriptor.
       */
      void attach(int fd);
      
      /**
       * Set the network buffer pointing to an external buffer, or
       * pass buffer=NULL to dynamically allocate it.
       */
      void setBuffer(void* buffer, unsigned int size);
      
      /**
       * Send a message packed into out network buffer to a socket
       * described by a previously attached fd.
       */
      NetSendId send();
      
      /**
       * Send a message packed into out network buffer to the
       * specified socket.  
       */
      NetSendId send(int fd);
      
      /**
       * Return the last send state
       */
      NetSendId state();
      
      /**
       * Return the file descriptor to which we're currently
       * attached.
       */
      int getFd();
      
      /**
       * Start packing a message into a network buffer.
       */
      void startPut(int opcode);
      
      /**
       * Finish packing a message into a network buffer.
       */
      void endPut();
      
      /**
       * Pack a byte into a network buffer.
       */
      void putChar(int ndata, unsigned char *data);
      
      /**
       * Pack a short into a network buffer
       */
      void putShort(int ndata, unsigned short *data);
      
      /**
       * Pack a int into a network buffer.
       */
      void putInt(int ndata, unsigned int *data);
      
      /**
       * Pack a float into a network buffer.
       */
      void putFloat(int ndata, float *data);	
      
      /**
       * Pack a double into a network buffer.
       */
      void putDouble(int ndata, double *data);
      
      /**
       * Pack an object to a network buffer.
       */
      void putObj(const NetObjTable* types, int id, void *obj);
      
      /**
       * Incrementally put bytes into a network buffer.
       */
      void incNput(int nbytes);
      
      /**
       * Install a send handler.  
       */
      void installSendHandler(NET_SEND_HANDLER(*handler), void* arg);
      
      /**
       * Install an error handler.  
       */
      void installErrorHandler(NET_ERROR_HANDLER(*handler), void* arg);
      
    private:
      
      /**
       * The substance of any constructor for this class.
       */
      void privateConstructor(int fd, unsigned int size);
      
      /**
       * The internal NetBuf object.
       */
      gcp::control::NetSendStr* netStream_;
      
      /**
       * True when the network buffer has been allocated.
       */
      bool netBufAllocated_;
      
      /**
       * True when we are attached to a file descriptor.
       */
      bool attached_;
      
      /**
       * Send a message in our network buffer to the
       * specified socket.  
       */
      NetSendId privateSend(int fd);
      
      /**
       * Return the last send state
       */
      NetSendId privateState(gcp::control::NetSendStr::NetSendId id);
      
      // Handler functions
      
      /**
       * A handler to be called when a send has finished
       */
      NET_SEND_HANDLER(*sendHandler_);
      
      /**
       * The argument to pass to the send handler.
       */
      void* sendArg_;
      
      /**
       * A handler to be called if an error occurs while sending
       */
      NET_SEND_HANDLER(*errorHandler_);
      
      /**
       * The argument to pass to the error handler.
       */
      void* errorArg_;
      
    }; // End class NetSendStr
    
  } // End namespace util
} // End namespace gcp


#endif // End #ifndef 


