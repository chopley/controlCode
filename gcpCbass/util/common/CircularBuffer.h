#ifndef GCP_UTIL_CIRCULARBUFFER_H
#define GCP_UTIL_CIRCULARBUFFER_H

/**
 * @file CircularBuffer.h
 * 
 * Tagged: Sun Mar 21 22:28:53 UTC 2004
 * 
 * @author Erik Leitch
 */
#include <vector>
#include <map>

#include "gcp/util/common/Mutex.h"
#include "gcp/util/common/Exception.h"

namespace gcp {
  namespace util {
    
    class type;
    
    template<class type>
      class CircularBuffer {
      public:
      
      /**
       * Constructor.
       */
      CircularBuffer(unsigned nFrame);
      
      /**
       * Destructor.
       */
      virtual ~CircularBuffer();
      
      /**
       * Return a pointer to the frame with the passed id.  If no
       * frame with that id exists, and create == true, this call will
       * create it.  If no frame with that id exists, and create ==
       * false, this call returns NULL.
       */
      type* getFrame(unsigned int id, bool create);

      /**
       * Return a pointer to the next free data frame in the buffer.
       * Creates a new frame on every call.
       */
      type* getNextFrame();
      
      /**
       * Return a pointer to the next slot to be dispatched to the
       * outside world.  Using this method guarantees that the
       * returned pointer will not be modified until the next call to
       * dispatchNextFrame(), since the type object
       * returned is locked until dispatchNextFrame() is called again.
       */
      type* dispatchNextFrame();
      
      /**
       * Public method to query how many frames are waiting in the queue.
       */
      unsigned int getNframesInQueue();
      
      protected:
      
      /**
       * Create a struct that we will use as an internal buffer.
       */
      struct CircularBufferSlot {
	unsigned int id_;
	type* frame_;
	struct CircularBufferSlot* next_;
      };
      
      /**
       * A vector of slots
       */
      std::vector<struct CircularBufferSlot> slots_;
      
      /**
       * The number of frames in our buffer
       */
      unsigned long nSlot_;           
      
      private:
      
      /**
       * A guard mutex for this frame buffer.
       */
      Mutex guard_;
      
      /**
       * A map to help with searching
       */
      std::map<unsigned int, struct CircularBufferSlot*> frameMap_;
      
      /**
       * Pointer to the next free slot
       */
      struct CircularBufferSlot* nextFreeSlot_;    
      
      /**
       * Pointer to the next slot to be dispatched to the outside
       * world.
       */
      struct CircularBufferSlot* nextSendSlot_;    
      
      /**
       * Pointer to the last slot dispatched to the outside world.
       */
      struct CircularBufferSlot* lastSentSlot_;    
      
      /**
       * The number of slots currently in use.
       */
      unsigned long nUsed_;            
      
      /**
       * Find a slot by id number
       */
      struct CircularBufferSlot* findSlot(unsigned int id);

      /**
       * Get the next free slot
       */
      struct CircularBufferSlot* getNextSlot();
      
      /**
       * Clear a previously used slot in the frame buffer
       */
      void clearSlot(CircularBufferSlot* slot);
      
    }; // End class CircularBuffer

    //=======================================================================
    // Methods
    //=======================================================================

    /**.......................................................................
     * Constructor
     */
    template<class type>
      CircularBuffer<type>::CircularBuffer(unsigned int nSlot)
      {
	nSlot_  = nSlot;
	nUsed_  = 0;
  
	// Initialize our index pointers to NULL
  
	nextFreeSlot_ = 0;
	nextSendSlot_ = 0;
	lastSentSlot_ = 0;

	// Check passed arguments

	if(nSlot_ < 1)
	  ThrowError("nSlot < 1");
  
	// Initialize each type to have a buffer large enough to
	// accomodate a frame
  
	slots_.resize(nSlot_);
  
	// Turn the vector into a circular linked list for convenience of
	// traversal.
  
	for(unsigned islot=0; islot < nSlot_-1; islot++)
	  slots_[islot].next_ = &slots_[islot+1];
	slots_[nSlot_-1].next_ = &slots_[0];

	// And set both index pointers pointing to the first element.
	// lastSentSlot we leave NULL, so we can tell when no frames have
	// been sent.

	nextFreeSlot_ = &slots_[0];
	nextSendSlot_ = &slots_[0];
      }

    /**.......................................................................
     * Destructor.
     */
    template<class type>
      CircularBuffer<type>::~CircularBuffer() {}

    /**.......................................................................
     * Return a pointer to the next slot to be dispatched to the outside
     * world.
     */
    template<class type>
      type* CircularBuffer<type>::dispatchNextFrame()
      {
	// If there are no slots waiting to be dispatched, return NULL

	if(nUsed_ == 0)
	  return 0;

	// Else set the return pointer pointing to the next frame to be sent
  
	type* dfm = nextSendSlot_->frame_;

	// Remove this slot from the map of known slots.  While it is being
	// sent, we don't want anyone else to find it as a valid slot.

	frameMap_.erase(nextSendSlot_->id_);

	// Lock the frame.  This protects against writers trying to modify
	// the frame while the calling thread is attempting to send it.

	dfm->lock();

	// Only decrement the count of frames waiting to be sent if this
	// call means we just finished sending a frame.
  
	if(lastSentSlot_ != 0) {
	  clearSlot(lastSentSlot_);
	  nUsed_--;
	}

	// Set the last sent frame pointing to the one we are
	// currently dispatching

	lastSentSlot_ = nextSendSlot_;

	// And increment the next send frame pointer

	nextSendSlot_ = nextSendSlot_->next_;

	return dfm;
      }

