#include "gcp/util/common/Connection.h"
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/NedReader.h"
#include "gcp/util/common/String.h"
#include "gcp/util/common/PtSrcReader.h"
#include "gcp/util/common/PtSrcTiler.h"
#include "gcp/util/common/Sort.h"

#include <curl/curl.h>

#include <string.h>
#include <iomanip>
#include <iostream>

#include <string.h>

using namespace std;
using namespace gcp::util;

std::vector<NedReader::Obj>     NedReader::objects_  = NedReader::initObjects();
std::vector<NedReader::Catalog> NedReader::catalogs_ = NedReader::initCatalogs();

/**.......................................................................
 * Constructor.
 */
NedReader::NedReader() 
{
  start_ = 0;
  stop_  = string::npos;

  objIncMask_ = 0x0;
  objExcMask_ = 0x0;
  catMask_    = 0x0;

  objSel_     = ANY; 
  catSel_     = ANY;
}

/**.......................................................................
 * Destructor.
 */
NedReader::~NedReader() {}

/**.......................................................................
 * Get a URL
 */
void NedReader::getUrl(std::string url)
{
  lastRead_.str("");

  std::ostringstream testStr;

  // global libcURL init

  curl_global_init( CURL_GLOBAL_ALL ) ;

  // create a context, sometimes known as a handle.
  // Think of it as a lookup table, or a source of config data.

  CURL* ctx = curl_easy_init() ;
  
  if(ctx == NULL) {
    ThrowError("Unable to initialize cURL interface");
  }
  
  // BEGIN: configure the handle:
  
  // target url:

  curl_easy_setopt( ctx , CURLOPT_URL,  url.c_str() ) ;

  // no progress bar:

  curl_easy_setopt( ctx , CURLOPT_NOPROGRESS , OPTION_TRUE) ;

  // (sending response headers to stderr)

  // This next line causes a seg fault when trying to pass data to my user function
  //  curl_easy_setopt( ctx , CURLOPT_WRITEHEADER , stderr) ;

  // response content: same choices as headers
  // send it to stdout to prove that libcURL differentiates the two

  curl_easy_setopt( ctx , CURLOPT_WRITEFUNCTION, handle_data);

  curl_easy_setopt( ctx , CURLOPT_WRITEDATA, (void*)this);

  // END: configure the handle 
	
  // action!

  const CURLcode rc = curl_easy_perform( ctx ) ;

  if(rc != CURLE_OK) {

    //    std::cerr << "Error from cURL: " << curl_easy_strerror( rc ) << std::endl ;
    std::cerr << "Error from cURL: " << std::endl;
    
  } else {

    // get some info about the xfer:

    double statDouble ;
    long statLong ;
    char* statString = NULL ;
    
    // known as CURLINFO_RESPONSE_CODE in later curl versions

    if(CURLE_OK == curl_easy_getinfo(ctx, CURLINFO_HTTP_CODE, &statLong)){
      COUT("Response code:  " << statLong);
    }
    
    if(CURLE_OK == curl_easy_getinfo(ctx, CURLINFO_CONTENT_TYPE, &statString)){
      COUT("Content type:   " << statString);
    }
    
    if(CURLE_OK == curl_easy_getinfo(ctx, CURLINFO_SIZE_DOWNLOAD, &statDouble)){
      COUT("Download size:  " << statDouble << " bytes");
    }
    
    if(CURLE_OK == curl_easy_getinfo(ctx, CURLINFO_SPEED_DOWNLOAD, &statDouble)){
      COUT("Download speed: " << statDouble << " bytes/sec");
    }
    
  }

  // cleanup

  curl_easy_cleanup(ctx);
  curl_global_cleanup();

  // Process data read from the 

  string::size_type loc = lastRead_.str().find( "Associations", 0);

  if(loc != string::npos) {
    start_ = loc + 13;
  } else {
    stop_ = string::npos;
  }
}

/**.......................................................................
 * Initialize the source iterator to the head of the list
 */
