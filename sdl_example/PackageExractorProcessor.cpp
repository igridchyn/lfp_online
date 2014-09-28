//
//  PackageExractorProcessor.cpp
//  sdl_example
//
//  Created by Igor Gridchyn on 05/06/14.
//  Copyright (c) 2014 Igor Gridchyn. All rights reserved.
//

#include "LFPProcessor.h"
#include "OnlineEstimator.cpp"

PackageExractorProcessor::PackageExractorProcessor(LFPBuffer *buffer)
    : LFPProcessor(buffer)
	, SCALE(buffer->config_->getFloat("pack.extr.xyscale"))
{
	Log("Created");
}

void PackageExractorProcessor::process(){
	//Log("Extract package...");

    // IDLE processing, waiting for user input
    if (buffer->chunk_ptr == nullptr){
		buffer->Log("PEXTR: zero pointer, return");
        return;
    }
    
    // see if buffer reinit is needed, rewind buffer
    if (buffer->buf_pos + 3 * buffer->num_chunks > buffer->LFP_BUF_LEN - buffer->BUF_HEAD_LEN){
        for (int c=0; c < buffer->CHANNEL_NUM; ++c){
            memcpy(buffer->signal_buf[c], buffer->signal_buf[c] + buffer->buf_pos - buffer->BUF_HEAD_LEN, buffer->BUF_HEAD_LEN * sizeof(int));
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
    char pos_flag = *((char*)buffer->chunk_ptr + 3);
    if (pos_flag != '1'){
        // extract position
        unsigned short bx = *((unsigned short*)(buffer->chunk_ptr + 16));
        unsigned short by = *((unsigned short*)(buffer->chunk_ptr + 18));
        unsigned short sx = *((unsigned short*)(buffer->chunk_ptr + 20));
        unsigned short sy = *((unsigned short*)(buffer->chunk_ptr + 22));
        
        // EVERY 240 !!! = 100 Hz
//        std::cout << "new pos at " << buffer->buf_pos << "\n";

        // WORKAROUND
        if (bx != buffer->pos_unknown_){
        	buffer->positions_buf_[buffer->pos_buf_pos_][0] = (unsigned int)(bx * SCALE);
        	buffer->positions_buf_[buffer->pos_buf_pos_][1] = (unsigned int)(by * SCALE);
        }else{
        	buffer->positions_buf_[buffer->pos_buf_pos_][0] = (unsigned int)buffer->pos_unknown_;
        	buffer->positions_buf_[buffer->pos_buf_pos_][1] = (unsigned int)buffer->pos_unknown_;
        }
        buffer->positions_buf_[buffer->pos_buf_pos_][2] = (unsigned int)sx;
        buffer->positions_buf_[buffer->pos_buf_pos_][3] = (unsigned int)sy;
        buffer->positions_buf_[buffer->pos_buf_pos_][4] = (unsigned int)buffer->last_pkg_id;
        
        buffer->pos_buf_pos_++;
    }
    
    for (int chunk=0; chunk < buffer->num_chunks; ++chunk, bin_ptr += buffer->TAIL_LEN + buffer->HEADER_LEN) {
        for (int block=0; block < 3; ++block) {
            short * sbin_ptr = (short*)bin_ptr;
            for (int c=0; c < buffer->CHANNEL_NUM; ++c, sbin_ptr++) {
                // ??? is it necessary to order the channels?
                // ??? filter directly the data in bin buffer? [for better performance]
                
                // !!!??? +1 to make similar to *.dat
                buffer->signal_buf[buffer->CH_MAP_INV[c]][buffer->buf_pos + chunk*3 + block] = *(sbin_ptr) + 1;
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
	if (!(buffer->last_pkg_id % buffer->SAMPLING_RATE / 4)){
		buffer->log_stream << "INFO: PackExtr pkg id = " << buffer->last_pkg_id << "\n";
		buffer->log_stream << "INFO: PackExtr pkg value (chan 4) = " << buffer->signal_buf[4][buffer->buf_pos - 1]<< "\n";
		buffer->log_stream.flush();
	}
    
    // TODO: use filter width !!!
    buffer->RemoveSpikesOutsideWindow(buffer->last_pkg_id - 20);

	//Log("Package extraction done");
}

std::string PackageExractorProcessor::name() {
	return "PackageExtractorProcessor";
}
