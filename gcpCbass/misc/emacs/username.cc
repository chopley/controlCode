#define __FILEPATH__ "misc/emacs/username.cc"

#include <iostream>
#include <unistd.h>
#include <pwd.h>
#include <sys/types.h>

//       struct passwd {
//         char    *pw_name;       user name 
//         char    *pw_passwd;     user password 
//         uid_t   pw_uid;         user id 
//         gid_t   pw_gid;         group id 
//         char    *pw_gecos;      real name 
//         char    *pw_dir;        home directory 
//         char    *pw_shell;      shell program 
//       };

int main(void)
{
 struct passwd* passwd = getpwuid(geteuid());
 std::cout << passwd->pw_gecos << std::endl;
}