void NedReader::openCatalogFile() 
{
  srcIter_ = srcs_.begin();
}

// No-ops

void NedReader::closeCatalogFile() {}
void NedReader::applyCorrections(PtSrcReader::Source& src) {}

/**.......................................................................
 * Return true when we are at the end of the list of sources
 */
bool NedReader::eof() 
{
  return srcIter_ == srcs_.end();
}

/**.......................................................................
 * Parse the next source entry
 */
PtSrcReader::Source NedReader::readNextEntry()
{
  PtSrcReader::Source src = *srcIter_;
  ++srcIter_;
  return src;
}

/**.......................................................................
 * Methods for parsing the response from the NED database server
 */
void NedReader::initializeResponseParse()
{
  // If the error string was returned, set stop_ so that eof() will
  // return true

  if(lastRead_.str().find("PARAM name=\"Error\" value=\" No object found.\"")!=string::npos) {
    stop_ = string::npos;
  } else {
    stop_ = 0;
  }
}

bool NedReader::atEndOfResponse()
{
  return stop_ == string::npos || stop_ == lastRead_.str().size()-1;
}

/**.......................................................................
 * Parse the next source entry
 */
PtSrcReader::Source NedReader::readNextSourceFromResponse()
{
  // Find the end of the current line from the file

  stop_ =   lastRead_.str().find('\n', start_);

  // Parse the source information

  PtSrcReader::Source src = parseSource(lastRead_.str().substr(start_, stop_-start_));

  // Increment the next start index

  start_ = stop_+1;

  return src;
}

/**.......................................................................
 * Interpret source information from the tab-separated NED return format
 */
PtSrcReader::Source NedReader::parseSource(std::string line)
{
  PtSrcReader::Source src;

  String str(line);

  // Ignore the index

  str.findNextStringSeparatedByChars("|");

  // The survey and name are stuck together in the same string,
  // separated by whitespace
 
  String nameSpec = str.findNextStringSeparatedByChars("|");

  src.survey_ = nameSpec.findNextString().str();
  src.name_   = nameSpec.findNextString().str();

  src.ra_.setDegrees(str.findNextStringSeparatedByChars("|").toDouble());
  src.dec_.setDegrees(str.findNextStringSeparatedByChars("|").toDouble());

#if 1
  COUT("Source name is: "       << src.name_);
  COUT("Source type is: "       << str.findNextInstanceOf("|"));
  COUT("Veclocity is: "         << str.findNextInstanceOf("|"));
  COUT("Redshift is: "          << str.findNextInstanceOf("|"));
  COUT("Redshift flag is: "     << str.findNextInstanceOf("|"));
#endif

  src.distance_.setArcMinutes(str.findNextInstanceOf("|").toDouble());

#if 0
  COUT("Nref is: "              << str.findNextInstanceOf("|"));
  COUT("Nnotes is: "            << str.findNextInstanceOf("|"));
  COUT("Nphoto is: "            << str.findNextInstanceOf("|"));
#endif

  return src;
}

/**.......................................................................
 * Construct a valid NED URL for searching near a specified position.
 */
