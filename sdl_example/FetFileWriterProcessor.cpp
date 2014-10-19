/*
 * FetFileWriterProcessor.cpp
 *
 *  Created on: Oct 18, 2014
 *      Author: igor
 */

#include "FetFileWriterProcessor.h"

FetFileWriterProcessor::FetFileWriterProcessor(LFPBuffer *buf)
:LFPProcessor(buf)
{
	// TODO Auto-generated constructor stub
	std::string path_base = buf->config_->getString("spike.writer.path.base");
	std::string spk_path_base = path_base + "spk.";
	write_spk_ = buf->config_->getBool("spike.writer.spk.write");

	for (int t=0; t < buf->tetr_info_->tetrodes_number; ++t){
		fet_files_.push_back(new std::ofstream(path_base + "fet." + Utils::NUMBERS[t]));
		if (write_spk_){
			spk_files_.push_back(new std::ofstream(spk_path_base+ Utils::NUMBERS[t]));
		}
	}

	whl_file_ = new std::ofstream(path_base + "whl");
}

FetFileWriterProcessor::~FetFileWriterProcessor() {
	// TODO Auto-generated destructor stub
	for (int i=0; i < fet_files_.size(); ++i){
		fet_files_[i]->close();

		if (write_spk_){
			spk_files_[i]->close();
		}
	}
}

void FetFileWriterProcessor::process() {
	while(buffer->spike_buf_pos_fet_writer_ < buffer->spike_buf_pos_unproc_){
		Spike *spike = buffer->spike_buffer_[buffer->spike_buf_pos_fet_writer_++];
		if (spike == nullptr || spike->discarded_)
			continue;

		const int& tetrode = spike->tetrode_;
		std::ofstream& fet_file = *(fet_files_[tetrode]);

		if (spike->discarded_){
			continue;
		}

		for(int f=0; f < 16; ++f){
			fet_file << (int)round(spike->getFeature(f, 3)) << " ";
		}
		fet_file << spike->pkg_id_ << "\n";

		if (write_spk_){
			std::ofstream& spk_file = *(spk_files_[tetrode]);

			// TODO parametrize
			for (int c=0; c < spike->num_channels_; ++c){
				for (int w=0; w < 128; ++w ){
					spk_file << spike->waveshape[c][w] << " ";
				}
			}

			spk_file << "\n";
		}
	}

	if (!whl_start_written_ && buffer->pos_first_pkg_ >= 0){
		(*whl_file_) << buffer->pos_first_pkg_ << "\n";
		whl_start_written_ = true;
	}

	// write fet
	while(buffer->pos_buf_pos_whl_writer_ < buffer->pos_buf_pos_){
		unsigned int *pos_rec = buffer->positions_buf_[buffer->pos_buf_pos_whl_writer_];
		(*whl_file_) << pos_rec[0] << " " << pos_rec[1] << " " << pos_rec[2] << " " << pos_rec[3] << " " << pos_rec[4] << "\n";
		buffer->pos_buf_pos_whl_writer_++;
	}
}
