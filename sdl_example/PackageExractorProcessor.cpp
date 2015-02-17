//
//  PackageExractorProcessor.cpp
//  sdl_example
//
//  Created by Igor Gridchyn on 05/06/14.
//  Copyright (c) 2014 Igor Gridchyn. All rights reserved.
//

#include "PackageExtractorProcessor.h"
#include "OnlineEstimator.cpp"

PackageExractorProcessor::PackageExractorProcessor(LFPBuffer *buffer)
    : LFPProcessor(buffer)
	, SCALE(buffer->config_->getFloat("pack.extr.xyscale"))
	, exit_on_data_over_(buffer->config_->getBool("pack.extr.exit.on.over", false))
	, read_pos_(buffer->config_->getBool("pack.extr.read.pos", true))
{
	Log("Created");
	report_rate_ = buffer->SAMPLING_RATE * 60;
}

void PackageExractorProcessor::process(){
	//Log("Extract package...");

    // IDLE processing, waiting for user input
    if (buffer->chunk_ptr == nullptr){
//		buffer->Log("PEXTR: zero pointer, return");
    	if (!end_reported_){
    		Log("EOF reached!");
    		end_reported_ = true;
    	}
    	else{
    		// give chance to all subsequent processors to process...
    		if (exit_on_data_over_){
    			Log("Exit on data over...");
    			exit(0);
    		}
    	}
        return;
    }
    
    // see if buffer reinit is needed, rewind buffer
    if (buffer->buf_pos + 3 * buffer->num_chunks > buffer->LFP_BUF_LEN - buffer->BUF_HEAD_LEN){
        for (int c=0; c < buffer->CHANNEL_NUM; ++c){
        	// TODO WAS INT ?
            memcpy(buffer->signal_buf[c], buffer->signal_buf[c] + buffer->buf_pos - buffer->BUF_HEAD_LEN, buffer->BUF_HEAD_LEN * sizeof(signal_type));
            memcpy(buffer->filtered_signal_buf[c], buffer->filtered_signal_buf[c] + buffer->buf_pos - buffer->BUF_HEAD_LEN, buffer->BUF_HEAD_LEN * sizeof(int));
        }
        
        buffer->zero_level = buffer->buf_pos + 1;
        buffer->buf_pos = buffer->BUF_HEAD_LEN;
		buffer->buf_pos_trig_ = buffer->BUF_HEAD_LEN;
        
        // std::cout << "SIGNAL BUFFER REWIND (at pos " << buffer->buf_pos <<  ")!\n";
    }
    else{
        buffer->zero_level = 0;
    }
    
    // data extraction:
    // t_bin *ch_dat =  (t_bin*)(block + HEADER_LEN + BLOCK_SIZE * batch + 2 * CH_MAP[CHANNEL]);
    
    unsigned char *bin_ptr = buffer->chunk_ptr + buffer->HEADER_LEN;
    
	// TODO !!!!!!!!! check pos in all packages
    for (int c=0; c < buffer->num_chunks; ++c){
    	unsigned char *pos_chunk = buffer->chunk_ptr + c * buffer->CHUNK_SIZE;

    	char pos_flag = *((char*)pos_chunk + 3);
    	if (pos_flag != '1'){
    		if (buffer->pos_first_pkg_ == -1){
    			buffer->pos_first_pkg_ = buffer->last_pkg_id + c;
    		}

    		// extract position
    		unsigned short bx = *((unsigned short*)(pos_chunk + 16));
    		unsigned short by = *((unsigned short*)(pos_chunk + 18));
    		unsigned short sx = *((unsigned short*)(pos_chunk + 20));
    		unsigned short sy = *((unsigned short*)(pos_chunk + 22));

    		// EVERY 240 !!! = 100 Hz
//    		std::cout << "new pos at " << buffer->buf_pos << ": " << bx << ", " << by  << "\n";

    		// skip every other position sample
    		skip_next_pos_ = ! skip_next_pos_;
    		if (!skip_next_pos_ && read_pos_){
    			// WORKAROUND
    			if (bx != buffer->pos_unknown_){
    				buffer->positions_buf_[buffer->pos_buf_pos_].x_big_LED_ = (bx * SCALE) + buffer->coord_shift_x_;
    				buffer->positions_buf_[buffer->pos_buf_pos_].y_big_LED_ = (by * SCALE) + buffer->coord_shift_y_;
    				buffer->positions_buf_[buffer->pos_buf_pos_].valid = true;
    			}
    			if (sx != buffer->pos_unknown_){
    				buffer->positions_buf_[buffer->pos_buf_pos_].x_small_LED_ = (sx * SCALE) + buffer->coord_shift_x_;
    				buffer->positions_buf_[buffer->pos_buf_pos_].y_small_LED_ = (sy * SCALE) + buffer->coord_shift_y_;
    				buffer->positions_buf_[buffer->pos_buf_pos_].valid = true;
    			}
    			buffer->positions_buf_[buffer->pos_buf_pos_].pkg_id_ = buffer->last_pkg_id + c;

    			buffer->pos_buf_pos_++;;
    		}
    	}
    }
    
    for (int chunk=0; chunk < buffer->num_chunks; ++chunk, bin_ptr += buffer->TAIL_LEN + buffer->HEADER_LEN) {
        for (int block=0; block < 3; ++block) {
            short * sbin_ptr = (short*)bin_ptr;
            for (int c=0; c < buffer->CHANNEL_NUM; ++c, sbin_ptr++) {
                // ??? is it necessary to order the channels?
                // ??? filter directly the data in bin buffer? [for better performance]
                
                // !!!??? +1 to make similar to *.dat
            	// TODO cut to char after PCA computation only ???
            	// TODO validate OOB ?
            	// TODO find efficient solution without using define
#ifdef CHAR_SIGNAL
                 buffer->signal_buf[buffer->CH_MAP_INV[c]][buffer->buf_pos + chunk*3 + block] = ((*(sbin_ptr) + 1) / 256);
#else
                 buffer->signal_buf[buffer->CH_MAP_INV[c]][buffer->buf_pos + chunk*3 + block] = (*(sbin_ptr) + 1);
#endif
				// MAPPING TEST
				//buffer->signal_buf[c][buffer->buf_pos + chunk * 3 + block] = *(sbin_ptr)+1;
                
                // DEBUG
                //if (c == 0)
                //    printf("%d\n", *((short*)bin_ptr) + 1);
            }
            bin_ptr += 2 *  buffer->CHANNEL_NUM;
        }
    }
    
    buffer->buf_pos += 3 * buffer->num_chunks;
	// TODO extract from package
    buffer->last_pkg_id += 3 * buffer->num_chunks;

	// DEBUG
//	if (!(buffer->last_pkg_id % buffer->SAMPLING_RATE / 4)){
//		Log("pkg id = ",  buffer->last_pkg_id);
//		Log("pkg value (chan 4) = ", (unsigned int)buffer->signal_buf[4][buffer->buf_pos - 1]);
//	}
    
    // TODO: use filter width !!!
    buffer->RemoveSpikesOutsideWindow(buffer->last_pkg_id - 20);

	//Log("Package extraction done");

    // DEBUG - performance test
    buffer->CheckPkgIdAndReportTime(buffer->last_pkg_id, Utils::Converter::int2str(buffer->last_pkg_id) + " pkg is reached, start clock\n", true);

    if (buffer->last_pkg_id - last_reported_ > report_rate_){
    	last_reported_ = buffer->last_pkg_id;
    	Log(std::string("Processed ") + Utils::Converter::int2str(last_reported_ / (buffer->SAMPLING_RATE * 60)) + " minutes of data...");
    }
}

std::string PackageExractorProcessor::name() {
	return "PackageExtractorProcessor";
}
