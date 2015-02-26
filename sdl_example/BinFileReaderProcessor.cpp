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
	, nblock_(buf->config_->getInt("bin.nblock", 1))
	, x_shift_upon_file_change_(buf->config_->getFloat("bin.x.shift"))
	, y_shift_upon_file_change_(buf->config_->getFloat("bin.y.shift"))
	, SHIFT_ODD(getBool("bin.shift.odd")){

	std::stringstream ss;
	ss << "Shift of " << x_shift_upon_file_change_ << " / " << y_shift_upon_file_change_ <<
			" will be set for each " << (SHIFT_ODD ? "ODD" : "EVEN") << " file (starting from file #0).";
	Log(ss.str());

	// if shift even files - start with the shifted ones
	if (!SHIFT_ODD){
		buffer->coord_shift_x_ = x_shift_upon_file_change_;
		buffer->coord_shift_y_ = y_shift_upon_file_change_;
	}

	if (file_path_.length() > 0){
		if (!Utils::FS::FileExists(file_path_)){
			buffer->Log("File doesn't exist or is unavailable: " + file_path_);
			exit(LFPONLINE_ERROR_BIN_FILE_DOESNT_EXIST_OR_UNAVAILABLE);
		}
		bin_file_ = fopen(file_path_.c_str(), "rb");
		Log(std::string("1-st file duration: ") + axonaFileDuration(file_path_));
	}
	else{
		Log("No bin file path provided in the config, exiting");
		exit(LFPONLINE_ERROR_NO_BIN_FILE_PROVIDED);
	}

	// read additional files
	files_list_.push_back(file_path_);
	unsigned int nfiles = 1;

	while(true){
		file_path_ = buf->config_->getString(std::string("bin.path.") + Utils::NUMBERS[nfiles + 1], "");
		if (file_path_.size() == 0){
			file_path_ = files_list_[0];
			break;
		}

		if (!Utils::FS::FileExists(file_path_)){
			buffer->Log("File doesn't exist or is unavailable: " + file_path_);
			exit(LFPONLINE_ERROR_BIN_FILE_DOESNT_EXIST_OR_UNAVAILABLE);
		}

		nfiles ++;
		Log(std::string(Utils::NUMBERS[nfiles]) + "-th file duration: " + axonaFileDuration(file_path_));
		files_list_.push_back(file_path_);
	}

	Log(std::string(Utils::NUMBERS[nfiles]) + " bin files provided.");
	Log("Total files duration: " + axonaFilesDuration(files_list_));

	block_ = new unsigned char[chunk_size_ * nblock_];

	buf->pipeline_status_ = PIPELINE_STATUS_READ_BIN;
	buf->input_duration_ = totalAxonaPackages(files_list_);
}

void BinFileReaderProcessor::process() {
	if (!feof(bin_file_)) {
		fread((void*)block_, 1, chunk_size_ * nblock_, bin_file_);

		buffer->chunk_ptr = block_;
		buffer->num_chunks = nblock_;
	}
	else if (current_file_ < files_list_.size() - 1)
	{
		current_file_ ++;
		Log(std::string("File ") + file_path_ + " over, open next one:\n\t" + files_list_[current_file_]);
		file_path_ = files_list_[current_file_];
		fclose(bin_file_);
		bin_file_ = fopen(file_path_.c_str(), "rb");

		if ((!(current_file_ % 2)) ^ SHIFT_ODD){
			Log("Add shift in file number ", current_file_);
			buffer->coord_shift_x_ = x_shift_upon_file_change_;
			buffer->coord_shift_y_ = y_shift_upon_file_change_;
		}
		else{
			Log("No shift in file number ", current_file_);
			buffer->coord_shift_x_ = 0;
			buffer->coord_shift_y_ = 0;
		}
	}
	else if	(!end_bin_file_reported_){
		end_bin_file_reported_ = true;
		buffer->pipeline_status_ = PIPELINE_STATUS_INPUT_OVER;
		Log("Out of data packages. Press ESC to exit...\n");
		buffer->chunk_ptr = nullptr;
	}
}

std::string BinFileReaderProcessor::axonaFileDuration(std::string file_path) {
	unsigned int nsamples = boost::filesystem::file_size(file_path) * 3 / chunk_size_;
	return axonaFileDurationFromNSampes(nsamples);
}

std::string BinFileReaderProcessor::axonaFilesDuration(
		std::vector<std::string> file_list) {
	return axonaFileDurationFromNSampes(totalAxonaPackages(file_list));
}

std::string BinFileReaderProcessor::axonaFileDurationFromNSampes(
	const unsigned int& nsamples) {
	std::stringstream ss;
	ss << nsamples / (buffer->SAMPLING_RATE * 60) << " min " <<  nsamples / (buffer->SAMPLING_RATE) % 60 << " sec @ " << buffer->SAMPLING_RATE << " samples / sec";
	return ss.str();
}

unsigned int BinFileReaderProcessor::totalAxonaPackages(
		std::vector<std::string> file_list) {
	unsigned int total_duration = 0;
	for (unsigned int f = 0; f < file_list.size(); ++f) {
		total_duration += boost::filesystem::file_size(file_list[f]) * 3 / chunk_size_;
	}
	return total_duration;
}

BinFileReaderProcessor::~BinFileReaderProcessor() {
	fclose(bin_file_);
	delete[] block_;
}

