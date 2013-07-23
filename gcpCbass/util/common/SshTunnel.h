// $Id: SshTunnel.h,v 1.2 2010/02/23 17:19:58 eml Exp $

#ifndef GCP_UTIL_SSHTUNNEL_H
#define GCP_UTIL_SSHTUNNEL_H

/**
 * @file SshTunnel.h
 * 
 * Tagged: Fri Jan 26 20:19:39 NZDT 2007
 * 
 * @version: $Revision: 1.2 $, $Date: 2010/02/23 17:19:58 $
 * 
 * @author Erik Leitch
 */
#include "gcp/util/common/Runnable.h"

#include <iostream>
#include <string>
#include <sstream>

namespace gcp {
  namespace util {

    class CoProc;

    //-----------------------------------------------------------------------
    // A class for opening an ssh tunnel to a port on a remote machine.
    // The machine may or may not be behind a firewall (gateway) machine that
    // allows only ssh connections.
    //
    // This class simply spawns a background thread that makes a
    // system-level call to ssh that maps local ports to ports on a remote
    // machine that may or not be behind a firewall.
    //
    // Note that for the connection to succeed, you must have set up
    // ssh permissions on the remote machine to accept incoming ssh
    // connections without prompting for a password
    //-----------------------------------------------------------------------

    class SshTunnel {
    public:

      // Constructors

      
      // Constructor to ssh tunnel to a port (port) on a machine
      // (host) behind a gateway (gateway) machine.  If there is no
      // gateway between us and the host, use the next constructor, or
      // specify the same aregument for both

      SshTunnel(std::string gateway, std::string host, unsigned short port, 
		unsigned timeOutInSeconds=0);

      SshTunnel(std::string gateway, std::string host, unsigned short port, unsigned short hostPort, 
		unsigned timeOutInSeconds=0);

      // Constructor to tunnel to a port on the specified machine

      SshTunnel(std::string host, unsigned short port, 
		unsigned timeOutInSeconds=0);

      SshTunnel(std::string host, unsigned short port, unsigned short hostPort,
		unsigned timeOutInSeconds=0);

      /**
       * Output Operator.
       */
      friend std::ostream& operator<<(std::ostream& os, SshTunnel& obj);

      /**
       * Destructor.
       */
      virtual ~SshTunnel();

      bool succeeded();
      std::string error();

    private:

      std::string host_;
      std::string gateway_;
      unsigned short port_;
      unsigned short hostPort_;
      std::ostringstream error_;
      CoProc* proc_;
      bool success_;

      void spawn();

      void initialize(std::string gateway, std::string host, unsigned short port, unsigned short hostPort, 
		      unsigned timeOutInSeconds=0);

      void blockUntilSuccessOrTimeOut(unsigned timeOutInSeconds);

      void parseStdOutMsg(int fd, std::ostringstream& os, bool& connected, bool& stop);
      void parseStdErrMsg(int fd, std::ostringstream& os, bool& connected, bool& stop);

    }; // End class SshTunnel

  } // End namespace util
} // End namespace gcp



#endif // End #ifndef GCP_UTIL_SSHTUNNEL_H
