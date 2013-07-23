#include "gcp/util/common/TipperFileClient.h"

#include<iostream>

using namespace std;

using namespace gcp::util;

/**.......................................................................
 * Constructor.
 */
TipperFileClient::TipperFileClient(std::string fileName) :
  TipperSshClient(0)
{
  bool fileExists=false;
  std::ifstream inFile(fileName.c_str());

  fileExists = inFile;

  inFile.close();

  of_.open(fileName.c_str(), ios::app);

  if(!of_)
    ThrowError("Unable to open file: " << fileName);

  if(!fileExists)
    of_ << tipperData_.header() << std::endl;
}

/**.......................................................................
 * Destructor.
 */
TipperFileClient::~TipperFileClient() 
{
  if(of_) {
    of_.close();
  }
}

/**.......................................................................
 * Destructor.
 */
void TipperFileClient::processServerData()
{
  of_ << tipperData_ << std::endl;
}
