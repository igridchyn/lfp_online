//
//  SpikeAlignmentProcessor.cpp
//  sdl_example
//
//  Created by Igor Gridchyn on 17/04/14.
//  Copyright (c) 2014 Jozsef Csicsvari, Igor Gridchyn. All rights reserved.
//

#include "LFPProcessor.h"


void SpikeAlignmentProcessor::process(){
	process_tetrode(-1);
}

void SpikeAlignmentProcessor::process_tetrode(int tetrode_to_process){
    // try to populate unpopulated spikes -
	unsigned int& spike_buf_ptr = tetrode_to_process >=0 ? spike_buf_tetrodewise_ptrs_[tetrode_to_process] : buffer->spike_buf_nows_pos;

    while (spike_buf_ptr < buffer->spike_buf_pos &&
           buffer->spike_buffer_[spike_buf_ptr]->pkg_id_ < buffer->last_pkg_id - 25 - Spike::WL_LENGTH/2){
        Spike *spike = buffer->spike_buffer_[spike_buf_ptr];

        // parallel mode: skip non-target tetrodes
        if (tetrode_to_process >=0 && spike->tetrode_ != tetrode_to_process){
        	spike_buf_ptr ++;
        	continue;
        }

        // check if there are not too many spikes in the noise removal queue and signal noise if yes
        if (tetrode_to_process == -1){
			while (!noise_detection_queue_.empty()){
				Spike *front = noise_detection_queue_.front();
				if (front->pkg_id_ < spike->pkg_id_  - NOISE_WIN){
					noise_detection_queue_.pop();
				}else{
					break;
				}
			}
			noise_detection_queue_.push(spike);
			// TODO: keep pointer to last spike assigned as noise to repeat the assignment
			if (noise_detection_queue_.size() > NNOISE){
				last_noise_pkg_id_ = spike->pkg_id_;
				unsigned int noise_ptr = spike_buf_ptr;
				while (noise_ptr > 0 && buffer->spike_buffer_[noise_ptr]->pkg_id_ > spike->pkg_id_ - NOISE_WIN){
					buffer->spike_buffer_[noise_ptr]->discarded_ = true;
					noise_ptr--;
				}

				 spike_buf_ptr++;
				 spike->discarded_ = true;
				 continue;
			}
        }

		// DEBUG
		buffer->CheckPkgIdAndReportTime(spike->pkg_id_, "Time from after package extraction until arrival in SpikeAlign, # of spikes = " + Utils::Converter::int2str(buffer->spike_buf_pos) + "\n");

        int num_of_ch = buffer->tetr_info_->channels_number(spike->tetrode_);
        buffer->AllocateWaveshapeMemory(spike);
        spike->num_channels_ = num_of_ch;
        
        int tetrode = spike->tetrode_;
        
        for (int ch=0; ch < num_of_ch; ++ch){
            // for reconstructed ws
//            spike->waveshape[ch] = new int[128];
            memset(spike->waveshape[ch], 0, sizeof(ws_type)*128);
            
            // copy signal for the channel from buffer to spike object
            //memcpy(spike->waveshape[ch], buffer->signal_buf[buffer->tetr_info_->tetrode_channels[spike->tetrode_][ch]] + buffer->buf_pos - (buffer->last_pkg_id - spike->pkg_id_) - Spike::WL_LENGTH / 2, Spike::WL_LENGTH * sizeof(int));
            
            memcpy(spike->waveshape[ch], buffer->filtered_signal_buf[buffer->tetr_info_->tetrode_channels[spike->tetrode_][ch]] + buffer->buf_pos - (buffer->last_pkg_id - spike->pkg_id_) - Spike::WL_LENGTH / 2 - 1, Spike::WL_LENGTH * sizeof(ws_type));
            
            
            // DEBUG
            //            printf("Waveshape, %d, ch. %d: ", spike->pkg_id_, ch);
            //            for (int i=0; i<spike->WL_LENGTH;++i){
            //                printf("%d ", spike->waveshape[ch][i]);
            //            }
            //            printf("\n");
            
        }
        
        // find max value in the given range
        ws_type max_val = spike->waveshape[0][0];
        int peak_time=0;
        int max_chan = 0;
        
        // TODO: pointers
        for(int i=0;i<spike->num_channels_;i++){
            for(int j=1;j<Spike::WL_LENGTH-1;j++)
            {
                if (spike->waveshape[i][j] < max_val){
                    max_val=spike->waveshape[i][j];
                    peak_time=j;
                    max_chan=i;
                }
            }
        }
        
        // TODO: shift buffer or create one more for aligned spikes
        if ((peak_time==1) && (spike->waveshape[max_chan][0] < max_val)){
            spike_buf_ptr++;
            spike->discarded_ = true;
            continue; /* not a peak */ // - if peak in the beginning, but previous value is larger
        }
        
        if ((peak_time==(Spike::WL_LENGTH-2))&&(spike->waveshape[max_chan][Spike::WL_LENGTH-1] < max_val)){
            spike_buf_ptr++;
            spike->discarded_ = true;
            continue; /* not a peak */ // - if peak in the end, but the last value is larger
        }
        
        // new peak position
        int peak_pos = spike->pkg_id_ - Spike::WL_LENGTH/2 + peak_time;
        
        // if not within refractory period from previous spike
        
		if (peak_pos - prev_spike_pos_[tetrode] > REFRACTORY_PERIOD || prev_spike_[tetrode] == nullptr)
    	{
			// !!!  This is the EARLIEST point when the spike is accepted for further analysis
            if (prev_spike_[tetrode] != nullptr){
                prev_spike_[tetrode]->power_ = prev_max_val_[tetrode];

                // ADD spike to buffer's population window and use for ISI estimation
                // TODO !!!!!! synchronous !!!!!!
                buffer->UpdateWindowVector(prev_spike_[tetrode]);
            }
            
            prev_spike_pos_[tetrode] = peak_pos;
            prev_max_val_[tetrode] = max_val;
            prev_spike_[tetrode] = spike;
            // copy waveshape right away
            if (prev_spike_[tetrode] != nullptr){
            	int buf_start_shift = buffer->buf_pos - (buffer->last_pkg_id - prev_spike_pos_[tetrode])
                                        				- Spike::WS_LENGTH_ALIGNED/ 2 - 1;

            	for(int ch=0; ch < num_of_ch; ++ch){
            		memcpy(prev_spike_[tetrode]->waveshape[ch],
            				buffer->filtered_signal_buf[buffer->tetr_info_->tetrode_channels[prev_spike_[tetrode]->tetrode_][ch]]
            				                            + buf_start_shift,
            				                            Spike::WS_LENGTH_ALIGNED * sizeof(ws_type));

            		// DEBUG
            		int buf_start_shift = buffer->buf_pos - (buffer->last_pkg_id - prev_spike_pos_[tetrode])
            		                            				- Spike::WS_LENGTH_ALIGNED/ 2 - 1;
            		if(buf_start_shift < 0)
            			buffer->Log("WTF? Reading waveshape outside of filtered signal buffer...", spike_buf_ptr);
            	}
            }
    	}
        // ??? otherwise - replace with current, if larger  ??? - still spiking
        else{
            if (max_val < prev_max_val_[tetrode]){
                prev_spike_[tetrode]->discarded_ = true;
                
                prev_spike_pos_[tetrode] = peak_pos;
                prev_max_val_[tetrode] = max_val;
                prev_spike_[tetrode] = spike;
            }
            else{
                // current within refractory and with smaller amplitude -> discard
                spike->discarded_ = true;
            }
        }
        
		// DEBUG
		buffer->CheckPkgIdAndReportTime(spike->pkg_id_, "Time from after package extraction until start of temporal shift check in SpikeAlign\n");

        // make sure the temporal order of spikes didn't change (assuming it is correct for all previous spikes) by pushing current spikes down in buffer until more recent spike is found
		if (tetrode_to_process == -1){
			int spike_sort_pos = spike_buf_ptr;
			while (spike_sort_pos > 0 && buffer->spike_buffer_[spike_sort_pos - 1] != nullptr && spike->pkg_id_ < buffer->spike_buffer_[spike_sort_pos - 1]->pkg_id_) {
				// swap
				buffer->spike_buffer_[spike_sort_pos] = buffer->spike_buffer_[spike_sort_pos - 1];
				buffer->spike_buffer_[spike_sort_pos - 1] = spike;

				spike_sort_pos --;
			}
		}
        
        // DEBUG
		buffer->CheckPkgIdAndReportTime(spike->pkg_id_, "Time from after package extraction until end of temporal shift check in SpikeAlign\n");

		spike_buf_ptr++;
    }
}

