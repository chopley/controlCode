/* Read a line from stdin, and parse it into white-space separated words.
 * Double quotes may enclose an argument, and are removed; quoted arguments
 * may include white-space characters.
 * Return the number of words and an array of pointers to the start of each
 * null-terminated word. If there are more than max arguments, the additional
 * ones are returned as part of the max'th argument.
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define READLINE_LIBRARY
#include "readline.h"
#include "history.h"

int get_command(FILE *fp, char *prompt, int max, int *narg, char *args[]);

static int init = 1;

int get_command(FILE *fp, char *prompt, int max, int *narg, char *args[])
{
  static char line[256]; /* returned pointers point into this array */
  char *p;
  int done, quote;
  char *line_read = (char *) NULL;

  *narg = 0;
  /* Read a line from input stream */
  if (fp == stdin) {
    if (init) {
      init = 0;
      rl_bind_key('\t', rl_insert);
    }
    line_read = readline(prompt);
    if (line_read == NULL)
      return 1;
    if (*line_read)
      add_history(line_read);
    strncpy(line, line_read, 256);
    free(line_read);
  } else {
    if (prompt)
      fputs(prompt, stderr);
    if (fgets(line, sizeof(line), fp) == NULL)
      return 1;
    /* Remove trailing carriage-return */
    line[strlen(line)-1] = '\0';
  }
  p = line;
  do {
    /* Skip leading whitespace */
    while (isspace((int)*p))
      p++;
#if DEBUG
    printf("Checking for arg %d: «%s»\n", *narg, p);
#endif
    /* Null or '#' terminates line */
    if (*p == '\0' || *p == '#') 
      return 0; 
    /* This is the start of the next argument */
    quote = (*p == '"');
    if (quote)
      p++;
    args[*narg] = p;
    (*narg)++;
    if (*narg >= max)
      return 0;
    /* Find the end of the argument */
    if (quote) {
      while (*p != '"' && *p != '\0')
	p++;
      done = (*p == '\0');
    } else {
      while (!isspace((int)*p)  && *p != '\0' && *p != '#')
	p++;
      done = (*p == '\0') || (*p == '#');
    }
    *p = '\0';
    p++;
  } while (!done);
  return 0;
}
