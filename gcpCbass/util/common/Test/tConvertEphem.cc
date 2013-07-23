#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>

#include "gcp/program/common/Program.h"

#include "gcp/util/common/Exception.h"
#include "gcp/util/common/IoLock.h"
#include "gcp/util/common/SpecificName.h"
#include "gcp/util/common/String.h"

using namespace std;
using namespace gcp::util;using namespace gcp::program;void Program::initializeUsage() {};
using namespace gcp::program;

KeyTabEntry Program::keywords[] = {
  { "infile",   "",      "s", "Input ephem file"},
  { "outfile",  "",      "s", "Output ephem file"},
  { END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS},
};

void printHeader(std::string srcName, std::ofstream& fout);
void printEphem(std::ostringstream& os, std::ofstream& fout);

int Program::main()
{
  std::ifstream fin(Program::getParameter("infile").c_str());

  string s;
  ostringstream os;

  unsigned nline=0;
  while(getline(fin, s)) {
    nline++;
    os << s << std::endl;
  }

  fin.close();

  std::ofstream fout(Program::getParameter("outfile").c_str(), ios::out);

  printHeader("Jupiter", fout);
  printEphem(os, fout);

  fout.close();

  return 0;
}

void printEphem(ostringstream& os, std::ofstream& fout)
{
  // Encapsulate the ephemeris in a String object, for parsing

  String str(os.str());
  String body = str.findNextInstanceOf("$$SOE\n", true, "\n$$EOE", true);

  String calDate, calTime, jdDate;
  String rahr,    ramin,   rasec;
  String decdeg,  decmin,  decsec;
  String delta,   deldot;
  String strline;

  double mjd;

  do {
    os.str("");
    strline =    body.findNextInstanceOf("",      false, "\n",   false, true);
    calDate = strline.findNextInstanceOf(" ",     true,  " ",    true,  false);
    calTime = strline.findNextInstanceOf(" ",     true,  " ",    true,  false);
    jdDate  = strline.findNextInstanceOf(" ",     true,  "    ", true,  false);

    rahr    = strline.findNextInstanceOf("     ", true,  " ",    true,  false);
    ramin   = strline.findNextInstanceOf(" ",     true,  " ",    true,  false);
    rasec   = strline.findNextInstanceOf(" ",     true,  " ",    true,  false);

    decdeg  = strline.findNextInstanceOf(" ",     true,  " ",    true,  false);
    decmin  = strline.findNextInstanceOf(" ",     true,  " ",    true,  false);
    decsec  = strline.findNextInstanceOf(" ",     true,  "  ",   true,  false);

    delta   = strline.findNextInstanceOf("  ",    true,  "  ",   true,  false);
    deldot  = strline.findNextInstanceOf("  ",    true,  "  ",   false, false);

    mjd = jdDate.toDouble() - 2400000.5;

    fout << setw(10)  << setprecision(4) << std::fixed << mjd << "  " 
	 << rahr   << ":" << ramin  << ":" << rasec  << "  "
	 << decdeg << ":" << decmin << ":" << decsec << "  "
	 << delta << "  #  " << calDate << " " << calTime << std::endl; 

  } while(!body.atEnd());
}

void printHeader(std::string srcName, std::ofstream& fout)
{
  fout << "#                     " << SpecificName::experimentNameCaps() << " Ephemeris for " << srcName << std::endl;
  fout << "#" << std::endl;
  fout << "# Note that TT ~= TAI + 32.184." << std::endl;
  fout << "# See slaDtt.c for how to convert TT to UTC." << std::endl;
  fout << "#" << std::endl;
  fout << "# Also note that MJD = JD - 2400000.5" << std::endl;
  fout << "#" << std::endl;
  fout << "# MJD (TT)   Right Ascen    Declination   Distance (au)      Calendar (TT)" << std::endl;
  fout << "#---------  -------------  -------------  ------------     -----------------" << std::endl;
}