SpikeAlignmentProcessor::SpikeAlignmentProcessor(LFPBuffer* buffer)
: LFPProcessor(buffer)
, REFRACTORY_PERIOD(buffer->config_->getInt("spike.detection.refractory", 16)){
    int tetr_num = buffer->tetr_info_->tetrodes_number();
    
    prev_spike_pos_ = new unsigned int[tetr_num];
    memset(prev_spike_pos_, 0, sizeof(unsigned int)*tetr_num);
    prev_max_val_ = new int[tetr_num];
    memset(prev_max_val_, 0, sizeof(int)*tetr_num);
    prev_spike_ = new Spike*[tetr_num];
    memset(prev_spike_, 0, sizeof(Spike*)*tetr_num);

	buffer->Log("SpikeAlignmentProcessor created");

//	noise_stream_.open("/tmp/ns");
}

void SpikeAlignmentProcessor::desync() {
	// TODO: make a default implementation after encapsulating master ptr into the LFPProcessor
	// default : set t-wise pointers to master pointer
	for (unsigned int t=0; t < buffer->tetr_info_->tetrodes_number(); ++t){
		spike_buf_tetrodewise_ptrs_[t] = buffer->spike_buf_nows_pos;
	}
}

void SpikeAlignmentProcessor::sync() {
	// choose max (as the limit is spike detection ptr which can differ in different tetrodes)
	unsigned int prev_ptr = buffer->spike_buf_nows_pos;

	for (unsigned int t=0; t < buffer->tetr_info_->tetrodes_number(); ++t){
		if (spike_buf_tetrodewise_ptrs_[t] > buffer->spike_buf_nows_pos)
			buffer->spike_buf_nows_pos = spike_buf_tetrodewise_ptrs_[t];
	}

	// check temporal order
	for (unsigned int spike_buf_ptr = prev_ptr; spike_buf_ptr < buffer->spike_buf_nows_pos; ++spike_buf_ptr){
		Spike *spike = buffer->spike_buffer_[spike_buf_ptr];

		// TODO !!! check validity for sync mode, MAKE PARALLEL VERSION ??? - or avoid
		// check if there are not too many spikes in the noise removal queue and signal noise if yes
		while (!noise_detection_queue_.empty()){
			Spike *front = noise_detection_queue_.front();
			if (front->pkg_id_ < spike->pkg_id_  - NOISE_WIN){
				noise_detection_queue_.pop();
			}else{
				break;
			}
		}
		noise_detection_queue_.push(spike);
		// TODO: keep pointer to last spike assigned as noise to repeat the assignment
		if (noise_detection_queue_.size() > NNOISE){
			last_noise_pkg_id_ = spike->pkg_id_;
			unsigned int noise_ptr = spike_buf_ptr;
			while (noise_ptr > 0 && buffer->spike_buffer_[noise_ptr]->pkg_id_ > spike->pkg_id_ - NOISE_WIN){
				buffer->spike_buffer_[noise_ptr]->discarded_ = true;
				noise_ptr--;
			}

			spike_buf_ptr++;
			spike->discarded_ = true;
			continue;
		}
//
		int spike_sort_pos = spike_buf_ptr;
		while (spike_sort_pos > 0 && buffer->spike_buffer_[spike_sort_pos - 1] != nullptr && spike->pkg_id_ < buffer->spike_buffer_[spike_sort_pos - 1]->pkg_id_) {
			// swap
			buffer->spike_buffer_[spike_sort_pos] = buffer->spike_buffer_[spike_sort_pos - 1];
			buffer->spike_buffer_[spike_sort_pos - 1] = spike;

			spike_sort_pos --;
		}
	}
}
