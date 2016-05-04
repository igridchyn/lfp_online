/*
 * SwReaderProcessor.cpp
 *
 *  Created on: Jun 30, 2014
 *      Author: igor
 */

#include "SwReaderProcessor.h"
#include "Utils.h"
#include <fstream>

SwReaderProcessor::SwReaderProcessor(LFPBuffer *buf)
:SwReaderProcessor(buf,
		buf->config_->getOutPath("swr.reader.path")
		){}

SwReaderProcessor::SwReaderProcessor(LFPBuffer *buf, std::string path)
	: LFPProcessor(buf)
	, start_to_peak_((unsigned int)(buf->config_->getFloat("swr.reader.start.to.peak.ms") * buf->SAMPLING_RATE / 1000.0))
	, peak_to_end_((unsigned int)(buf->config_->getFloat("swr.reader.peak.to.end.ms") * buf->SAMPLING_RATE / 1000.0)){

	Utils::FS::CheckFileExistsWithError(path, buf);
	std::ifstream swr_stream(path);

	std::string swr_format_name_ = buffer->config_->getString("swr.reader.format");
	if (swr_format_name_ == "start_peak_end"){
		swr_format_ = SWR_FORMAT_START_PEAK_END;
	} else if (swr_format_name_ == "synchrony_start"){
		swr_format_ = SWR_FORMAT_SYNCHRONY_START;
	} else {
		Log("ERROR: Unrecognised SWR format, accepted values are : start_peak_end, synchrony_start");
		exit(12432);
	}

	while(!swr_stream.eof()){
		swrs_.push_back(std::vector<unsigned int>());

		switch(swr_format_){
		case SWR_FORMAT_START_PEAK_END:
			// read times of SWR beginning, peak and end
			int b,p,e;
			swr_stream >> b >> p >> e;
			swrs_[swrs_.size() - 1].push_back(b);
			swrs_[swrs_.size() - 1].push_back(p);
			swrs_[swrs_.size() - 1].push_back(e);
			break;

		case SWR_FORMAT_SYNCHRONY_START:
			unsigned int ss;
			swr_stream >> ss;
			swrs_[swrs_.size() - 1].push_back(ss > start_to_peak_ ? ss - start_to_peak_ : 0);
			swrs_[swrs_.size() - 1].push_back(ss);
			swrs_[swrs_.size() - 1].push_back(ss + peak_to_end_);
			break;
		}
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

	if (buffer->last_pkg_id > swrs_[current_sw_][0]){

		buffer->swrs_.push_back(swrs_[current_sw_]);

		if (current_sw_ < swrs_.size()){
			current_sw_ ++;
		}
	}
}
