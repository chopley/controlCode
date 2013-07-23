#ifndef GCP_UTIL_TERMINALSERVERMSG_H
#define GCP_UTIL_TERMINALSERVERMSG_H

/**
 * @file TerminalServerMsg.h
 * 
 * Tagged: Mon May 10 17:57:30 PDT 2004
 * 
 * @author Erik Leitch
 */
namespace gcp {
  namespace util {
    
    class TerminalServerMsg : public GenericTaskMsg {
    public:
      
      enum MsgType {
	LINE,
	
      }
      
    private:
    }; // End class TerminalServerMsg
    
  } // End namespace util
} // End namespace gcp




#endif // End #ifndef GCP_UTIL_TERMINALSERVERMSG_H