void NedReader::
constructNearPositionSearchUrl(ostringstream& os, HourAngle ra, Declination dec, Angle radius, 
			       unsigned objIncMask, unsigned objExcMask, 
			       unsigned catMask,    unsigned objSel,     unsigned catSel)
{
  os.str("");
  os << "http://ned.ipac.caltech.edu/cgi-bin/nph-objsearch";

  os << "?in_csys=Equatorial&in_equinox=J2000.0";

  os << "&lon=" << setw(2) << std::setfill('0') << ra.getIntegerHours();
  os << "%3A"   << setw(2) << std::setfill('0') << ra.getIntegerMinutes();
  os << "%3A"   << setw(2) << std::setfill('0') << ra.getIntegerSeconds();
  os << "."     << setw(3) << std::setfill('0') << ra.getIntegerMilliSeconds();

  os << "&lat=" << setw(2) << std::setfill('0') << dec.getIntegerDegrees();
  os << "%3A"   << setw(2) << std::setfill('0') << dec.getIntegerArcMinutes();
  os << "%3A"   << setw(2) << std::setfill('0') << dec.getIntegerArcSeconds();
  os << "."     << setw(3) << std::setfill('0') << dec.getIntegerMilliArcSeconds();

  os << "&radius=" << radius.arcmin();

  os << "&search_type=Near+Position+Search";
  os << "&out_csys=Equatorial&out_equinox=J2000.0";
  os << "&obj_sort=Distance+to+search+center";

  os << "&of=ascii_bar";
  //os << "&of=ascii_tab";
  //os << "&of=press_text";

  os << "&zv_breaker=30000.0";
  os << "&list_limit=5";
  os << "&img_stamp=YES";
  os << "&z_constraint=Unconstrained";
  os << "&z_value1=&z_value2=&z_unit=z";

  switch(objSel) {
  case ANY:
    os << "&ot_include=ANY"; 
    break; 
  case ALL:
    os << "&ot_include=ALL"; 
    break;
  }

  includeObjects(os,  objIncMask);
  excludeObjects(os,  objExcMask);

  switch(catSel) {
  case ANY:
    os << "&nmp_op=ANY"; 
    break; 
  case ALL:
    os << "&nmp_op=ALL"; 
    break;
  case NOT:
    os << "&nmp_op=NOT"; 
    break;
  case NON:
    os << "&nmp_op=NON"; 
    break;
  }

  selectCatalogs(os, catMask);
}

unsigned short NedReader::objIdToNedId(unsigned short id)
{
  switch(id) {
  case CLASSIFIED_EXTRAGALACTIC:
    return 1;
    break;
  case UNCLASSIFIED_EXTRAGALACTIC:
    return 2;
    break;
  case GALAXY_COMPONENT:
    return 3;
    break;
  }
}

unsigned short NedReader::catIdToNedId(unsigned short id)
{
  switch(id) {
  case CAT_ONE:
    return 1;
    break;
  case CAT_TWO:
    return 2;
    break;
  case CAT_THREE:
    return 3;
    break;
  case CAT_FOUR:
    return 4;
    break;
  }
}

/**.......................................................................
 * Add object types
 */
void NedReader::
includeObjects(ostringstream& os, unsigned mask)
{
  if(mask == ALL)
    return;

  for(unsigned iObj=0; iObj < objects_.size(); iObj++) {
    if(mask & objects_[iObj].type_) {
      os << "&in_objtypes" << objIdToNedId(objects_[iObj].id_) << "=" << objects_[iObj].inputName_;
    }
  }
}

/**.......................................................................
 * Exclude object types
 */
void NedReader::
excludeObjects(ostringstream& os, unsigned mask)
{
  if(mask == OBJ_NONE)
    return;

  for(unsigned iObj=0; iObj < objects_.size(); iObj++) {
    if(mask & objects_[iObj].type_) {
      os << "&ex_objtypes" << objIdToNedId(objects_[iObj].id_) << "=" << objects_[iObj].inputName_;
    }
  }
}

/**.......................................................................
 * Add an object to a mask
 */
void NedReader::
maskObject(std::string name, unsigned& mask)
{
  for(unsigned iObj=0; iObj < objects_.size(); iObj++) {
    if(strcasecmp(name.c_str(), objects_[iObj].expl_.c_str())==0) {
      mask |= objects_[iObj].type_;
      return;
    }
  }
}

/**.......................................................................
 * Add catalog types
 */
void NedReader::
selectCatalogs(ostringstream& os, unsigned mask)
{
  COUT("Inside selectCatalogs: " << mask);

  if(mask == CAT_ALL)
    return;

  for(unsigned iObj=0; iObj < catalogs_.size(); iObj++) {

    COUT("checking mask: " << mask << " for: " << catalogs_[iObj].inputName_);

    if(mask & catalogs_[iObj].type_) {
      COUT("masking : " << catalogs_[iObj].inputName_);
      os << "&name_prefix" << catIdToNedId(catalogs_[iObj].id_) << "=" << catalogs_[iObj].inputName_;
    }
  }
}

