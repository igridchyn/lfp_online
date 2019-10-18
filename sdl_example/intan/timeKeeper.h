#ifndef TIMEKEEPER_H
#define TIMEKEEPER_H
#include <time.h>
#include <stdlib.h>
#include <stdio.h>

using namespace std;


class timeKeeper
{
 public:
  timeKeeper();
  struct timespec set_timespec_from_ms(double milisec);
  struct timespec diff(struct timespec* start, struct timespec* end);
  int microsecond_from_timespec(struct timespec* duration);

 private:
  struct timespec start; 
  struct timespec end;
  struct timespec duration;
  struct timespec req;
};



#endif // ACQUISITION_H
