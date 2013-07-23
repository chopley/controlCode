#include "gcp/util/common/TipperReader.h"
#include "gcp/util/common/Date.h"

#include<iostream>

#include "unistd.h"

using namespace std;

using namespace gcp::util;

/**.......................................................................
 * Constructor.
 */
TipperReader::TipperReader() {}

/**.......................................................................
 * Destructor.
 */
TipperReader::~TipperReader() {}

void TipperReader::getMostRecentData()
{
  FILE* fp = 0;
  char line[100];
  char* tok=0;

  if((fp=fopen("SmmTip.log", "r"))==0)
    ThrowError("Unable to open file");

  while(fgets(line, 100, fp) != 0);

  fclose(fp);

  // Now the line should contain the last entry

  unsigned int year, month, day, hour, min, sec;

  sscanf(line, "%d-%d-%d\t%d:%d:%d\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf",
	 &year, &month,  &day, &hour, &min, &sec, 
	 &data_.filter_, &data_.tHot_, &data_.tWarm_, 
	 &data_.tAmb_,   &data_.tChop_, &data_.tInt_, 
	 &data_.tSnork_, &data_.tAtm_, &data_.tau_, 
	 &data_.tSpill_, &data_.r_, &data_.mse_);

  TimeVal tVal;
  tVal.setMjd(Date::calToMjd(day, month, year, hour, min, sec));

  data_.mjdDays_ = tVal.getMjdDays();
  data_.mjdMs_   = tVal.getMjdMilliSeconds();
}