/**.......................................................................
 * Add an catalog to a mask
 */
void NedReader::
maskCatalog(std::string name, unsigned& mask)
{
  for(unsigned iObj=0; iObj < catalogs_.size(); iObj++) {
    if(strcasecmp(name.c_str(), catalogs_[iObj].expl_.c_str())==0) {
      mask |= catalogs_[iObj].type_;
      return;
    }
  }
}

/**.......................................................................
 * Static member initialization
 */
std::vector<NedReader::Catalog> NedReader::initCatalogs()
{
  std::vector<NedReader::Catalog> catalogs;

  catalogs.push_back(Catalog(CAT_ABELL,   CAT_ONE,   "ABELL", "ABELL", "Abell Cluster catalog"));

  catalogs.push_back(Catalog(CAT_MACS,    CAT_TWO,   "MACS",   "MACS",  "Massive Cluster Catalog (from literature)"));

  catalogs.push_back(Catalog(CAT_MESSIER, CAT_TWO,   "MESSIER", "MESSIER",    "Messier Catalog of nebulae"));

  catalogs.push_back(Catalog(CAT_MG,      CAT_TWO,   "MG",    "MG",    "MIT/Greenbank Radio Catalog"));
  catalogs.push_back(Catalog(CAT_MG1,     CAT_TWO,   "MG1",   "MG1",   "First MIT/Greenbank 5-GHz survey"));
  catalogs.push_back(Catalog(CAT_MG2,     CAT_TWO,   "MG2",   "MG2",   "Second MIT/Greenbank 5-GHz survey"));
  catalogs.push_back(Catalog(CAT_MG3,     CAT_TWO,   "MG3",   "MG3",   "Third MIT/Greenbank 5-GHz survey"));
  catalogs.push_back(Catalog(CAT_MG4,     CAT_TWO,   "MG4",   "MG4",   "Fourth MIT/Greenbank 5-GHz survey"));

  catalogs.push_back(Catalog(CAT_NGC,     CAT_TWO,   "NGC",   "NGC",   "New General Catalog"));

  catalogs.push_back(Catalog(CAT_3C,    CAT_THREE, "3C",    "3C",    "3rd Cambridge Survey"));
  catalogs.push_back(Catalog(CAT_4C,    CAT_THREE, "4C",    "4C",    "4th Cambridge Survey"));
  catalogs.push_back(Catalog(CAT_5C,    CAT_THREE, "5C",    "5C",    "5th Cambridge Survey"));
  catalogs.push_back(Catalog(CAT_6C,    CAT_THREE, "6C",    "6C",    "6th Cambridge Survey"));
  catalogs.push_back(Catalog(CAT_7C,    CAT_THREE, "7C",    "7C",    "7th Cambridge Survey"));
  catalogs.push_back(Catalog(CAT_8C,    CAT_THREE, "8C",    "8C",    "8th Cambridge Survey"));
  catalogs.push_back(Catalog(CAT_9C,    CAT_THREE, "9C",    "9C",    "9th Cambridge Survey"));

  catalogs.push_back(Catalog(CAT_87GB,  CAT_THREE, "87GB",  "87GB",  "1987 Greenbank Survey"));
  catalogs.push_back(Catalog(CAT_SCUBA, CAT_THREE, "SHADES","SHADES","SCUBA Half-Degree Extragalactic Survey"));

  catalogs.push_back(Catalog(CAT_XMM,   CAT_FOUR,  "XMM",   "XMM",   "XMM Catalog"));
  catalogs.push_back(Catalog(CAT_WARP,  CAT_FOUR,  "WARP",  "WARP",  "WARP Catalog"));
  catalogs.push_back(Catalog(CAT_WMAP,  CAT_FOUR,  "WMAP",  "WMAP",  "WMAP Catalog"));
					   
  return catalogs;
}

/**.......................................................................
 * Static member initialization
 */
