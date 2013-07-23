#include "gcp/util/common/Debug.h"
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/LogStream.h"
#include "gcp/util/common/String.h"

#include <iostream>
#include <sstream>
#include <errno.h>

#include <string.h>

using namespace std;
using namespace gcp::util;

const std::string String::emptyString_("");
const std::string String::whiteSpace_(" \t");

/**.......................................................................
 * Constructor.
 */
String::String() 
{
  initialize();
}

String::String(unsigned int iVal) 
{
  initialize();
  std::ostringstream os;
  os << iVal;
  str_ = os.str();
}

String::String(const std::string& str) 
{
  initialize();
  str_ = str;
}

void String::initialize()
{
  iStart_ = 0;
}

/**.......................................................................
 * Destructor.
 */
String::~String() {}

/**.......................................................................
 * Strip all occurrences of the characters in stripStr from a target
 * string.
 */
void String::strip(std::string& targetStr, const std::string& stripStr)
{
  // For each char in the strip string, remove all occurrences in the
  // target string

  for(unsigned istrip=0; istrip < stripStr.size(); istrip++)
    strip(targetStr, stripStr[istrip]);
}

/**.......................................................................
 * Strip all occurrences of the characters in stripStr from a target
 * string.
 */
void String::strip(const std::string& stripStr)
{
  strip(str_, stripStr);
}

/**.......................................................................
 * Strip all occurrences of a character from a target string.
 */
void String::strip(std::string& targetStr, char stripChar)
{
  bool erased;

  do {
    erased = false;
    
    std::string::size_type idx;
    idx = targetStr.find(stripChar);
    
    if(idx != std::string::npos) {
      targetStr.erase(idx);
      erased = true;
    }
    
  } while(erased);
}

/**.......................................................................
 * Strip all occurrences of a character from a target string.
 */
void String::strip(char stripChar)
{
  strip(str_, stripChar);
}

/**.......................................................................
 * Return true if our string contains the target character
 */
bool String::contains(char c)
{
  std::string::size_type idx;

  idx = str_.find(c);

  return idx != std::string::npos;
}

/**.......................................................................
 * Return true if our string contains the target string
 */
bool String::contains(string s)
{
  if(strstr(str_.c_str(), s.c_str()) != 0) {
    return true;
  }

  return false;
}

bool String::matches(unsigned char c, std::string matchSet)
{
  for(unsigned iEl=0; iEl < matchSet.size(); iEl++)
    if(c == matchSet[iEl])
      return true;

  return false;
}

/**......................................................................
 * Assignment operators
 */
void String::operator=(const std::string& str)
{
  iStart_ = 0;
  str_    = str;
}

void String::operator=(const String str)
{
  iStart_ = str.iStart_;
  str_    = str.str_;
}

bool String::operator==(String str)
{
  return str_ == str.str_;
}

char& String::operator[](unsigned int index)
{
  return str_[index];
}

bool String::operator!=(String str)
{
  return str_ != str.str_;
}

bool String::operator<(String& str)
{
  unsigned thisLen = str_.size();
  unsigned thatLen = str.size();
  unsigned minLen = (thisLen < thatLen) ? thisLen : thatLen;

  if(*this == str)
    return false;

  char c1, c2;
  for(unsigned i=0; i < minLen; i++) {
    c1 = str_[i];
    c2 = str[i];

    if(c1 == c2)
      continue;

    if(islower(c1)) {
      if(islower(c2)) {
	return c1 < c2;
      } else {
	return true;
      }
    } else {
      if(isupper(c2)) {
	return c1 < c2;
      } else {
	return false;
      }
    }
  }

  // Else the strings are equal to the minimum length.  Then the
  // shorted string will be alphabetized first

  return thisLen < thatLen;
}

/**.......................................................................
 * Allows cout << String
 */
std::ostream& gcp::util::operator<<(std::ostream& os, String str)
{
  os << str.str_;
  return os;
}

String String::findFirstInstanceOf(std::string stop)
{
  iStart_ = 0;
  return findNextInstanceOf("", false, stop, true);
}

String String::findFirstInstanceOf(std::string start, std::string stop)
{
  iStart_ = 0;
  return findNextInstanceOf(start, true, stop, true);
}

String String::findNextInstanceOf(std::string stop)
{
  return findNextInstanceOf("", false, stop, true);
}

String String::findNextInstanceOf(std::string start, std::string stop)
{
  return findNextInstanceOf(start, true, stop, true);
}

String String::findNextString()
{
  return findNextInstanceOf(" ", false, " ", false);
}

/**.......................................................................
 * Search a string for substrings separated by the specified start
 * and stop strings.  
 *
 * If useStart = true, then we will search for the start string first, then
 *                     search for the stop string, and return everything 
 *                     between them.
 * 
 *                     otherwise, we will just search for the end string
 *                     and return everything up to it.
 */
