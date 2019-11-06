#ifndef DATABUFFER_H
#define DATABUFFER_H
#define MAX_BUFFER_LENGTH 2000000 // data_buffer 
#include <string>
#include <queue>
#include "timeKeeper.h"
#include "shared_memory_data.h"
#include <pthread.h> // to be able to create threads

using namespace std;

/***************************************************************
buffer to hold the data comming from the acquisition process
that will be used by the recording and oscilloscope.

for efficiency new data will overwrite the older ones
in a wrap around manner.

This is more work to code but is more efficient

All modification to buffer occurs under a mutex lock so that
different thread can operate in parallel without racing conditions
or other problems
***************************************************************/ 
class dataBuffer
{
 public:
  dataBuffer();
  ~dataBuffer();
  void setNumChannels(int numChannels);
  int getNumChannels();
  void addNewData(int numSamples,short int* data); // transfer from acquition buffer to dataBuffer
  int getNewData(unsigned long int firstSample,short int* data, int maxSamples, int numChannels, unsigned int* channelList); // transfer from dataBuffer to rec or osc
  //  int getNewDataReverse(unsigned long int firstSample,double* data, int maxSamples, int numChannels, unsigned int* channelList, double factor_microvolt); depreciated
  void resetData();
  void set_sampling_rate(int sr);
  int get_sampling_rate();
  unsigned long int get_number_samples_read();
  
 private:
  pthread_mutex_t data_buffer_mutex;
  unsigned long int number_samples_read; // total samples read since acq started
  unsigned long int oldest_sample_number; // oldest sample number currently in buffer
  int number_samples_in_buffer; // number of valid samples in buffer
  int max_number_samples_in_buffer; // number of valid samples in buffer when full
  int index_next_sample;
  int buffer_size;
  int number_channels;
  short int* buffer;// buffer to get data from comedi devices
  int addAtEnd;
  int sampling_rate; // in Hz;

};


#endif
