#include <string.h>
/* Header file for use with these routines */
#include "date.h"
/* MCS header file */
#include "astrom.h"

/* Convert MJD to printable date string, using MCS routines */

void mjd_to_date(double mjd, char *date_string, int size)
{
  OutputStream *output;
  output = new_OutputStream();
  if(!output)
    goto error;
  if(open_StringOutputStream(output, 0, date_string, size)) {
    close_OutputStream(output);
    goto error;
  };
  if(output_utc(output, "", 0, 0, mjd)) {
    close_OutputStream(output);
    goto error;
  };
  close_OutputStream(output);
  del_OutputStream(output);
  return;

error:
  strncpy(date_string, "[error]", size);
  return;
}

/*.......................................................................
 * Convert a UTC date string to the equivalent Modified Julian Date.
 *
 * Input:
 *  date_string  char *  The string to be decoded, eg:
 *                        "12-may-1998 12:34:50" or "12-May-1998 3:45".
 * Input/Output:
 *  mjd        double *  The decoded Modified Julian Date.
 * Output:
 *  return        int    0 - OK.
 *                       1 - Error (an error message will have been
 *                                  sent to stderr).
 */
int date_to_mjd(char *date_string, double *mjd)
{
  InputStream *input = new_InputStream();
  if(!input ||
     open_StringInputStream(input, 0, date_string) ||
     input_utc(input, 1, 0, mjd)) {
    close_InputStream(input);
    return 1;
  };
  close_InputStream(input);
  del_InputStream(input); 
  return 0;
}
