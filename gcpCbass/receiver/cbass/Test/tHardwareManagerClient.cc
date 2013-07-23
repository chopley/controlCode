
#include <iostream>

#include "gcp/program/common/Program.h"

#include "gcp/receiver/specific/BolometerConsumer.h"

#include "Utilities/HardwareManagerClient.h"

using namespace std;
using namespace gcp::program;
using namespace gcp::receiver;

using namespace MuxReadout;

#define PRINTVEC(vec) \
  for(unsigned i=0; i < vec.size(); i++)\
    COUT(i << " " << vec[i]);

void Program::initializeUsage() {};

KeyTabEntry Program::keywords[] = {
  {"host",  "stoli2",    "s", "host"},
  {"port",  "5207",      "s", "port"},
  {"squid", "sa1",       "s", "squid id"},
  {"bolo",  "rb1",       "s", "bolo id"},
  { END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS},
};

void printVec(std::vector<std::string>& vec);

int Program::main()
{

  COUT("cleint: " << Program::getParameter("host"));
  HardwareManagerClient client(Program::getParameter("host"),
			       Program::getiParameter("port"));

  std::vector<unsigned> boards;
  std::vector<std::string> sqchannels;
  std::vector<std::string> channels;

  if(client.connect()) {

    COUT("Boards: ");
    boards = client.getBoards();
    PRINTVEC(boards);

    COUT("SQUID Channels: ");
    sqchannels = client.getSQChannels();
    PRINTVEC(sqchannels);

    COUT("Readout Channels: ");
    channels = client.getChannels();
    COUT("Channels is of size: " << channels.size());
    //    PRINTVEC(channels);

    COUT("");

    sqchannels = client.getSquidChannels(channels);

    int seq = client.getSeqId();
    
    seq = client.getSeqId();

    MuxReadout::MuxXMLFile* xml=0;
    bool success=false;

    client.getAllRegisters(&xml, &success);


    if(xml) {
      COUT(xml->get_xml_text());
      delete xml;
    }

    //    client.getAllRegisters(&xml, &success);
  }

  return 0;
}