    /**.......................................................................
     * Return a pointer to the next free slot in the buffer, or NULL
     * if there are none.
     */
    template<class type>
      struct CircularBuffer<type>::CircularBufferSlot* 
      CircularBuffer<type>::getNextSlot()
      {
	struct CircularBufferSlot* slot = nextFreeSlot_;
  
	// If we have come full circle in the buffer, start eating our tail

	if(nUsed_ == nSlot_) {

	  clearSlot(nextSendSlot_);
	  nextSendSlot_ = nextSendSlot_->next_;

	  // Only increment nUsed if the buffer is not already full

	} else {
	  nUsed_++;
	}
    
	// Set the index pointer pointing to the next free slot

	nextFreeSlot_ = nextFreeSlot_->next_;
  
	return slot;
      }

    /**.......................................................................
     * Return a pointer to the next available frame in the buffer
     */
    template<class type>
      type* CircularBuffer<type>::getNextFrame()
      {
	struct CircularBufferSlot* slot = getNextSlot();

	if(slot != 0)
	  return slot->frame_;

	return 0;
      }

    /**.......................................................................
     * Return a pointer to a data frame in the buffer.  If a slot with the
     * passed id has already been installed, that slot will be returned.
     * If no slots matching the passed id were found, and create == true,
     * the next free slot will be initialized with the passed id, and its
     * pointer returned.  Else NULL will be returned if no slot with the
     * passed id was found, and create == false.
     */
    template<class type>
      type* CircularBuffer<type>::getFrame(unsigned int id, bool create)
      {
	type* frame=0;
	bool wasErr = false;

	// Lock the frame buffer.  We do this so that no other call to
	// getFrame() can modify the slot map while we are searching it.
	// 
	// Even with interlocking, we can still run into trouble if multiple
	// threads are simultaneously creating slots with create == true,
	// since the returned frame may refer to a slot whose id has since
	// been modified by another call to this method, but this is not my
	// usage model.  What I'm concerned about is a single thread
	// creating frames with create == true, and multiple threads looking
	// them up with create == false.  In this case, as long as we use
	// interlocking, we are guaranteed that one thread will not search
	// the map while another thread is in the process of modifying it,
	// and vice versa.

	guard_.lock();

	try {

	  // Search the map for a match
    
	  struct CircularBufferSlot* slot = findSlot(id);
    
	  // If a slot with that id was found, return it.  If no slot was
	  // found, and we are not creating slots with this call, return
	  // NULL.
    
	  if(slot != 0 || !create) {
	    if(slot == 0)
	      frame = 0;
	    else 
	      frame = (slot == 0) ? 0 : slot->frame_;
	  } else {

	    // Else create the slot

	    slot = getNextSlot();
      
	    // If no free slots were found, slot will be NULL.  
      
	    if(slot!=0) {
      
	      // If a free slot was found, insert it into the map of
	      // tagged frames
	
	      frameMap_[id] = slot;
	
	      // And set the id of this slot
	
	      slot->id_ = id;
	
	      // Set the return value

	      frame = slot->frame_;
	    }
	  }
	} catch (...) {
	  wasErr = true;
	}

	// Release the frame buffer guard mutex

	guard_.unlock();

	// If an error occurred, throw it here.

	if(wasErr)
	  ThrowError("Caught an exception");

	// Else return the frame

	return frame;
      } 

    /**.......................................................................
     * Find a slot by id number
     */
    template<class type>
      struct CircularBuffer<type>::CircularBufferSlot* 
      CircularBuffer<type>::findSlot(unsigned int id)
      {
	std::map<unsigned int, struct CircularBufferSlot*>::iterator slot;
	
	slot = frameMap_.find(id);
  
	if(slot != frameMap_.end())
	  return slot->second;
  
	return 0;
      }

    /**.......................................................................
     * Public method to query how many frames are waiting in the queue.
     */

    template<class type>
      unsigned int CircularBuffer<type>::getNframesInQueue()
      {
	return nUsed_;
      }

    /**.......................................................................
     * Clear a previously used slot in the frame buffer
     */
    template<class type>
      void CircularBuffer<type>::clearSlot(CircularBufferSlot* slot)
      {
	// Delete this slot's entry in the map

	frameMap_.erase(slot->id_); 

	// Make sure this frame is unlocked

	slot->frame_->unlock();     

	// And reinitialize its contents.

	slot->frame_->reinitialize();
      }
    
  } // End namespace util
} // End namespace gcp


#endif // End #ifndef GCP_UTIL_CIRCULARBUFFER_H