std::vector<NedReader::Obj> NedReader::initObjects()
{
  std::vector<NedReader::Obj> objects;

  objects.push_back(Obj(GALAXY,             CLASSIFIED_EXTRAGALACTIC, "Galaxies",  "G",      "Galaxies"));
  objects.push_back(Obj(GALAXY_PAIR,        CLASSIFIED_EXTRAGALACTIC, "GPairs",    "GPair",  "Galaxy Pair"));
  objects.push_back(Obj(GALAXY_TRIPLET,     CLASSIFIED_EXTRAGALACTIC, "GTriples",  "GTrpl",  "Galaxy Triplet"));
  objects.push_back(Obj(GALAXY_GROUP,       CLASSIFIED_EXTRAGALACTIC, "GGroups",   "GGroup", "Galaxy Group"));
  objects.push_back(Obj(GALAXY_CLUSTER,     CLASSIFIED_EXTRAGALACTIC, "GClusters", "GClstr", "Galaxy Cluster"));
  objects.push_back(Obj(QSO,                CLASSIFIED_EXTRAGALACTIC, "QSO",       "QSO",    "QSO"));
  objects.push_back(Obj(QSO_GROUP,          CLASSIFIED_EXTRAGALACTIC, "QSOGroups", "QGroup", "QSO Group"));
  objects.push_back(Obj(GRAV_LENS,          CLASSIFIED_EXTRAGALACTIC, "GravLens",  "G_Lens", "Gravitational Lens"));
  objects.push_back(Obj(DAMPED_LYMAN_ALPHA, CLASSIFIED_EXTRAGALACTIC, "DLA",       "DLA",    "Damped Lyman-Alpha System"));
  objects.push_back(Obj(ABSORB_LINE_SYSTEM, CLASSIFIED_EXTRAGALACTIC, "AbsLineSys","AbLS",   "Absorption Line System"));
  objects.push_back(Obj(EMISSION_LINE_SRC,  CLASSIFIED_EXTRAGALACTIC, "EmissnLine","EmLS",   "Emission Line System"));

  // Unclassified Extragalactic Components

  objects.push_back(Obj(RADIO_SRC,          UNCLASSIFIED_EXTRAGALACTIC, "Radio",     "RadioS", "Radio Source"));
  objects.push_back(Obj(SUBMM_SRC,          UNCLASSIFIED_EXTRAGALACTIC, "Smm",       "SmmS",   "Sub-mm Source"));
  objects.push_back(Obj(IR_SRC,             UNCLASSIFIED_EXTRAGALACTIC, "Infrared",  "IrS",    "Infrared Source"));
  objects.push_back(Obj(VISUAL_SRC,         UNCLASSIFIED_EXTRAGALACTIC, "Visual",    "VisS",   "Visual Source"));
  objects.push_back(Obj(UV_EXCESS_SRC,      UNCLASSIFIED_EXTRAGALACTIC, "UVExcess",  "UvES",   "UVExcess Source"));
  objects.push_back(Obj(XRAY_SRC,           UNCLASSIFIED_EXTRAGALACTIC, "Xray",      "XrayS",  "Xray Source"));
  objects.push_back(Obj(GAMMARAY_SRC,       UNCLASSIFIED_EXTRAGALACTIC, "GammaRay",  "GammaS", "GammaRay Source"));
  
  // Components of Galaxies

  objects.push_back(Obj(SUPERNOVA,          GALAXY_COMPONENT, "Supernovae", "SN",    "Supernova"));
  objects.push_back(Obj(HII_REGION,         GALAXY_COMPONENT, "HIIregion",  "HII",   "HII Region"));
  objects.push_back(Obj(PLANETARY_NEBULA,   GALAXY_COMPONENT, "PN",         "PN",    "Planetary Nebula"));
  objects.push_back(Obj(SUPERNOVA_REMNANT,  GALAXY_COMPONENT, "SNR",        "SNR",   "Supernova Remnant"));

  return objects;
}

void NedReader::matchAny(std::ostringstream& os)
{
  os << "&ot_include=ANY";
}

