#include "gcp/util/common/Exception.h"
#include "gcp/util/common/InetAddress.h"

#include <netdb.h>
#include <stdio.h>

using namespace std;

using namespace gcp::util;

/**.......................................................................
 * Constructor.
 */
InetAddress::InetAddress() {}

/**.......................................................................
 * Destructor.
 */
InetAddress::~InetAddress() {}

std::string InetAddress::resolveName(std::string address, bool doThrow)
{
  struct addrinfo * result;
  struct addrinfo * res;
  int error;
  std::string name("unknown");


  // Resolve the domain name into a list of addresses

  error = getaddrinfo(address.c_str(), NULL, NULL, &result);
 
  if(error != 0) {
    if(doThrow) {
      ThrowError("Error in getaddrinfo: " << gai_strerror(error));
    } else {
      ReportError("Error in getaddrinfo: " << gai_strerror(error));
      return name;
    }
  }   
 
  // Loop over all returned results and do inverse lookup

  for (res = result; res != NULL; res = res->ai_next)
  {   
    char hostname[NI_MAXHOST] = "";
 
    error = getnameinfo(res->ai_addr, res->ai_addrlen, hostname, NI_MAXHOST, NULL, 0, 0); 
 
    if (error != 0) {
      ReportError("Error in getnameinfo: " << gai_strerror(error));
      continue;
    }
 
    if(*hostname) {
      name = hostname;
      break;
    }
   }   
 
  freeaddrinfo(result);

  return name;
}
