#ifndef __SIMMAIN_H__
#define __SIMMAIN_H__

#include <chrono>

class SimMain : public CBase_SimMain {

 private:
  /// Member Variables (Object State) ///
  int numElements;
  int doneCount;

  std::chrono::time_point<std::chrono::high_resolution_clock> start_time;
  std::chrono::time_point<std::chrono::high_resolution_clock> end_time;

 public:

  /// Constructors ///
  SimMain(CkArgMsg* msg);
  SimMain(CkMigrateMessage* msg);

  /// Entry Methods ///
  void done();

};


#endif //__SIMMAIN_H__
