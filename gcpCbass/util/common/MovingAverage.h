#ifndef GCP_UTIL_MOVING_AVERAGE_H
#define GCP_UTIL_MOVING_AVERAGE_H

#include <deque>
#include "gcp/util/common/Exception.h"

namespace gcp {
  namespace util {
    
    template<typename T> 
    class MovingAverage {
    public:
      MovingAverage(unsigned int numSamples);
      MovingAverage();
      bool valid() {return (samples_.size() == numSamples_);};
      void addSample(T sample);
      operator T() {
        if(samples_.size()) {
          return total_ / samples_.size();
        } else {
          return 0;
        }
      }; 
      void reset();
      void setNumSamples(unsigned int numSamples);
    private:
      MovingAverage(const MovingAverage<T>& rhs); // don't allow copy
      void operator=(const MovingAverage<T>& rhs); // don't allow assignment
      std::deque<T> samples_;
      T total_;
      unsigned int numSamples_;
    }; // End class MovingAverage
  
    template<typename T>
    MovingAverage<T>::MovingAverage(unsigned int numSamples) : numSamples_(numSamples), total_(0) {}; 

    template<typename T>
    MovingAverage<T>::MovingAverage() : numSamples_(1), total_(0) {}; 

    template<typename T>
    void MovingAverage<T>::addSample(T sample) {
      total_ += sample;
      samples_.push_back(sample);
      while(samples_.size() > numSamples_)
      {
        total_ -= samples_.front(); 
        samples_.pop_front();
      }
    }

    template<typename T>
    void MovingAverage<T>::reset() {
      samples_.clear();
      total_ = 0;
    }

    template<typename T>
    void MovingAverage<T>::setNumSamples(unsigned int numSamples) {
      numSamples_ = numSamples;
    }

  } // End namespace util
} // End namespace gcp

#endif // End #ifndef GCP_UTIL_MOVING_AVERAGE_H
