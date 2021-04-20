// g++ -Wall -o trigger trigger.cpp -lrt -lpthread

// TODO
// 	 ? which samples to read from the available array ?
//   ? TIMESTAMP ?
//   test no tracking - stop after 2 seconds
//   test immobility
//   ADJUST IMMO CONSTANTS

#include <sys/io.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <iostream>
#include <fstream>
#include <time.h>
#include <pthread.h>
#include <math.h>
#include <queue>
#include <cmath>
#define POSITRACKSHARE "/tmppositrackshare"
#define POSITRACKSHARENUMFRAMES 100
//#define BASE 0xe010
#define BASE 0xd010
#define PI 3.14159265

// inhibition stratedy: inhibit in sector; immobile > 2s => stop; write timestamps when trigger started and ended

using namespace std;
struct positrack_shared_memory
{
	int numframes;
	unsigned long int id [POSITRACKSHARENUMFRAMES]; // internal to this object, first valid = 1, id 0 is invalid
	unsigned long int frame_no [POSITRACKSHARENUMFRAMES]; // from tracking system, frame sequence number
	struct timespec ts [POSITRACKSHARENUMFRAMES];
	double x[POSITRACKSHARENUMFRAMES]; // position x
	double y[POSITRACKSHARENUMFRAMES]; // position y
	double hd[POSITRACKSHARENUMFRAMES]; // head direction
	pthread_mutexattr_t attrmutex;
	int is_mutex_allocated;
	pthread_mutex_t pmutex;
	};
	struct psm;
	using namespace std;
	struct timespec ts_now;

bool in_sector(int cx, int cy, int ss, int se, double x, double y, int shape, double radius){
	switch(shape){
	case 0: {// SECTOR
		// in the range [0 ; 360]
		float angle = atan2(y-cy, x-cx) * 180 / PI + 180; 
		//cout << "ANGLE: " << angle << "\n";
		return angle > ss && angle < se;
		break;
	}

	case 1:{ // CIRCLE
		return (y-cy)*(y-cy) + (x-cx)*(x-cx) < radius * radius;
		break;
	}

	case 2:{ // RECT
		return x > cx && y > cy && x < ss && y < se;
		break;
	}

	default:
		cout << "UNSOPPORTED SHAPE ARGUMENT";
		exit(1);
	}	
}

bool pattern_on(long long pattern_start, long long time, int light_interval, int pause){
	long long rem = remainder(time - pattern_start, light_interval + pause);
	//cout << time-pattern_start << " " << light_interval + pause << " " << rem << "\n";
	bool PON = (rem > 0) && (rem < light_interval);
	return PON;
}

void ttl_off(ofstream& timestamps, unsigned char pstate, bool & ttl_on_new, const long long & time_msec){
	pstate = pstate & 239;
	timestamps << time_msec << "\n";
	timestamps.flush();
	//timestamps << frame_no << "\n";
	outb(pstate, BASE);
	ttl_on_new = false;
	timestamps.flush();
}