void NedReader::matchAll(std::ostringstream& os)
{
  os << "&ot_include=ALL";
}

size_t NedReader::
handle_data(void* buffer, size_t size, size_t nmemb, void* userp)
{
  NedReader* reader = (NedReader*)userp;
  //ostringstream* os = (ostringstream*)userp;

  reader->lastRead_ << (char*)buffer;
  //(*os) << (char*)buffer;

  return size*nmemb;
}

/**.......................................................................
 * Calculate the minimal RA range we need to search
 */
void NedReader::setRaRange(HourAngle& ra, Declination& dec, Angle& totalRadius)
{
  Connection conn;
  if(!conn.isReachable("ned.ipac.caltech.edu")) {
    ThrowError("NED server is not currently reachable");
  }
    
  std::ostringstream url;

  srcs_.resize(0);

  // NED searches are restricted to 5 degree (300 arcminute) radius

  Angle fieldRadius;
  fieldRadius.setArcMinutes(300);
  
  // Get a vector of field centers of this radius that cover the total
  // range specified

  COUT("Entering constructFields with ra = " << ra << " dec = " << dec << " rad = " 
       << fieldRadius);

  std::vector<PtSrcTiler::Field> fields = PtSrcTiler::constructFields(ra, dec, fieldRadius, totalRadius);

  COUT("Got: " << fields.size() << " fields");

  // Now each field, construct the search string and get the results

  for(unsigned iFld=0; iFld < fields.size(); iFld++) {
    url.str("");

    constructNearPositionSearchUrl(url, fields[iFld].ra_, fields[iFld].dec_, fieldRadius, objIncMask_, objExcMask_, catMask_, objSel_, catSel_);

    COUT("Getting URL: " << url.str());

    getUrl(url.str());
    
    // Now convert the returned data into sources

    initializeResponseParse();

    while(!atEndOfResponse())
      srcs_.push_back(readNextSourceFromResponse());
  }


  removeDuplicates();
}

void NedReader::listSources() 
{
  for(std::list<PtSrcReader::Source>::iterator src = srcs_.begin(); src != srcs_.end(); src++)
    COUT(*src);
}

void NedReader::removeDuplicates()
{
  srcs_.sort(PtSrcReader::Source::isLessThan);
  srcs_.unique(PtSrcReader::Source::isEqualTo);
}

std::vector<std::string> NedReader::listObjects(unsigned short id)
{
  std::vector<std::string> objs;

  for(unsigned i=0; i < objects_.size(); i++) {
    if(id & objects_[i].id_)
      objs.push_back(objects_[i].expl_);
  }

  return Sort::sort(objs);
}

std::vector<std::string> NedReader::listCatalogs()
{
  std::vector<std::string> cats;

  for(unsigned i=0; i < catalogs_.size(); i++) {
    cats.push_back(catalogs_[i].expl_);
  }

  return Sort::sort(cats);
}

void NedReader::clearObjectIncludeMask()
{
  objIncMask_ = 0x0;
}

void NedReader::clearObjectExcludeMask()
{
  objExcMask_ = 0x0;
}

void NedReader::addToObjectIncludeMask(unsigned mask)
{
  objIncMask_ |= mask;
}

void NedReader::addToObjectExcludeMask(unsigned mask)
{
  objExcMask_ |= mask;
}

void NedReader::setObjectIncludeMask(unsigned mask)
{
  objIncMask_ = mask;
}

void NedReader::setObjectExcludeMask(unsigned mask)
{
  objExcMask_ = mask;
}



void NedReader::clearCatalogMask()
{
  catMask_ = 0x0;
}

void NedReader::addToCatalogMask(unsigned mask)
{
  catMask_ |= mask;
}

void NedReader::setCatalogMask(unsigned mask)
{
  catMask_ = mask;
}

void NedReader::setObjectSelection(unsigned type)
{
  objSel_ = type;
}

void NedReader::setCatalogSelection(unsigned type)
{
  catSel_ = type;
}

