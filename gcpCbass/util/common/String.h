#ifndef GCP_UTIL_STRING_H
#define GCP_UTIL_STRING_H

/**
 * @file String.h
 * 
 * Tagged: Wed May 12 09:30:13 PDT 2004
 * 
 * @author Erik Leitch
 */
#include <string>

namespace gcp {
  namespace util {
    
    class String {
    public:
      
      /**
       * Constructor.
       */
      String();
      String(unsigned int);
      String(const std::string& str);
      
      /**
       * Destructor.
       */
      virtual ~String();
      
      static void strip(std::string& targetStr, const std::string& stripStr);
      static void strip(std::string& targetStr, char stripChar);

      void strip(const std::string& stripStr);
      void strip(char stripChar);
      
      bool contains(char target);
      
      bool contains(std::string s);

      void replace(char stripChar, char replaceChar);
      static void replace(std::string& targetStr, char stripChar, char replaceChar);

      void operator=(const std::string& str);
      void operator=(const String str);
      
      bool operator<(String& str); 
      bool operator==(String str);
      bool operator!=(String str);

      char& operator[](unsigned int index);

      inline std::string& str()
	{
	  return str_;
	}
      
      /**
       * Allows cout << String
       */
      friend std::ostream& operator<<(std::ostream& os, String str);
      
      String findFirstInstanceOf(std::string start, bool useStart, 
				 std::string stop, bool useStop);

      String findFirstInstanceOf(std::string start, std::string stop);

      String findFirstInstanceOf(std::string stop);

      String findNextInstanceOf(std::string start, bool useStart, 
				std::string stop, bool useStop, bool consumeStop=false);

      String findNextInstanceOf(std::string start, std::string stop);

      String findNextInstanceOf(std::string stop);

      String findNextString();

      String toLower();
      String toUpper();

      static const std::string emptyString_;
      static const std::string whiteSpace_;

      String findNextStringSeparatedByChars(std::string separators, bool matchEndOfString=true);
      bool atEnd();
      void resetToBeginning();

      void initialize();
      
      int toInt();
      float toFloat();
      double toDouble();

      bool isEmpty();
      bool matches(unsigned char c, std::string matchSet);

      unsigned size();

    private:
      
      std::string::size_type iStart_;
      std::string str_;
      
    }; // End class String
    
  } // End namespace util
} // End namespace gcp




#endif // End #ifndef GCP_UTIL_STRING_H
