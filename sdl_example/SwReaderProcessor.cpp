/*
 * SwReaderProcessor.cpp
 *
 *  Created on: Jun 30, 2014
 *      Author: igor
 */

#include "SwReaderProcessor.h"
#include <fstream>

SwReaderProcessor::SwReaderProcessor(LFPBuffer *buf)
:SwReaderProcessor(buf,
		buf->config_->getString("dat.path.base") + buf->config_->getString("swreader.ext")
		){}

SwReaderProcessor::SwReaderProcessor(LFPBuffer *buf, std::string path)
	: LFPProcessor(buf){

	std::ifstream swr_stream(path);

	while(!swr_stream.eof()){
		swrs_.push_back(std::vector<unsigned int>());

		// read times of SWR beginning, peak and end
		int b,p,e;
		swr_stream >> b >> p >> e;
		swrs_[swrs_.size() - 1].push_back(b);
		swrs_[swrs_.size() - 1].push_back(p);
		swrs_[swrs_.size() - 1].push_back(e);
	}

	buffer->log_string_stream_ << "Read " << swrs_.size() << " SWRs.\n";
	buffer->Log();
	swr_stream.close();
}

SwReaderProcessor::~SwReaderProcessor() {
}

void SwReaderProcessor::process() {
	if (swrs_.size() == current_sw_)
		return;

	// TODO assign end later ?
	if (buffer->last_pkg_id > swrs_[current_sw_][0]){

		buffer->swrs_.push_back(swrs_[current_sw_]);

		if (current_sw_ < swrs_.size()){
			current_sw_ ++;
		}
	}
}
