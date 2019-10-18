/**************************************************************
 class to share data from the dataBuffer class with other 
 programs running on the same computer
 **************************************************************/
#ifndef SHARE_DATA_H
#define SHARE_DATA_H
#include <pthread.h> // to be able to create threads
#define KTANSHAREMEMORYDATANAME "/tmpktansharememorydata"
#define KTANSHAREMEMORYDATAPOINTS 2000000

using namespace std;

struct ktan_sm_data_struct
{
  short data[KTANSHAREMEMORYDATAPOINTS];
  int number_channels;
  int max_number_samples_in_shared_memory;
  int sampling_rate;
  unsigned long int number_samples_read; // total samples read since acq started and have been copied in the shared memory data
  unsigned long int oldest_sample_number; // oldest sample number currently in shared memory
  bool is_init;
  pthread_mutexattr_t attrmutex;
  int is_mutex_allocated;
  pthread_mutex_t pmutex;
};

class shared_memory_data
{
 public:
  shared_memory_data();
  ~shared_memory_data();
  void initialize(int number_channels,int sampling_rate);

 private:
  struct ktan_sm_data_struct* ksmd; // to share memory with other processes
  int shared_memory_size;
  int shared_memory_des;
};

#endif
