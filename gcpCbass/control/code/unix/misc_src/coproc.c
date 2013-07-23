#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int clr_coprocess(int *wfd, int *rfd, FILE **wfp, FILE **rfp);
int coprocess(char *exe, char **argv, FILE **rfp, FILE **wfp);

#ifdef junk
int main(int argc, char *argv[])
{
  char buff[80];
  FILE *wfp;
  FILE *rfp;
  if(coprocess("sort", argv, &rfp, &wfp))
    exit(1);
  fprintf(wfp, "hello\nworld\nabloginous\n");
  fclose(wfp);
  wfp = NULL;
  while(fgets(buff, sizeof(buff), rfp)) {
    char *cptr = strchr(buff, '\n');
    if(cptr) *cptr = '\0';
    printf("%s\n", buff);
  };
  return 0;
}
#endif

int coprocess(char *exe, char **argv, FILE **rfp, FILE **wfp)
{
  int wfd[2];          /* Read/write file-descriptors for child stdin */
  int rfd[2];          /* Read/write file-descriptors for child stdout */
  int pid;             /* The child process ID */
  *wfp = NULL;
  *rfp = NULL;
/*
 * Open a pipe for the child stdin and create a file pointer equivalent.
 */
  if(pipe(wfd) < 0)
    return 1;
  else if((*wfp = fdopen(wfd[1], "w")) == NULL)
    return clr_coprocess(wfd, NULL, NULL, NULL);
/*
 * Open a pipe for the child stdout and create a file pointer equivalent.
 */
  if(pipe(rfd) < 0)
    return clr_coprocess(wfd, rfd, wfp, NULL);
  else if((*rfp = fdopen(rfd[0], "r")) == NULL)
    return clr_coprocess(wfd, rfd, wfp, NULL);
/*
 * Create a child process.
 */
  pid = fork();
  if(pid < 0) {
    perror("coprocess: Unable to fork.\n");
    return clr_coprocess(wfd, rfd, wfp, rfp);
  } else if(pid > 0) {     /* Parent */
    close(wfd[0]);
    close(rfd[1]);
  } else {                 /* Child */
    close(wfd[1]);
    close(rfd[0]);
/*
 * Make the child ends of the pipes refer to stdin and stdout.
 */
    if(dup2(wfd[0], STDIN_FILENO) < 0 || dup2(rfd[1], STDOUT_FILENO) < 0) {
      perror("coprocess: dup2 error");
      _exit(1);
    };
    /*
     * Overlay the child process with the desired executable.
     */
    /* For some reason, the call to execvp no longer works -- 26 Apr
     * 2002.  must have happened in the fall when operating system was
     * upgraded.
     *
     * The following is based on an example from Steven's book, but I
     * don't understand why the explicit path and name are both
     * required
     */
    /*    execvp(exe, argv); */
    execl("/bin/sort", "sort", (char *) 0); 
    perror("coprocess: exec error");
    _exit(1);
  };
  return 0;
}

/*.......................................................................
 * Private cleanup function of coprocess().
 *
 * Input:
 *  wfd     int *  Pointer to &wfd[0].
 *  rfd     int *  Pointer to &rfd[0].
 *  wfp    FILE ** Write file pointer to close, or NULL.
 *  rfp    FILE ** Read file pointer to close, or NULL.
 * Output:
 *  return  int    1.
 */
static int clr_coprocess(int *wfd, int *rfd, FILE **wfp, FILE **rfp)
{
  if(wfp && *wfp)
    fclose(*wfp);
  if(rfp && *rfp)
    fclose(*rfp);
  if(rfd) {
    close(rfd[0]);
    close(rfd[1]);
  };
  if(wfd) {
    close(wfd[0]);
    close(wfd[1]);
  };
  return 1;
}