String String::findNextInstanceOf(std::string start, bool useStart, 
				  std::string stop,  bool useStop, bool consumeStop)
{
  String retStr;
  std::string::size_type iStart=0, iStop=0;

  // If we are searching for the next string separated by the start
  // and stop strings, then start by searching for the start string

  if(useStart) {

    iStart_ = str_.find(start, iStart_);

    // Return empty string if we hit the end of the string without
    // finding a match

    if(iStart_ == std::string::npos) {
      return retStr;
    }
    
    // Else start the search for the end string at the end of the
    // string we just found

    iStart_ += start.size();
  }

  // Now search for the stop string

  iStop = str_.find(stop, iStart_);

  // If we insist that the stop string is present, return an empty
  // string if it wasn't

  if(useStop) {
    if(iStop == std::string::npos) {
      iStart_ = iStop;
      return retStr;
    }
  }

  // Else match anything up to the stop string, if it was found, or
  // the end of the string if it wasn't

  retStr = str_.substr(iStart_, iStop-iStart_);

  // We will start the next search at the _beginning_ of the stop
  // string, so just increment iStart_ to point to the first char of
  // the stop string (or end of string, if the stop string wasn't found)
 
  iStart_ = iStop;

  // If consumeStop = true, then start the next search just _after_
  // the stop string (if it was found).

  if(consumeStop && iStop != std::string::npos) {
    iStart_ += stop.size();
  }

  return retStr;
}

bool String::atEnd()
{
  return iStart_ == std::string::npos;
}

int String::toInt()
{
  errno = 0;
  int iVal = strtol(str_.c_str(), NULL, 10);
  if(iVal==0 && errno==EINVAL)
    ThrowError("Cannot convert '" << str_ << "' to a valid integer");
  
  return iVal;
}

float String::toFloat()
{
  errno = 0;
  float fVal = strtof(str_.c_str(), NULL);
  if(errno==EINVAL)
    ThrowError("Cannot convert '" << str_ << "' to a valid float");

  return fVal;
}

double String::toDouble()
{
  errno = 0;
  double dVal = strtod(str_.c_str(), NULL);
  if(errno==EINVAL)
    ThrowError("Cannot convert '" << str_ << "' to a valid double");

  return dVal;
}

bool String::isEmpty()
{
  return str_ == "";
}

/**.......................................................................
 * Return the next string separated by any of the chars in separators
 */
String String::findNextStringSeparatedByChars(std::string separators, 
					      bool matchEndOfString)
{
  String retStr("");
  unsigned iStart=0, iStop=0;
  unsigned iEl=0;

  for(iEl=iStart_; iEl < str_.size(); iEl++) {
    if(matches(str_[iEl], separators)) {
      break;
    }
  }

  // If we hit the end of the string without finding a match, return
  // an empty string, and don't advance the start counter.  Unless the
  // separator string was the empty string, in which case we will
  // match end of string too

  if(iEl == str_.size() && !matchEndOfString)
    return retStr;

  // Else extract the portion of the string that matches

  retStr = str_.substr(iStart_, iEl-iStart_);

  // Now consume any trailing characters that also match, including whitespace

  for(unsigned iMatch = iEl; iMatch < str_.size(); iMatch++) {
    if(matches(str_[iMatch], separators) || matches(str_[iMatch], whiteSpace_))
      iEl++;
    else
      break;
  }

  // Advance iStart_ to the first non-matching character

  iStart_ = iEl;

  // And return the matched substring

  return retStr;
}

unsigned String::size()
{
  return str_.size();
}

void String::resetToBeginning()
{
  iStart_ = 0;
}

/**.......................................................................
 * Replace all occurrences of a character with a replacement character
 */
void String::replace(char stripChar, char replaceChar)
{
  replace(str_, stripChar, replaceChar);
}

/**.......................................................................
 * Replace all occurrences of a character with a replacement character
 */
void String::replace(std::string& targetStr, char stripChar, char replaceChar)
{
  bool replaced;

  do {
    replaced = false;
    
    std::string::size_type idx;
    idx = targetStr.find(stripChar);
    
    if(idx != std::string::npos) {
      targetStr[idx] = replaceChar;
      replaced = true;
    }
    
  } while(replaced);
}

String String::toLower()
{
  String retStr = str_;

  for(unsigned i=0; i < size(); i++) {
    retStr[i] = tolower(retStr[i]);
  }

  return retStr;
}

String String::toUpper()
{
  String retStr = str_;

  for(unsigned i=0; i < size(); i++) {
    retStr[i] = toupper(retStr[i]);
  }

  return retStr;
}
