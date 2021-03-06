/*
 * FetFileWriterProcessor.cpp
 *
 *  Created on: Oct 18, 2014
 *      Author: igor
 */

#include "FetFileWriterProcessor.h"

FetFileWriterProcessor::FetFileWriterProcessor(LFPBuffer *buf)
: LFPProcessor(buf)
, spike_buf_ptr_limit_(buf->GetSpikeBufPointer(buf->config_->getString("spike.writer.limit")))
{
	std::string path_base = buf->config_->getOutPath("spike.writer.path.base");
	write_spk_ = buf->config_->getBool("spike.writer.spk.write");
	binary_ = buf->config_->getBool("spike.writer.binary", false);

	std::string extapp = binary_ ? "b" : "";

	std::string spk_path_base = path_base + "spk" + extapp + ".";

	for (size_t t=0; t < buf->tetr_info_->tetrodes_number(); ++t){
		std::string fetpath = path_base + "fet" + extapp + "." + Utils::NUMBERS[t];
		if (Utils::FS::FileExists(fetpath)){
			Log(std::string("ERROR: File exists at the requested output file, delete files and restart the progam:") + fetpath);
			exit(81275);
		}

		fet_files_.push_back(new std::ofstream(fetpath, binary_ ? std::ofstream::binary : std::ofstream::out));
		if (write_spk_){
			std::string spkpath = spk_path_base+ Utils::NUMBERS[t];
			if (Utils::FS::FileExists(spkpath)){
				Log(std::string("ERROR: File exists at the requested output file, delete files and restart the progam:") + spkpath);
				exit(81275);
			}
			spk_files_.push_back(new std::ofstream(spkpath, binary_ ? std::ofstream::binary : std::ofstream::out));
		}
		if (!binary_){
			const unsigned int n_entries = buf->feature_space_dims_[t] + 5;
			*(fet_files_[t]) << n_entries << "\n";
		}
	}

	std::string whlpath = path_base + "whl";
	if (Utils::FS::FileExists(whlpath)){
		Log(std::string("ERROR: File exists at the requested output file, delete files and restart the progam:") + whlpath);
		exit(81275);
	}
	whl_file_ = new std::ofstream(whlpath);
}

FetFileWriterProcessor::~FetFileWriterProcessor() {
	for (unsigned int i=0; i < fet_files_.size(); ++i){
		fet_files_[i]->flush();
		fet_files_[i]->close();

		if (write_spk_){
			spk_files_[i]->flush();
			spk_files_[i]->close();
		}
	}

	whl_file_->flush();
	whl_file_->close();
}

void FetFileWriterProcessor::process() {
	// last condition - in case PCA min was not reached but rather caluclated after data was over, now spikes have to be written
	if (buffer->pipeline_status_ == PIPELINE_STATUS_INPUT_OVER && !streams_flushed_after_input_over_ && spike_buf_ptr_limit_ > 0 && buffer->spike_buf_pos_fet_writer_ == spike_buf_ptr_limit_){
		for (unsigned int i=0; i < fet_files_.size(); ++i){
			fet_files_[i]->flush();

			if (write_spk_){
				spk_files_[i]->flush();
			}
		}

		whl_file_->flush();

		streams_flushed_after_input_over_ = true;

		Log("Flushed streams after input over at spike pos ", buffer->spike_buf_pos_fet_writer_);

		buffer->processing_over_ = true;
	}

	// write whl
	while(buffer->pos_buf_pos_whl_writer_ < buffer->pos_buf_pos_){
		SpatialInfo &pos_rec = buffer->positions_buf_[buffer->pos_buf_pos_whl_writer_];
		float xs = pos_rec.x_small_LED_;
		float ys = pos_rec.y_small_LED_;
		float xb = pos_rec.x_big_LED_;
		float yb = pos_rec.y_big_LED_;
		if (Utils::Math::Isnan(xs)){
			xs = (float)buffer->pos_unknown_;
			ys = (float)buffer->pos_unknown_;
		}
		if (Utils::Math::Isnan(xb)){
			xb = (float)buffer->pos_unknown_;
			yb = (float)buffer->pos_unknown_;
		}
		(*whl_file_) << xs << " " << ys << " " << xb << " " << yb << " " << pos_rec.pkg_id_ << " " << pos_rec.valid << "\n";
		buffer->pos_buf_pos_whl_writer_++;
	}

	while(buffer->spike_buf_pos_fet_writer_ < spike_buf_ptr_limit_){

		Spike *spike = buffer->spike_buffer_[buffer->spike_buf_pos_fet_writer_];

		if (spike->discarded_){
			buffer->spike_buf_pos_fet_writer_++;
			continue;
		}

		if (spike->pc == nullptr){
			return;
		}

		buffer->spike_buf_pos_fet_writer_++;

		const int& tetrode = spike->tetrode_;
		std::ofstream& fet_file = *(fet_files_[tetrode]);

		const unsigned int fetn = spike->num_channels_ * spike->num_pc_;

		// TODO customize extra-features
		for(unsigned int f=0; f < fetn + 4; ++f){
			if (binary_)
				fet_file.write((char*)spike->getFeatureAddr(f), sizeof(float));
			else
				fet_file << spike->getFeature(f) << " ";
		}

		if (binary_)
			fet_file.write((char*)&spike->pkg_id_, sizeof(unsigned int));
		else
			fet_file << spike->pkg_id_ << "\n";

		if (write_spk_){
			std::ofstream& spk_file = *(spk_files_[tetrode]);

			for (int c=0; c < spike->num_channels_; ++c){
				if (binary_)
					spk_file.write((char*)spike->waveshape[c], 128 * sizeof(ws_type));
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
}
