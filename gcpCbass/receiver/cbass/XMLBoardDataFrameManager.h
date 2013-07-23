#ifndef GCP_RECEIVER_XMLBOARDDATAFRAMEMANAGER_H
#define GCP_RECEIVER_XMLBOARDDATAFRAMEMANAGER_H

/**
 * @file XMLBoardDataFrameManager.h
 * 
 * Tagged: Wed Mar 31 09:59:54 PST 2004
 * 
 * @author Erik Leitch
 */

// C header files from the array control code

#include "gcp/control/code/unix/libunix_src/specific/specificregs.h"

#include "Utilities/MuxXMLDatacard.h"

#include "gcp/util/common/BoardDataFrameManager.h"

#include "gcp/util/common/Exception.h"

#include <map>
#include <vector>

namespace gcp {
  namespace receiver {
      
    class DataFrame;
      
    class XMLBoardDataFrameManager : 
      public gcp::util::BoardDataFrameManager {

      public:
	
      /**
       * Constructor with initialization from a DataFrame object.
       */
      XMLBoardDataFrameManager(std::string regmap, std::string board,
			       bool archivedOnly=false);
	
	
      virtual void setTo(MuxReadout::MuxXMLFile *xml);

      /**
       * Destructor.
       */
      virtual ~XMLBoardDataFrameManager();
	
      class XMLRegisterMap {
      public:
	    
	virtual void 
	  set(unsigned int addr, int value, unsigned int boardNum) = 0;

	virtual void 
	  set(unsigned int addr, double value, unsigned int boardNum) = 0;

	virtual void 
	  set(std::string name, double value, unsigned int boardNum) {};

	virtual void 
	  set(std::string name, std::vector<double> &values, 
	      unsigned int boardNum) {};

	virtual void 
	  set(std::string name, int value, unsigned int boardNum) = 0;

	virtual void 
	  set(std::string name, std::vector<int> &values, 
	      unsigned int boardNum) {};

	virtual void 
	  set(std::string name, std::string value, 
	      unsigned int boardNum) = 0;

	virtual void write() = 0;

	template <class valueType> class RegisterList {
	public:
	      
	  RegisterList(BoardDataFrameManager* frame, 
		       unsigned int index1Size=0, 
		       unsigned int index2Size=0, 
		       unsigned int index3Size=0) :
	    frame_(frame), minMappedAddress_(0), index1Size_(index1Size), 
	    index2Size_(index2Size), index3Size_(index3Size) {};
	          
	private:
	      
	  bool addMapped(char* regName)
	    {
	      RegMapBlock* regBlock = NULL;
	      try {
		regBlock = frame_->getReg(regName);
	      } catch (...) {
		ReportError("Register " 
			    << regName << " not found.");
	      }

	      if(regBlock != NULL) {
		registers_.push_back(regBlock);
		unsigned int maxIndex;
		if(index3Size_) {
		  maxIndex = index3Size_ * index2Size_ * index1Size_;
		} else if (index2Size_) {
		  maxIndex = index2Size_ * index1Size_;
		} else {
		  maxIndex = index1Size_;
		}

		std::vector<valueType>* buffPtr = 
		  new std::vector<valueType>(maxIndex);

		buffers_.push_back(buffPtr);

		return true;

	      } else {
		ReportError(" register " 
			    << regName << " not found");
		return false;
	      }
	    }
                
	public:
                
	  void add(char* regName) 
	    {
	      if(addMapped(regName)) {
		minMappedAddress_++;
	      }
	    }
                
	  void add(char* regName, unsigned int address)
	    {
	      if((addressToIndex_.find(address) != addressToIndex_.end()) 
		 || (address < minMappedAddress_)) {
		ReportError("Duplicate register address " << regName 
			    << " " << address);
		return;
	      }

	      addressToIndex_[address] = registers_.size();

	      addMapped(regName);
	    }	
                
	  void add(char* regName, std::string xmlTag)
	    {
	      if(xmlTagToIndex_.find(xmlTag) != xmlTagToIndex_.end()) {
		ReportError("Duplicate register tag " << regName 
			    << " " << xmlTag);
		return;
	      }
                  
	      xmlTagToIndex_[xmlTag] = registers_.size();
	      addMapped(regName);
	    }
                            
	  void setResolved(unsigned int registerNum, valueType value, 
			   unsigned int index1=0, unsigned int index2=0, 
			   unsigned int index3=0) 
	    {
	      if(registerNum >= buffers_.size()) {
		ReportError("Register number " << registerNum 
			    << " out of range ");
		return;
	      }
	          
	      unsigned int index;
	      if(index3Size_) {
		index = index1*index2Size_*index3Size_ + 
		  index2*index3Size_ + index3;
	      } else if(index2Size_) {
		index = index1 * index2Size_ + index2;
	      } else {
		index = index1;
	      }

	      (*static_cast< std::vector<valueType>* >(buffers_[registerNum]))[index] = value;
	    };
	        
	  void set(unsigned int registerNum, valueType value, 
		   unsigned int index1=0, 
		   unsigned int index2=0, 
		   unsigned int index3=0)
	    {
	      if(registerNum >= minMappedAddress_) {

		std::map<unsigned int, unsigned int>::iterator 
		  indexIter = addressToIndex_.find(registerNum);

		if(indexIter == addressToIndex_.end()) {
		  //		  ReportError("Register number " << registerNum 
			      //			      << " not defined");
		  return;
		}
		registerNum = indexIter->second;
	      }
	      setResolved(registerNum, value, index1, index2, index3);
	    }
       		  
	  typedef std::map<std::string, unsigned int> StringIntMap; 
		        
	  void set(std::string xmlTag, valueType value, 
		   unsigned int index1=0, unsigned int index2=0, 
		   unsigned int index3=0) 
	    {
	      StringIntMap::iterator tagIter = xmlTagToIndex_.find(xmlTag);
	      if(tagIter != xmlTagToIndex_.end()) {
		setResolved(tagIter->second, value, index1, index2, index3);
	      }
	    }
                
	  void set(std::string xmlTag, std::vector<valueType> &values, 
		   unsigned int index1=0, unsigned int index2=0)
	    {
	      StringIntMap::iterator tagIter = xmlTagToIndex_.find(xmlTag);
	      if(tagIter != xmlTagToIndex_.end()) {
		for(int index = 0; index < values.size(); index++) {
		  setResolved(tagIter->second, values[index], index);
		}
	      }
	    }

	  void set(std::string xmlTag, std::vector<int> &values)
	    {
	      StringIntMap::iterator tagIter = xmlTagToIndex_.find(xmlTag);
	      if(tagIter != xmlTagToIndex_.end()) {
		for(int index = 0; index < values.size(); index++) {
		  setResolved(tagIter->second, (valueType)values[index], index);
		}
	      }
	    }
                  
	  void write() 
	    {
	      std::vector<void*>::iterator 
		bufIter = buffers_.begin();

	      std::vector<RegMapBlock*>::iterator 
		regIter = registers_.begin();

	      while(regIter != registers_.end()) {
		try {
		  frame_->writeBoardReg(*regIter++, static_cast<valueType*>(&(*static_cast< std::vector<valueType>* >(*bufIter++))[0]));
		} catch(gcp::util::Exception& err) {
		  COUT(err.what());
		}
	      }

	    }   
	       
	private:

	  BoardDataFrameManager* frame_;
	  unsigned int minMappedAddress_;
	  std::map<unsigned int, unsigned int>addressToIndex_;
	  StringIntMap xmlTagToIndex_;
	  std::vector<RegMapBlock*>registers_;
	  std::vector< void* >buffers_;
	  unsigned int index1Size_;
	  unsigned int index2Size_;
	  unsigned int index3Size_;
	}; // RegisterList
	    
	template<class dataType> 
	  void add(RegisterList<dataType>* registers,
		   char* regName, std::string xmlTag, 
		   std::string dataType) {

	  registers->add(regName, xmlTag);
	  namedRegisters_.push_back(NamedRegister(xmlTag, dataType));
	}	    
	  
	class NamedRegister {
	public:

	  NamedRegister(std::string name, std::string typeName) : 
	    name_(name), typeName_(typeName) {};

	  std::string name_;
	  std::string typeName_;
	};
	    	    	    	    	        
	std::vector<NamedRegister> namedRegisters_;

      }; // XMLRegisterMap
	
      protected:
	
      void getRegisters(MuxReadout::MuxXMLFile* xml,
			std::vector<std::string>& containerNames,
			std::string addressVectorName,
			std::string valueVectorName,
			XMLRegisterMap& lookup,
			unsigned int boardNum);
	
    }; // XMLBoardDataFrameManager
      
  } // End namespace receiver
}; // End namespace gcp




#endif // End #ifndef GCP_RECEIVER_XMLDATAFRAMEMANAGER_H