int main(int argc, char *argv[])
	{
	if (argc < 3){
		cout << "USAGE: trigger (1)<file with geometry/light pattern params>:\n";
		cout << "		g1 g2 g3 g4 shape(0=sector, 1=circle, 2=rectangle) \n";
		cout << "		pattern_pulse_duration(ms) pattern_pause_duration(ms) pattern_duration(ms) pattern_minimal_frames_inside\n";
		cout << "		immobility_speed_threshold immobility_time\n";
		cout << "		pattern_mode\n\n";

		cout << " 		geometry params: 0=sector: (center x, center y, sector start angle, sector end angle); 1=circle(center x, center y, radius); 2=rectangle(min x, min y, max x, max y)\n";
		cout << "		pattern_mode = 1 if pattern needs to be executed when animal eneters the trigger zone; = 0 for simply light on inside zone, light off outside zone\n\n";
		cout << " 		(2)<timestamps ouput file>\n\n";
		exit(0);
	}

	cout << "Version 0.8\n";

	// read geometry params: center and sector start / end
	ifstream fparams(argv[1]);
	// SHAPE TYPES: 0=SECTOR, 2=CIRCLE, 3=RECT
	int cx, cy, ss, se, IMMO_LIMIT, SHAPE;
	double IMMO_THOLD_SQ = 0.0, RAD;
	int PATTERN_PULSE, PATTERN_PAUSE, PATTERN_MAX;
	// whether animal left sector after pattern was initiated and completed fully
	bool PATTERN_RESET = true;
	// number of frames observed within ROI - unknown pos in-between allowed but not out of ROI
	int frames_in, PATTERN_MIN_IN;
	bool PATTERN_MODE;
	int pattern_mode;

	std::string ssham, ssham2;
	fparams >> cx >> cy >> ss >> se >> SHAPE;
	std::getline(fparams, ssham);
	fparams >> PATTERN_PULSE >> PATTERN_PAUSE >> PATTERN_MAX >> PATTERN_MIN_IN;
	//std::getline(fparams, ssham2);
	fparams >> IMMO_THOLD_SQ >> IMMO_LIMIT;
	fparams >> pattern_mode;
	PATTERN_MODE = bool(pattern_mode);
	RAD = ss;
	switch(SHAPE){
			case 0:{
				cout << "SECTOR: center: " << cx << ", " << cy << "; sector start, end: " << ss << " " << se << ", radius: " << RAD << ", shape:" << SHAPE << "\n";
				break;
			}

			case 1:{
				cout << "CIRCLE: center: " << cx << ", " << cy << "; radius: " << RAD << "\n";
				break;
			}

			case 2:{
				cout << "RECTANGLE: min x: " << cx << ", min y" << cy << "max x: " << ss << "max y" << se << "\n";
				break;
			}
	}
	cout << "Pattern pulse, ms: " << PATTERN_PULSE << ", patter pause, ms: " << PATTERN_PAUSE << ", pattern duration, ms:" << PATTERN_MAX << ", pattern minimal frames insisde: " << PATTERN_MIN_IN << "\n";
	cout << "Immobility squared speed thold / limit: " << IMMO_THOLD_SQ << "/" << IMMO_LIMIT << "\n";

	int mem_size=sizeof(positrack_shared_memory);
	positrack_shared_memory* psm;
	int des_num;
	unsigned long int frame_id = 0;
	unsigned long int frame_no = 0;
	double x, y, hd;

	bool DEBUG = argc > 3;

	ifstream debug_track;
	if (DEBUG){
		debug_track.open(argv[3]);
		cout << "USE DEBUG CORRDS INPUT\n";
	} else {
			cout << "shm_open\n";
			des_num=shm_open(POSITRACKSHARE, O_RDWR ,0600);
			if(des_num ==-1)
			{
				cerr << "problem with shm_open\n";
				cerr << "des_num: " << des_num << '\n';
				cerr << "Make sure positrack is running\n";
				return -1;
			}
			if (ftruncate(des_num, mem_size) == -1)
			{
				cerr << "Error with ftruncate\n";
				return -1;
			}
			cout << "nmap\n";
			psm = (positrack_shared_memory*) mmap(0, mem_size, PROT_READ | PROT_WRITE, MAP_SHARED, des_num, 0);
			if (psm == MAP_FAILED)
			{
				cerr << "Error with mmap\n";
				return -1;
			}
	}

	// positions queue
	queue<float> xq;
	queue<float> yq;
	int last_mobility_time = -1000;
	long long  last_validpos_time = -1000;
	int last_frame_no = -1000;

	const int SLEEP_TIME = 200;
	const double INVALID_OFF_DELAY = 2000; // msec
	const int QSIZE = 10;

	int immo_counter = 0;
	// const int IMMO_LIMIT = 100; // 2s @ 50 Hz
	//const double IMMO_THOLD_SQ = 8.0 * 8.0;

	time_t last_frame_time = time(0);

	// <0 IF NOR ACTIVE
	long long pattern_start = -1;

	//
	///// Do whatever you want here !
	/////
	//// read new frames from memory
	ofstream timestamps(argv[2]);
	bool ttl_on = false;
	bool ttl_on_new = false;
	bool frame1 = true;

	std::cout << std::fixed;

	while(true)
	{
		// read from the share memory
		unsigned int memidx = 0;
		long long time_msec;

		if (DEBUG){
			int sham;
			debug_track >> x >> y >> sham >> sham >> sham;
			std::getline(debug_track, ssham);
			time_msec = sham / 24;
			frame_no += 1;
			
		} else {
			if (frame1)
				cout << "get frame info\n";

			pthread_mutex_lock(&psm->pmutex);
			frame_id=psm->id[memidx];
			frame_no=psm->frame_no[memidx];
			ts_now=psm->ts[memidx];
			x=psm->x[memidx];
			y=psm->y[memidx];
			hd=psm->hd[memidx];
			pthread_mutex_unlock(&psm->pmutex);
			//time_msec = ts_now.tv_sec * 1000 + int(ts_now.tv_nsec/1000.0);
			time_msec = int(ts_now.tv_nsec/1000000.0);
		}

		unsigned char pstate = 0; // TODO OUT OF LOOP ?
		if (!DEBUG){
			if (frame1)
				cout << "ioperm + inb\n";

			int ret = ioperm(BASE, 1, 1);
			pstate = DEBUG ? 0 : inb(BASE);
		}

		// no new frames, keep sleeping
		if (frame_no == last_frame_no){
			// turn off if valid frame was long ago
			time_t now = time(0);
			if ((now - last_frame_time > 2) && ttl_on){
				cout << "POSITRACK SEEMS TO HAVE STOPPED, SET TTL OFF\n";
				if (DEBUG) {cout << "TTL_OFF\n"; timestamps << time_msec << "\n";} else ttl_off(timestamps, pstate, ttl_on, time_msec);
				//ttl_off(timestamps, pstate, ttl_on, time_msec);
			}

			usleep(SLEEP_TIME);
			continue;
		}
		last_frame_no = frame_no;
		//cout << "frame_id: " << frame_id << " frame_no: " << frame_no << " x: " << x << " y: " << y << " hd: " << hd << ", time:" << ts_now.tv_sec + ts_now.tv_nsec/1000000.0 << " microsec" << '\n';
		cout << "frame_id: " << frame_id << " frame_no: " << frame_no << " x: " << x << " y: " << y << " hd: " << hd << ", time:" << time_msec<< " ms; " << ts_now.tv_sec << " tv_sec, " << ts_now.tv_nsec << " tv_nsec\n";

		ttl_on_new = ttl_on;
		last_frame_time = time(0);	

		// immobility check
		if (x > 0){
			xq.push(x);
			yq.push(y);
			if (xq.size() > QSIZE){
				xq.pop();
				yq.pop();
			}

			if (pow(xq.back() - xq.front(), 2) + pow(yq.back() - yq.front(), 2) < IMMO_THOLD_SQ){
				immo_counter ++;
			} else {
				immo_counter = 0;
			}
		}

		// trigger logic here

		if (x > 0 && y > 0){
			//if (in_sector(cx, cy, ss, se, x, y) && !ttl_on && (immo_counter == 0)){
			if (in_sector(cx, cy, ss, se, x, y, SHAPE, RAD)) {
				frames_in ++;
				if (PATTERN_MODE){
						if((frames_in > PATTERN_MIN_IN )&& (pattern_start < 0) && (immo_counter == 0) && PATTERN_RESET){
								//timestamps << time_msec << " ";
								//pstate = pstate | 16;
								//outb(pstate, BASE);
								//ttl_on_new = true;
								pattern_start = time_msec;
								PATTERN_RESET = false;

								// RUN THE WHOLE PATTERN!
								cout << "RUNNING TRIGGER PATTERN\n";
								long long t = time_msec;
								while (t < time_msec + PATTERN_MAX){
									pstate = pstate | 16;
									cout << "TTL ON\n";
									timestamps << t << " ";
									timestamps.flush();
									if (!DEBUG) outb(pstate, BASE);
									usleep(PATTERN_PULSE * 1000);
									if(DEBUG) {timestamps << t + PATTERN_PULSE << "\n";} else ttl_off(timestamps, pstate, ttl_on_new, time_msec);
									usleep(PATTERN_PAUSE * 1000);
									cout << "TTL OFF\n";
									t += PATTERN_PAUSE + PATTERN_PULSE;
								}
								pattern_start = -1;
						}
				} else if (!ttl_on && (immo_counter == 0)) {
					pstate = pstate | 16;
					cout << "TTL ON\n";
					timestamps << time_msec << " ";
					timestamps.flush();
					if (!DEBUG) outb(pstate, BASE);
					//if(DEBUG) {timestamps << t + PATTERN_PULSE << "\n";} else ttl_off(timestamps, pstate, ttl_on_new, time_msec);
					ttl_on_new = true;
				}
			} else if (!in_sector(cx, cy, ss, se, x, y, SHAPE, RAD)){
				frames_in = 0;
				if (PATTERN_MODE){
					if(pattern_start < 0){
						PATTERN_RESET = true;
					}
				} else if (ttl_on) {
					if(DEBUG) {timestamps << time_msec << "\n";} else ttl_off(timestamps, pstate, ttl_on_new, time_msec);
                    cout << "TTL OFF\n";
					ttl_on_new = false;
				}
			}

			// NEW LOGIC - EXIT IS IRRELEVANT
//			if ((!in_sector(cx, cy, ss, se, x, y) || immo_counter > IMMO_LIMIT) && ttl_on){
//				 ttl_off(timestamps, pstate, ttl_on_new, time_msec);
//			}
			last_validpos_time = time_msec;
		// turn off if invalid pos for too long
		}
		
		// NEW LOGIC - NO STOP BY X/Y
//		 else if (time_msec - last_validpos_time > INVALID_OFF_DELAY && ttl_on){
//			ttl_off(timestamps, pstate, ttl_on_new, frame_no, time_msec);
//			cout << "Invalid pos STOP\n";
//		}

		// IS PATTERN RUNNING ? - this must not execute since whole pattern runs in pattern mode once triggered
		if (PATTERN_MODE && pattern_start > 0){
			// pettern end?
			if (time_msec - pattern_start > PATTERN_MAX) {
				pattern_start = -1;
				cout << "PATTERN OVER AT " << time_msec << "\n";
				if (ttl_on){
					if (DEBUG) {cout << "TTL OFF (PATTERN_MAX)\n"; ttl_on_new = false; timestamps << time_msec << "\n";} else ttl_off(timestamps, pstate, ttl_on_new, time_msec);
				}
				//ttl_off(timestamps, pstate, ttl_on_new, time_msec);
			} 
			else {
				if ((!ttl_on) && pattern_on(pattern_start, time_msec, PATTERN_PULSE, PATTERN_PAUSE)){
					timestamps << time_msec << " ";
					timestamps.flush();
		  			pstate = pstate | 16;
		     		cout << "TTL ON\n"; 
					if (!DEBUG) outb(pstate, BASE);
		     		//outb(pstate, BASE);
					ttl_on_new = true;
				}
				else if (ttl_on && !pattern_on(pattern_start, time_msec, PATTERN_PULSE, PATTERN_PAUSE)){
					cout << "TTL OFF (PATTERN)\n";
					if(DEBUG) { ttl_on_new = false; timestamps << time_msec << "\n";} else ttl_off(timestamps, pstate, ttl_on_new, time_msec);
					//ttl_off(timestamps, pstate, ttl_on_new, time_msec);
				}
			}
		}
		
		// check whether has been immobile for 2s
		ttl_on = ttl_on_new;
		//sleep(1);
		usleep(SLEEP_TIME);
		frame1 = false;
	}
	// unmap the shared memory
	if(munmap(psm, mem_size) == -1)
	{
		cerr << "problem with munmap\n";
		return -1;
	}
	return 0;
}
