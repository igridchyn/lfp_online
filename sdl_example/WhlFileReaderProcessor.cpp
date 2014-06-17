/*
 * WhlFileReaderProcessor.cpp
 *
 *  Created on: Jun 12, 2014
 *      Author: igor
 */

#include "WhlFileReaderProcessor.h"

WhlFileReaderProcessor::WhlFileReaderProcessor(LFPBuffer *buffer, const std::string& whl_path, const unsigned int& sampling_rate)
: LFPProcessor(buffer)
, whl_path_(whl_path)
, whl_stream_(whl_path)
, sampling_rate_(sampling_rate)
{

}

WhlFileReaderProcessor::~WhlFileReaderProcessor() {
	// TODO Auto-generated destructor stub
}

void WhlFileReaderProcessor::process(){
	while(last_pos_pkg_id_ < buffer->last_pkg_id + sampling_rate_){
		float x,y;
		whl_stream_ >> x >> y;

		last_pos_pkg_id_ += sampling_rate_;

		// TODO: rewind (also for spike buffer in fet reader)
		unsigned int *pos_rec = buffer->positions_buf_[buffer->pos_buf_pos_];

		if (x < 0){
			pos_rec[0] = 1023;
			pos_rec[1] = 1023;
//			pos_rec[2] = 0;
//			pos_rec[3] = 0;
		}
		else{
			pos_rec[0] = x;
			pos_rec[1] = y;
//			pos_rec[2] = 0;
//			pos_rec[3] = 0;
		}

		pos_rec[4] = last_pos_pkg_id_;
		buffer->pos_buf_pos_ ++;
	}
}
