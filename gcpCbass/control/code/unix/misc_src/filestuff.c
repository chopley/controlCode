/* Utilities for checking files */

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

int check_file(char *path);
int check_directory(char *path);

/* Does file exist and is it readable ? 0=> yes */

int check_file(char *path)
{
  return access(path, F_OK|R_OK);
}

/* Does directory exist? 0=> yes */

int check_directory(char *path)
{
  struct stat stbuf;
  if (stat(path, &stbuf))
    return -1;
  if ((stbuf.st_mode & S_IFMT) == S_IFDIR)
    return 0;
  return -1;
}
