#include <iostream>

#include <utility>

#include <vector>
#include <deque>

using namespace std;

#include "gcp/util/common/Exception.h"
#include "gcp/util/common/LogStream.h"
#include "gcp/util/common/TimeVal.h"

#include "gcp/program/common/Program.h"

using namespace gcp::util;
using namespace gcp::program;

KeyTabEntry Program::keywords[] = {
  {"nq",        "1000",   "i", "Length of the filter function"},
  {"niter",     "100000", "i", "Number of sample to iterate over"},
  {"construct", "t",      "b", "true|false -- construct objects in the loop"},
  {"doQ",       "t",      "b", "true|false -- use queues?"},
  {END_OF_KEYWORDS}
};

void Program::initializeUsage() {};

//=======================================================================
// Test code 
//=======================================================================

void runWithQueue(unsigned qlen, unsigned niter, bool construct);
void runWithLists(unsigned qlen, unsigned niter);

int Program::main(void)
{
  unsigned qlen  = Program::getiParameter("nq");
  unsigned niter = Program::getiParameter("niter");
  bool construct = Program::getbParameter("construct");
  bool doQ       = Program::getbParameter("doQ");

  TimeVal tValStart, tValStop, tDiff1, tDiff2;

  tValStart.setToCurrentTime();
  runWithQueue(qlen, niter, construct);
  tValStop.setToCurrentTime();
  tDiff1 = tValStop - tValStart;

  tValStart.setToCurrentTime();
  runWithLists(qlen, niter);
  tValStop.setToCurrentTime();
  tDiff2 = tValStop - tValStart;

  COUT(endl << 
       "Elapsed times: " << endl << endl
       << "  deque: " << tDiff1.getTimeInSeconds() << " seconds" << endl
       << "  lists: " << tDiff2.getTimeInSeconds() << " seconds" << endl
       << "  ratio: " << tDiff1.getTimeInSeconds()/tDiff2.getTimeInSeconds() << endl); 

  return 0;
}

//=======================================================================
// Test using queues
//=======================================================================

double popSample(std::deque<pair<double, unsigned> >& dq);
void pushNewIntegration(std::deque<pair<double, unsigned> >& dq);
void addSampleToComputation(std::deque<pair<double, unsigned> >& dq, short int sample);

void runWithQueue(unsigned qlen, unsigned niter, bool construct)
{
  std::deque<pair<double, unsigned> > dq;
  short int si=2;

  for(unsigned i=0; i < qlen; i++) {
    pushNewIntegration(dq);
  }

  cout << "dq is of length: " << dq.size() << endl;

  for(unsigned i=0; i < niter; i++) {

    if(construct) {
      popSample(dq);
      pushNewIntegration(dq);
    }

    addSampleToComputation(dq, si);
  }

}

double popSample(std::deque<pair<double, unsigned> >& dq)
{
  double retval=dq.front().first;
  dq.pop_front();
  return retval;
}

void pushNewIntegration(std::deque<pair<double, unsigned> >& dq)
{
  dq.push_back(make_pair<double, unsigned>(0.0, 0));
}

void addSampleToComputation(std::deque<pair<double, unsigned> >& dq, short int sample)
{
  for(deque<pair<double, unsigned> >::iterator iter=dq.begin(); iter != dq.end(); iter++){
    iter->first += sample;
    iter->second++;
  }
}

//=======================================================================
// Test with naked linked lists
//=======================================================================

namespace queueTest {

  class Node {
  public:
    double first_;
    unsigned second_;
    unsigned short id_;
    Node* next_;
    Node* prev_;
    
    Node() {
      first_  = 0.0;
      second_ = 0;
      id_     = 0;
      next_   = 0;
      prev_   = 0;
    }

    virtual ~Node() {};
  };

}

//=======================================================================
// List version
//=======================================================================

void addSampleToComputation(queueTest::Node* start, unsigned qlen, short int sample);

void runWithLists(unsigned qlen, unsigned niter)
{
  short int si=2;
  std::vector<queueTest::Node> dq;
  queueTest::Node* head=0;

  dq.resize(qlen);

  // Turn the vector into a doubly-linked list

  head = &dq[0];
  for(unsigned i=0; i < qlen; i++) {
    dq[i].next_ = &dq[((i==qlen-1) ? 0 : i+1)];
    dq[i].prev_ = &dq[((i==0) ? qlen-1 : i-1)];
    dq[i].id_ = i;
  }

  // Traverse the list

  cout << "dq is of length: " << dq.size() << endl;

  for(unsigned i=0; i < niter; i++) {
    head = head->next_;
    addSampleToComputation(head, qlen, si);
  }
}

void addSampleToComputation(queueTest::Node* start, unsigned qlen, short int sample)
{
  queueTest::Node* iter=start;

  for(unsigned i=0; i < qlen; i++, iter=iter->next_) {
    iter->first_ += sample;
    iter->second_++;
  }
}
