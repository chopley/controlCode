#include "gcp/receiver/specific/BoloPixelManager.h"

#include "Utilities/HardwareManagerClient.h"

#include "gcp/util/common/Exception.h"

#include<iostream>

using namespace std;

using namespace gcp::receiver;

using namespace MuxReadout;

/**.......................................................................
 * Constructor from hardware manager
 */
BoloPixelManager::BoloPixelManager(std::string hwHost, unsigned int hwPort) 
{
  initialized_ = false;
  initializeFromHardwareManager(hwHost, hwPort);
}

/**.......................................................................
 * Constructor from config file.
 */
BoloPixelManager::BoloPixelManager(std::string configFile) 
{
  initialized_ = false;
  initializeFromFile(configFile);
}

/**.......................................................................
 * Right now this does nothing.  At some point in the future, this
 * may read pixel information from a file
 */
void BoloPixelManager::initializeFromFile(std::string) {}

/**.......................................................................
 * Initialize the bolometer array from the hardware manager
 */
void BoloPixelManager::initializeFromHardwareManager(std::string hwHost, unsigned int hwPort) 
{
  HardwareManagerClient client(hwHost, hwPort, true);

  std::vector<std::string> boloChannels;
  std::vector<std::string> squidChannels;

  if(client.connect(false)) {
    boloChannels  = client.getChannels();
    squidChannels = client.getSquidChannels(boloChannels);

    // Calibrator phase input

    boloChannels.push_back("2c:db7");
  } else {
    ThrowError("Couldn't connect to the hardware manager daemon");
  }

  pixels_.resize(boloChannels.size());

  unsigned pixInd;
  vector<double> xy;
  for(unsigned iPix=0; iPix < pixels_.size(); iPix++) {

    if(iPix == pixels_.size()-1)
    {
      pixInd = pixels_.size()-1;
    }
    else
    {
      pixInd = client.getPixelNumber(boloChannels[iPix]) - 1;
    }

    if(pixInd < 0 || pixInd > pixels_.size()-1)
      ThrowError("Iinvalid pixel index: " << pixInd);

    if(iPix == pixels_.size()-1)
    {
      pixels_[pixInd].setSquidReadoutChannel(squidChannels[iPix-1]);
    }
    else
    {
      xy     = client.getPixelXYCoords(boloChannels[iPix]);
      pixels_[pixInd].setSquidReadoutChannel(squidChannels[iPix]);
    }

    pixels_[pixInd].setPixelIndex(pixInd);
    pixels_[pixInd].setPixelXY(xy);
    pixels_[pixInd].setBoloReadoutChannel(boloChannels[iPix]);
  }
  initialized_ = true;
}

/**.......................................................................
 * Const Copy Constructor.
 */
BoloPixelManager::BoloPixelManager(const BoloPixelManager& objToBeCopied)
{
  *this = (BoloPixelManager&)objToBeCopied;
};

/**.......................................................................
 * Copy Constructor.
 */
BoloPixelManager::BoloPixelManager(BoloPixelManager& objToBeCopied)
{
  *this = objToBeCopied;
};

/**.......................................................................
 * Const Assignment Operator.
 */
void BoloPixelManager::operator=(const BoloPixelManager& objToBeAssigned)
{
  *this = (BoloPixelManager&)objToBeAssigned;
};

/**.......................................................................
 * Assignment Operator.
 */
void BoloPixelManager::operator=(BoloPixelManager& objToBeAssigned)
{
  pixels_ = objToBeAssigned.pixels_;
};

/**.......................................................................
 * Destructor.
 */
BoloPixelManager::~BoloPixelManager() {}

/**.......................................................................
 * Return the vector of pixels
 */
std::vector<BoloPixel> BoloPixelManager::getPixels()
{
  if(!initialized_) {
    ThrowError("Pixel array couldn't be initialized");
  } else {
    return pixels_;
  }
}
