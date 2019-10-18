#ifndef ACQUISITION_H
#define ACQUISITION_H

#define CHIP_ID_RHD2132  1
#define CHIP_ID_RHD2216  2
#define CHIP_ID_RHD2164  4
#define CHIP_ID_RHD2164_B  1000
#define REGISTER_59_MISO_A  53
#define REGISTER_59_MISO_B  58
#define SAMPLES_PER_DATA_BLOCK 60 // also defined in rhd2000DataBlock.h
#define ACQUISITION_NUM_DIGITAL_INPUTS_CHANNELS 16
#define ACQUISITION_SLEEP_TIME_MS 0.5 // if > 2 and smaller than 20, I get problem with buffer overflow. don't know why

#include <string>
#include <queue>
#include "timeKeeper.h"
#include "dataBuffer.h"
#include <pthread.h> // to be able to create threads

using namespace std;
class Rhd2000EvalBoard;
class Rhd2000DataBlock;

class acquisition
{
 public:
  acquisition(dataBuffer* datab);
  ~acquisition();
  
  bool start_acquisition();
  bool stop_acquisition();
  bool get_is_acquiring();
  int get_number_channels();
  bool get_set_successfully();
  static void *acquisition_thread_helper(void *context) // helper function to start the new thread
  {
    ((acquisition *)context)->acquisition_thread_function();
  }
  void printLocalBuffer();
  void set_check_positrack(bool val);
  
  
 private:
  Rhd2000EvalBoard *evalBoard;
  int errorCode;
  bool fastSettleEnabled;
  bool set_successfully;


  // to compare system and opal clocks
  struct timespec acquisition_start_ts;
  struct timespec acquistion_duration_ts;
  struct timespec now_ts;
  double last_frame_delay_ms;
  
  
  // amplifier settings 
  double desiredDspCutoffFreq;
  double actualDspCutoffFreq;
  double desiredUpperBandwidth;
  double actualUpperBandwidth;
  double desiredLowerBandwidth;
  double actualLowerBandwidth;
  bool dspEnabled;
  
  
  bool openBoardBit();
  double desiredImpedanceFreq;
  double actualImpedanceFreq;
  bool impedanceFreqValid;
  
  bool* manualDelayEnabled;
  int* manualDelay;
  
  bool* auxDigOutEnabled;
  int* auxDigOutChannel;
  
  
  // digital to analog converters
  int numDacs;
  bool* dacEnabled;
  int* dacSelectedChannel;
  int* chipId;
  int evalBoardMode;
  unsigned long int numBlocksLoaded;
  

  // for leds
  int ttlOut[16];
  // variable changed with sampling rate
  int sampleRate;
  double boardSamplingRate;
  int numUsbBlocksToRead;
  // cable length for the different ports
  double cableLengthPortA;  // in meters
  double cableLengthPortB;  // in meters
  double cableLengthPortC;  // in meters
  double cableLengthPortD;  // in meters
  int numPorts;
  bool* portEnabled;
  queue<Rhd2000DataBlock> dataQueue;
  queue<Rhd2000DataBlock> bufferQueue;
  int numStreams;

  short int * localBuffer;

  // variables for acquisition
  bool newDataReady;
  unsigned int dataBlockSize;
  unsigned int wordsInFifo;
  double fifoPercentageFull;
  double fifoCapacity;
  double samplePeriod;
  double latency;
 

  // variables to operate the acquisition buffer
  
  int numChips;
  int numAmplifierChannels;
  int numDigitalInputChannels;
  int totalNumChannels;

  
  // to play with leds during acquisition
  int ledArray[8];
  int ledIndex;

   // variables inherited from kacq
  bool is_acquiring; // to send signal to the comedi thread
  int acquisition_thread_running; // set by the comedi thread when enter and exit
  pthread_t acquisition_thread;
  int acquisition_thread_id;

  dataBuffer* db; // pointer that will be given an address in the constructor of acquisition

  struct timespec time_last_sample_acquired_timespec; 
  struct timespec inter_acquisition_sleep_timespec; // between read operations to driver
  struct timespec inter_acquisition_timespec;
  struct timespec acquisition_duration_ts;
   double inter_acquisition_sleep_ms;
  struct timespec timespec_pause_restat_acquisition_thread; // allow acquisition to complete
  struct timespec req;
  double pause_restart_acquisition_thread_ms;
  timeKeeper tk;


  // function to operate the intan evaluation board
  bool openInterfaceBoard();
  bool findConnectedAmplifiers();
  void changeSampleRate(int sampleRateIndex);
  int deviceId(Rhd2000DataBlock *dataBlock, int stream, int &register59Value);
  void updateAuxDigOut();
  // thread functions
  void *acquisition_thread_function(void);
  int move_to_dataBuffer();
  void checkFifoOK();
  void advanceLED();
  void turnOffLED();
  void setDacThreshold1(int threshold);
  void setDacThreshold2(int threshold);
  void setDacThreshold3(int threshold);
  void setDacThreshold4(int threshold);
  void setDacThreshold5(int threshold);
  void setDacThreshold6(int threshold);
  void setDacThreshold7(int threshold);
  void setDacThreshold8(int threshold);
  void setDacOutput();
  
};



#endif // ACQUISITION_H
