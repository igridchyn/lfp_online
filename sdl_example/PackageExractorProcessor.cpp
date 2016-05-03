//
//  PackageExractorProcessor.cpp
//  sdl_example
//
//  Created by Igor Gridchyn on 05/06/14.
//  Copyright (c) 2014 Igor Gridchyn. All rights reserved.
//

#include "PackageExtractorProcessor.h"
#include "OnlineEstimator.cpp"
#include "time.h"

PackageExractorProcessor::PackageExractorProcessor(LFPBuffer *buffer)
    : LFPProcessor(buffer)
	, SCALE(buffer->config_->getFloat("pack.extr.xyscale"))
	, exit_on_data_over_(buffer->config_->getBool("pack.extr.exit.on.over", false))
	, read_pos_(buffer->config_->getBool("pack.extr.read.pos", true))
	, mode128_(buffer->config_->getBool("pack.extr.128mode", false))
{
	Log("Created");
	report_rate_ = buffer->SAMPLING_RATE * 60;

	CH_MAP = new int[64]{8, 9, 10, 11, 12, 13, 14, 15, 24, 25, 26, 27, 28, 29, 30, 31, 40, 41, 42, 43, 44, 45, 46, 47, 56, 57, 58, 59, 60, 61, 62, 63, 0, 1, 2, 3, 4, 5, 6, 7, 16, 17, 18, 19, 20, 21, 22, 23, 32, 33, 34, 35, 36, 37, 38, 39, 48, 49, 50, 51, 52, 53, 54, 55};

	// OLD
	//CH_MAP_INV = new int[64]{ 32, 33, 34, 35, 36, 37, 38, 39, 0, 1, 2, 3, 4, 5, 6, 7, 40, 41, 42, 43, 44, 45, 46, 47, 8, 9, 10, 11, 12, 13, 14, 15, 48, 49, 50, 51, 52, 53, 54, 55, 16, 17, 18, 19, 20, 21, 22, 23, 56, 57, 58, 59, 60, 61, 62, 63, 24, 25, 26, 27, 28, 29, 30, 31 };
	//CH_MAP_INV = new int[64]{48,49,50,51,52,53,54,55,32,33,34,35,36,37,38,39,16,17,18,19,20,21,22,23,0,1,2,3,4,5,6,7,56,57,58,59,60,61,62,63,40,41,42,43,44,45,46,47,24,25,26,27,28,29,30,31,8,9,10,11,12,13,14,15};
	CH_MAP_INV = new int[64]{8,9,10,11,12,13,14,15,24,25,26,27,28,29,30,31,40,41,42,43,44,45,46,47,56,57,58,59,60,61,62,63,0,1,2,3,4,5,6,7,16,17,18,19,20,21,22,23,32,33,34,35,36,37,38,39,48,49,50,51,52,53,54,55};

	last_check_point_ = time(0);
}

