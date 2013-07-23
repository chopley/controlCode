#include "gcp/util/common/Exception.h"
#include "gcp/util/common/Pathname.h"

#include <iostream>

#include <pwd.h>
#include <string.h>
#include <stdlib.h>

using namespace std;

using namespace gcp::util;

/**.......................................................................
 * Constructor.
 */
Pathname::Pathname() {}

/**.......................................................................
 * Destructor.
 */
Pathname::~Pathname() {}

/**.......................................................................
 * Return a fully expanded version of the path to the named file
 */
std::string Pathname::expand(std::string dirStr, std::string fileStr)
{
  enum {BUFLEN=256};     /* The max size of a user name or path name */
  char buffer[BUFLEN];   /* A buffer for expanding ~ and $ prefixes */
  char cwd[BUFLEN];      /* The current working directory */

  char* dir  = &dirStr[0];
  char* name = &fileStr[0];

  // Check the arguments.

  if((!dir || dir[0] == '\0') && (!name || name[0] == '\0')) {
    ThrowError("No file name has been specified");
  };
  
  // No directory?

  if(!dir)
    dir = "";
  
  // No name?

  if(!name)
    name = "";
  
  // If the directory name starts with a ~ character then attempt to
  // interpolate the home directory of the following user-name.

  if(dir[0] == '~') {
    expandHomeDir(dir+1, BUFLEN, buffer);
    dirStr = buffer;
    dir  = &dirStr[0];
  } else if(dir[0] == '\0' && name[0] == '~') {
    expandHomeDir(name+1, BUFLEN, buffer);
    fileStr = buffer;
    name = &fileStr[0];
  } else if(dir[0] == '$') {
    expandEnvVar(dir+1, BUFLEN, buffer);
    dirStr = buffer;
    dir  = &dirStr[0];
  } else if(name[0] == '$') {
    expandEnvVar(name+1, BUFLEN, buffer);
    fileStr = buffer;
    name = &fileStr[0];
  };
  
  // If the path name isn't absolute, prefix the current working
  // directory.

  if(dir[0] != '/' && (dir[0] != '\0' || name[0] != '/')) {
    if(getcwd(cwd, BUFLEN) == NULL) {
      ThrowSysError("In getcwd(): ");
    };
  } else {
    cwd[0] = '\0';
  };
  
  // Concatenate the file name components as documented above.

  std::ostringstream expandedPath;

  expandedPath << cwd 
	       << ((cwd[0] == '\0') ? "" : "/") 
	       << dir 
	       << ((dir[0] != '\0' && dir[strlen(dir)-1] != '/' && name[0] !='/') ? "/" : "")
	       << name;

  return expandedPath.str();
}

/**.......................................................................
 * Read the user name at the start of a directory name and replace
 * it by the home directory of the user. Record the result in buffer[].
 */
void Pathname::expandHomeDir(char *dir, size_t buflen, char *buffer)
{
  char *cptr;          /* A pointer into dir[] */
  size_t ulen;         /* The length of the user name */
  size_t hlen;         /* The length of the home directory path */
  
  // The user name will be terminated either by a / character or by
  // the end of the string.

  cptr = strchr(dir, '/');
  if(cptr)
    ulen = cptr - dir;
  else
    ulen = strlen(dir);
  
  // Copy the user name into buffer[].

  if(ulen >= buflen) {
    ThrowError("Username too long");
  };

  strncpy(buffer, dir, ulen);
  buffer[ulen] = '\0';
  
  // Look up the password entry of the user.  First the POSIX threads
  // version - this is painful!

#if defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 199506L
  {
    struct passwd pwd;  /* The password entry of the user */
    struct passwd *ret; /* The returned pointer to pwd */
    long pwdlen;        /* The size of the getpwnam_r() buffer */
    char *pwdbuf=NULL;       /* A buffer of size pwdlen */
    int status;         /* The return value of getpwnam_r() */
    
    // Get the length of the buffer needed by the reentrant version of
    // getpwnam().

    pwdlen = sysconf(_SC_GETPW_R_SIZE_MAX);
    if(pwdlen < 0) {
      ThrowSysError("In sysconf(_SC_GETPW_R_SIZE_MAX): ");
    };
    
    // Allocate a buffer for getpwnam_r().

    pwdbuf = (char* )malloc(pwdlen);

    if(!pwdbuf) {
      ThrowError("Insufficient memory to allocate buffer for getpwnam_r()");
    };
    
    // Look up the password entry of the named user.

    status = ulen ? getpwnam_r(buffer, &pwd, pwdbuf, pwdlen, &ret) :
      getpwuid_r(geteuid(), &pwd, pwdbuf, pwdlen, &ret);

    if(status || ret==NULL) {
      free(pwdbuf);
      ThrowError("Unable to find home directory of " << buffer);
    };

    hlen = strlen(pwd.pw_dir);

    if(hlen >= buflen) {
      free(pwdbuf);
      ThrowError("Home directory too long.");
    };
    
    // Get the home directory then discard the password entry.

    strcpy(buffer, pwd.pw_dir);
    free(pwdbuf);
  };
  
  // Now the classic unix version.

#else
  struct passwd *pwd = getpwnam(buffer);
  if(!pwd) {
    ThrowError("Unable to find user " << buffer);
  };
  hlen = strlen(pwd->pw_dir);
  if(hlen >= buflen) {
    ThrowError("Home directory too long");
  };
  strcpy(buffer, pwd->pw_dir);
#endif
  
  // If anything followed the ~user component, append it to output
  // string.

  if(cptr) {
    if(hlen + strlen(cptr) > buflen) {
      ThrowError("Filename too long.");
    };
    strcpy(buffer + hlen, cptr);
  };
}

/**.......................................................................
 * Read the environment variable at the start of a directory name and
 * replace it by its value. Record the result in buffer[].
 */
void Pathname::expandEnvVar(char *dir, size_t buflen, char *buffer)
{
  char *cptr;          /* A pointer into dir[] */
  size_t nlen;         /* The length of the variable name */
  size_t vlen;         /* The length of the variable value */
  char *value;         /* The value of the environment variable */
  
  // The variable name will be terminated either by a / character or
  // by the end of the string.

  cptr = strchr(dir, '/');
  if(cptr)
    nlen = cptr - dir;
  else
    nlen = strlen(dir);
  
  // No name provided?

  if(nlen == 0) {
    ThrowError("Missing variable name in path name: " << dir);
  };
  
  // Copy the variable name into buffer[].

  if(nlen >= buflen) {
    ThrowError("Variable name too long");
  };

  strncpy(buffer, dir, nlen);
  buffer[nlen] = '\0';
  
  // Lookup the environment variable.

  value = getenv(buffer);
  if(!value) {
    ThrowError("Unknown environment variable");
  };
  
  // Expand the environment variable into the provided buffer.

  vlen = strlen(value);
  if(vlen > buflen) {
    ThrowError("The value is too long");
  };

  strcpy(buffer, value);
  
  // If anything followed the $var component, append it to output
  // string.

  if(cptr) {
    if(vlen + strlen(cptr) > buflen) {
      ThrowError("Filename too long");
    };
    strcpy(buffer + vlen, cptr);
  };
}
