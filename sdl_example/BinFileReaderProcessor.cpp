/*
 * BinFileReaderProcessor.cpp
 *
 *  Created on: Feb 10, 2015
 *      Author: igor
 */

#include "BinFileReaderProcessor.h"

BinFileReaderProcessor::BinFileReaderProcessor(LFPBuffer* buf)
	: LFPProcessor(buf)
	, file_path_(buf->config_->getString("bin.path", ""))
	, chunk_size_(getInt("chunk.size"))
	, nblock_(buf->config_->getInt("bin.nblock", 1)){

	if (file_path_.length() > 0){
		if (!Utils::FS::FileExists(file_path_)){
			std::cout << "File doesn't exist: " << file_path_ << "\n";
			exit(1);
		}
		bin_file_ = fopen(file_path_.c_str(), "rb");
		buffer->pipeline_status_ = PIPELINE_STATUS_READ_BIN;
	}
	else{
		Log("No bin file path provided in the config, exiting");
		exit(1);
	}

	block_ = new unsigned char[chunk_size_ * nblock_];

	buf->pipeline_status_ == PIPELINE_STATUS_READ_BIN;
	buf->input_duration_ = boost::filesystem::file_size(file_path_) * 3 / chunk_size_;
	std::cout << "Input file duration: " << buf->input_duration_ / (buf->SAMPLING_RATE * 60) << " min " <<  buf->input_duration_ / (buf->SAMPLING_RATE) % 60 << "sec @ " << buf->SAMPLING_RATE << " samples / sec" << "\n";
}

void BinFileReaderProcessor::process() {
	if (!feof(bin_file_)) {
		fread((void*)block_, 1, chunk_size_ * nblock_, bin_file_);

		buffer->chunk_ptr = block_;
		buffer->num_chunks = nblock_;
	}
	else if (!end_bin_file_reported_){
		end_bin_file_reported_ = true;
		buffer->pipeline_status_ = PIPELINE_STATUS_INPUT_OVER;
		Log("Out of data packages. Press ESC to exit...\n");
	}
}

BinFileReaderProcessor::~BinFileReaderProcessor() {
	// TODO Auto-generated destructor stub
	fclose(bin_file_);
	delete[] block_;
}

