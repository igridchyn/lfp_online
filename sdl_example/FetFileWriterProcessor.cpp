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
	std::string path_base = buf->config_->getOutPath("spike.writer.path.base");
	write_spk_ = buf->config_->getBool("spike.writer.spk.write");
	binary_ = buf->config_->getBool("spike.writer.binary", false);

	std::string extapp = binary_ ? "b" : "";

	std::string spk_path_base = path_base + "spk" + extapp + ".";

	for (int t=0; t < buf->tetr_info_->tetrodes_number(); ++t){
		fet_files_.push_back(new std::ofstream(path_base + "fet" + extapp + "." + Utils::NUMBERS[t], binary_ ? std::ofstream::binary : std::ofstream::out));
		if (write_spk_){
			spk_files_.push_back(new std::ofstream(spk_path_base+ Utils::NUMBERS[t], binary_ ? std::ofstream::binary : std::ofstream::out));
		}
		if (!binary_){
			// TODO !!! parametrize - number of entries
			*(fet_files_[t]) << "17\n";
		}
	}

	whl_file_ = new std::ofstream(path_base + "whl");
}

FetFileWriterProcessor::~FetFileWriterProcessor() {
	for (unsigned int i=0; i < fet_files_.size(); ++i){
		fet_files_[i]->close();

		if (write_spk_){
			spk_files_[i]->close();
		}
	}

	whl_file_->close();
}

void FetFileWriterProcessor::process() {
	while(buffer->spike_buf_pos_fet_writer_ < buffer->spike_buf_pos_unproc_){

		Spike *spike = buffer->spike_buffer_[buffer->spike_buf_pos_fet_writer_];

		if (spike == nullptr || spike->discarded_){
			buffer->spike_buf_pos_fet_writer_++;
			continue;
		}

		if (spike->pc == NULL){
			return;
		}

		buffer->spike_buf_pos_fet_writer_++;

		const int& tetrode = spike->tetrode_;
		std::ofstream& fet_file = *(fet_files_[tetrode]);

		if (spike->discarded_){
			continue;
		}

		// TODO !!!! write the same value
		for(int f=0; f < 16; ++f){
			if (binary_)
				fet_file.write((char*)spike->getFeatureAddr(f), sizeof(float));
			else
				fet_file << (int)round(spike->getFeature(f)) << " ";
		}

		if (binary_)
			fet_file.write((char*)&spike->pkg_id_, sizeof(int));
		else
			fet_file << spike->pkg_id_ << "\n";

		if (write_spk_){
			std::ofstream& spk_file = *(spk_files_[tetrode]);

			// TODO parametrize
			for (int c=0; c < spike->num_channels_; ++c){
				if (binary_)
					spk_file.write((char*)spike->waveshape[c], 128 * sizeof(int));
				else{
					for (int w=0; w < 128; ++w ){
						spk_file << spike->waveshape[c][w] << " ";
					}
				}
			}

			if (!binary_)
				spk_file << "\n";
		}
	}

	if (!whl_start_written_ && buffer->pos_first_pkg_ >= 0){
		(*whl_file_) << buffer->pos_first_pkg_ << "\n";
		whl_start_written_ = true;
	}

	// write fet
	while(buffer->pos_buf_pos_whl_writer_ < buffer->pos_buf_pos_){
		SpatialInfo &pos_rec = buffer->positions_buf_[buffer->pos_buf_pos_whl_writer_];
		(*whl_file_) << pos_rec.x_small_LED_ << " " << pos_rec.y_small_LED_ << " " << pos_rec.x_big_LED_ << " " << pos_rec.y_big_LED_ << " " << pos_rec.pkg_id_ << "\n";
		buffer->pos_buf_pos_whl_writer_++;
	}
}