void PackageExractorProcessor::process(){
	//Log("Extract package...");

//	if (skip_next_pkg_){
//		skip_next_pkg_ = false;
//		return;
//	}
//	skip_next_pkg_ = true;

    // IDLE processing, waiting for user input
    if (buffer->chunk_buf_ == nullptr){
//		buffer->Log("PEXTR: zero pointer, return");
    	if (!end_reported_){
    		Log("EOF reached!");
    		end_reported_ = true;
    	}
    	else{
    		// give chance to all subsequent processors to process...
    		if (exit_on_data_over_){
    			Log("Exit on data over...");
    			buffer->processing_over_ = true;
    		}
    	}
        return;
    }
    
    unsigned int num_chunks = buffer->chunk_buf_ptr_in_ / CHUNK_SIZE;
	total_chunks_ += num_chunks;

    // DEBUG - performance test
    // buffer->CheckPkgIdAndReportTime(buffer->last_pkg_id, buffer->last_pkg_id + num_chunks / 2 * 3, Utils::Converter::int2str(buffer->last_pkg_id) + " pkg is reached, start clock\n", true);

    // TMPDEBUG
//    if (num_chunks > 1){
//    	Log("NUMCH > 1: ", num_chunks);
//    }

    // see if buffer reinit is needed, rewind buffer
    if (buffer->buf_pos + 3 * num_chunks > buffer->LFP_BUF_LEN){
        for (unsigned int c=0; c < buffer->CHANNEL_NUM; ++c){

        	if (!buffer->is_valid_channel_[c])
        		continue;

            memcpy(buffer->signal_buf[c], buffer->signal_buf[c] + buffer->buf_pos - buffer->BUF_HEAD_LEN, (buffer->BUF_HEAD_LEN + 3) * sizeof(signal_type));
            memcpy(buffer->filtered_signal_buf[c], buffer->filtered_signal_buf[c] + buffer->buf_pos - buffer->BUF_HEAD_LEN, buffer->BUF_HEAD_LEN * sizeof(ws_type));
        }
        
        buffer->zero_level = buffer->buf_pos + 1;
        buffer->buf_pos = buffer->BUF_HEAD_LEN;
		buffer->buf_pos_trig_ = buffer->BUF_HEAD_LEN;
		// TODO !!! DO THIS IN PROPER PLACE
		buffer->filt_pos_ = buffer->BUF_HEAD_LEN - 25;
    }
    else{
        buffer->zero_level = 0;
    }
    
    // data extraction:
    // t_bin *ch_dat =  (t_bin*)(block + HEADER_LEN + BLOCK_SIZE * batch + 2 * CH_MAP[CHANNEL]);
    
    // Log("Read chunks: ", buffer->num_chunks);

    unsigned char *bin_ptr = buffer->chunk_buf_ + HEADER_LEN;
    
    // read pos
    for (unsigned int c=0; c < num_chunks; ++c){
    	unsigned char *pos_chunk = buffer->chunk_buf_ + c * CHUNK_SIZE;

    	// check whether the package contains the tracking information
    	char pos_flag = *((char*)pos_chunk + 3);
    	if ( (mode128_ && (pos_flag == 'B' || pos_flag == 'F')) ||
    			(!mode128_ && (pos_flag != '1'))){
    		// extract position
    		unsigned short bx = *((unsigned short*)(pos_chunk + 16));
    		unsigned short by = *((unsigned short*)(pos_chunk + 18));
    		unsigned short sx = *((unsigned short*)(pos_chunk + 20));
    		unsigned short sy = *((unsigned short*)(pos_chunk + 22));

    		// skip every other position sample
    		skip_next_pos_ = ! skip_next_pos_;
    		if (!skip_next_pos_ && read_pos_){
    			// WORKAROUND
    			if (bx != buffer->pos_unknown_){
    				buffer->positions_buf_[buffer->pos_buf_pos_].x_big_LED_ = (bx * SCALE) + buffer->coord_shift_x_;
    				buffer->positions_buf_[buffer->pos_buf_pos_].y_big_LED_ = (by * SCALE) + buffer->coord_shift_y_;
    				buffer->positions_buf_[buffer->pos_buf_pos_].valid = true;
    			} else {
    				buffer->positions_buf_[buffer->pos_buf_pos_].x_big_LED_ = nanf("");
    				buffer->positions_buf_[buffer->pos_buf_pos_].y_big_LED_ = nanf("");
    			}

    			if (sx != buffer->pos_unknown_){
    				buffer->positions_buf_[buffer->pos_buf_pos_].x_small_LED_ = (sx * SCALE) + buffer->coord_shift_x_;
    				buffer->positions_buf_[buffer->pos_buf_pos_].y_small_LED_ = (sy * SCALE) + buffer->coord_shift_y_;
    				buffer->positions_buf_[buffer->pos_buf_pos_].valid = true;
    			} else {
    				buffer->positions_buf_[buffer->pos_buf_pos_].x_small_LED_ = nanf("");
    				buffer->positions_buf_[buffer->pos_buf_pos_].y_small_LED_ = nanf("");
    			}
    			buffer->positions_buf_[buffer->pos_buf_pos_].pkg_id_ = buffer->last_pkg_id + c;

    			buffer->AdvancePositionBufferPointer();

    			// TMPDEBUG
    			if (buffer->pos_buf_pos_ >= buffer->POS_BUF_LEN){
    				Log("WARNING: POS BUF overflow!");
    			}
    		}
    	}
    }
    
    // number of full packages read so far (1-64 or 1-128)
    unsigned int full_packages_read = 0;

    for (unsigned int chunk=0; chunk < num_chunks; ++chunk, bin_ptr += TAIL_LEN + HEADER_LEN) {
        unsigned char *pos_chunk = buffer->chunk_buf_ + chunk * CHUNK_SIZE;

        // check whether the package contains the tracking information
        char pos_flag = *((char*)pos_chunk + 3);
        unsigned int chnum_shift = (pos_flag < 'C' || !mode128_) ? 0 : 64;

        // TODO: !!! which comes first ? - check bin file headers !!!
        if (!mode128_ || pos_flag > 'C'){
        	full_packages_read ++;
        }

    	for (int block=0; block < 3; ++block) {
            short * sbin_ptr = (short*)bin_ptr;

            for (unsigned int c=0; c < 64; ++c, sbin_ptr++) {

            	if (!buffer->is_valid_channel_[CH_MAP_INV[c] + chnum_shift])
            		continue;

                // !!!??? +1 to make similar to *.dat
            	// TODO cut to char after PCA computation only ???
            	// TODO validate OOB ?
#ifdef CHAR_SIGNAL
                 buffer->signal_buf[CH_MAP_INV[c] + chnum_shift][buffer->buf_pos + full_packages_read * 3 + block] = (*(sbin_ptr) + 1) / 256;
#else
                 buffer->signal_buf[CH_MAP_INV[c] + chnum_shift][buffer->buf_pos + full_packages_read * 3 + block] = *(sbin_ptr) + 1;
#endif
				// MAPPING TEST
				//buffer->signal_buf[c][buffer->buf_pos + chunk * 3 + block] = *(sbin_ptr)+1;
                
                // DEBUG
                //if (c == 0)
                //    printf("%d\n", *((short*)bin_ptr) + 1);

                // TMPDEBUG
//         		if (c == 0){
//         			buffer->debug_stream_ << (int)buffer->signal_buf[CH_MAP_INV[c] + chnum_shift][buffer->buf_pos + chunk*3 + block] << "\n";
//         		}
            }
            bin_ptr += 2 * 64;
        }
    }

    buffer->buf_pos += 3 * full_packages_read;
	// TODO extract from package
    buffer->last_pkg_id += 3 * full_packages_read;

    // reset input data buffer pointers
    {
#ifdef PIPELINE_THREAD
    	std::lock_guard<std::mutex> lk(buffer->chunk_access_mtx_);
#endif
    	buffer->chunk_buf_ptr_in_ = 0;
    }

	// DEBUG
//	if (!(buffer->last_pkg_id % buffer->SAMPLING_RATE / 4)){
//		Log("pkg id = ",  buffer->last_pkg_id);
//		Log("pkg value (chan 4) = ", (unsigned int)buffer->signal_buf[4][buffer->buf_pos - 1]);
//	}
    
    // TODO: use spike filter width / 2
    buffer->RemoveSpikesOutsideWindow(buffer->last_pkg_id - 20);

	//Log("Package extraction done");

    // DEBUG - performance test
    // buffer->CheckPkgIdAndReportTime(buffer->last_pkg_id - num_chunks * 3, buffer->last_pkg_id - num_chunks / 2 * 3, Utils::Converter::int2str(buffer->last_pkg_id) + " read data with target package, start clock, #of spikes = " + Utils::Converter::int2str(buffer->spike_buf_pos) + "\n", true);

    if (buffer->last_pkg_id - last_reported_ > report_rate_){
    	last_reported_ = buffer->last_pkg_id;
    	Log(std::string("Processed ") + Utils::Converter::int2str(last_reported_ / (buffer->SAMPLING_RATE * 60)) + " minutes of data...");
    	unsigned int check_time_ = (time(0) - last_check_point_);
    	last_check_point_ = time(0);
    	Log(std::string("\tin ") + Utils::Converter::int2str(check_time_) + " seconds");
    	Log("With # of spikes: ", buffer->spike_buf_pos);
		Log("And chunks: ", total_chunks_);
		Log("And last_pkg_id: ", buffer->last_pkg_id);
//    	exit(0);
    }
}

std::string PackageExractorProcessor::name() {
	return "Package Extractor";
}
