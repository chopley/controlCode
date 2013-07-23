#ifndef GCP_UTIL_CONFORMABLEQUANTITY_H
#define GCP_UTIL_CONFORMABLEQUANTITY_H

/**
 * @file ConformableQuantity.h
 * 
 * Tagged: Wed Dec  1 11:44:54 PST 2004
 * 
 * @author GCP data acquisition
 */
namespace gcp {
  namespace util {
    
    // A pure interface for unit'ed quantities
    
    class ConformableQuantity {
    public:
      
      /**
       * Constructor.
       */
      ConformableQuantity() {};
      
      /**
       * Destructor.
       */
      virtual ~ConformableQuantity() {};
      
      virtual void initialize() {};
      
      bool isFinite() {
	return finite_;
      }
      
    protected:
      
      void setFinite(bool finite) {
	finite_ = finite;
      }
      
      bool finite_;
      
    }; // End class ConformableQuantity
    
  } // End namespace util
} // End namespace gcp




#endif // End #ifndef GCP_UTIL_CONFORMABLEQUANTITY_H
