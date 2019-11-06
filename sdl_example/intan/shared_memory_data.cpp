/**************************************************************
 class to share data with other
 programs running on the same computer
 **************************************************************/
//#define DEBUG_SHARE_DATA
#include "shared_memory_data.h"
#include <iostream>
#include <stdlib.h> 
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <pthread.h>


shared_memory_data::shared_memory_data()
{
  #ifdef DEBUG_SHARE_DATA
  cerr << "entering shared_memory_data::shared_memory_data()\n";
  #endif
  
  shm_unlink(KTANSHAREMEMORYDATANAME); // just in case
  shared_memory_size=sizeof(struct ktan_sm_data_struct);
  shared_memory_des=shm_open(KTANSHAREMEMORYDATANAME, O_CREAT | O_RDWR | O_TRUNC, 0600);
  
  if(shared_memory_des ==-1)
     {
       cerr << "problem with shm_open in shared_memory_data::shared_memory_data()\n";
       return;
     } 
  if (ftruncate(shared_memory_des,shared_memory_size) == -1)
    {
      cerr << "problem with ftruncate in shared_memory_data::shared_memory_data()\n";
      return;
    }
  ksmd = (struct ktan_sm_data_struct*) mmap(0, shared_memory_size, PROT_READ | PROT_WRITE, MAP_SHARED, shared_memory_des, 0);
  if (ksmd == MAP_FAILED) 
    {
      cerr << "problem with mmap in shared_memory_data::shared_memory_data()\n;";
      return;
    }

  ksmd->is_init=false;
  
  /* Initialise attribute to mutex. */
  pthread_mutexattr_init(&ksmd->attrmutex);
  pthread_mutexattr_setpshared(&ksmd->attrmutex, PTHREAD_PROCESS_SHARED);
  /* Initialise mutex. */
  pthread_mutex_init(&ksmd->pmutex, &ksmd->attrmutex);
#ifdef DEBUG_SHARE_DATA
  cerr << "leaving shared_memory_data::shared_memory_data()\n";
#endif

}


shared_memory_data::~shared_memory_data()
{
#ifdef DEBUG_SHARE_DATA
  cerr << "entering shared_memory_data::~shared_memory_data()\n";
#endif
  pthread_mutex_destroy(&ksmd->pmutex);
  pthread_mutexattr_destroy(&ksmd->attrmutex);
  // unmap the shared memory
  if(munmap(ksmd,shared_memory_size) == -1) 
    {
      cerr << "problem with munmap in shared_memory_data::~shared_memory_data()\n";
      return;
    }
  shm_unlink(KTANSHAREMEMORYDATANAME);
#ifdef DEBUG_SHARE_DATA
  cerr << "leaving shared_memory_data::~shared_memory_data()\n";
#endif
}

void shared_memory_data::initialize(int number_channels,int sampling_rate)
{
#ifdef DEBUG_SHARE_DATA
  cerr << "entering shared_memory_data::initialize()\n";
#endif

  pthread_mutex_lock(&ksmd->pmutex);
  ksmd->number_channels=number_channels;
  ksmd->sampling_rate=sampling_rate;
  if(number_channels!=0)
    ksmd->max_number_samples_in_shared_memory=KTANSHAREMEMORYDATAPOINTS/number_channels;
  ksmd->number_samples_read=0;
  ksmd->oldest_sample_number=0; 
  ksmd->is_init=true;
  pthread_mutex_unlock(&ksmd->pmutex);
    
#ifdef DEBUG_SHARE_DATA
  cerr << "shared_memory_data::initialize()\n";
  cerr << "number_channels: " << ksmd->number_channels << '\n';
  cerr << "sampling_rate: " << ksmd->sampling_rate << '\n';
  cerr << "max_number_samples_in_shared_memory: " << ksmd->max_number_samples_in_shared_memory << '\n';
  cerr << "number_samples_read: " << ksmd->number_samples_read << '\n';
  cerr << "oldest_sample_number: " << ksmd->oldest_sample_number << '\n';
#endif

  
#ifdef DEBUG_SHARE_DATA
  cerr << "leaving shared_memory_data::initialize()\n";
#endif

}
