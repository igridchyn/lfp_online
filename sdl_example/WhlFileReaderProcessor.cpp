/*
 * WhlFileReaderProcessor.cpp
 *
 *  Created on: Jun 12, 2014
 *      Author: igor
 */

#include "WhlFileReaderProcessor.h"

WhlFileReaderProcessor::WhlFileReaderProcessor(LFPBuffer* buffer)
: WhlFileReaderProcessor(buffer,
		buffer->config_->getString("dat.path.base") + buffer->config_->getString("whl.extension", "whl"),
		buffer->config_->getFloat("whl.sampling.factor"),
		buffer->config_->getFloat("whl.sub.x"),
		buffer->config_->getFloat("whl.sub.y")
) {
}

WhlFileReaderProcessor::WhlFileReaderProcessor(LFPBuffer *buffer, const std::string& whl_path, const unsigned int& sampling_rate,
		const float sub_x, const float sub_y)
: LFPProcessor(buffer)
, whl_path_(whl_path)
, whl_stream_(whl_path)
, sampling_rate_(sampling_rate)
, SUB_X(sub_x)
, SUB_Y(sub_y)
{

}

WhlFileReaderProcessor::~WhlFileReaderProcessor() {
	whl_stream_.close();
}

void WhlFileReaderProcessor::process(){
	while(last_pos_pkg_id_ < buffer->last_pkg_id + sampling_rate_){
		float x,y;
		whl_stream_ >> x >> y;

		last_pos_pkg_id_ += sampling_rate_;

		// TODO: rewind (also for spike buffer in fet reader)
		SpatialInfo &pos_rec = buffer->positions_buf_[buffer->pos_buf_pos_];

		// otherwise remains unknown (valid = false) with coords set to -1
		if (!(x < 0 || x <= SUB_X || y <= SUB_Y)){
			pos_rec.x_small_LED_ = x - SUB_X;
			pos_rec.y_small_LED_ = y - SUB_Y;
			pos_rec.x_big_LED_ = x - SUB_X;
			pos_rec.y_big_LED_ = y - SUB_Y;
		}

		pos_rec.pkg_id_ = last_pos_pkg_id_;
		buffer->AdvancePositionBufferPointer();
	}
}


