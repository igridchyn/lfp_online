//
//  PackageExractorProcessor.cpp
//  sdl_example
//
//  Created by Igor Gridchyn on 05/06/14.
//  Copyright (c) 2014 Igor Gridchyn. All rights reserved.
//

#include "LFPProcessor.h"
#include "OnlineEstimator.cpp"

void PackageExractorProcessor::process(){
    // IDLE processing, waiting for user input
    if (buffer->chunk_ptr == NULL){
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
        
        std::cout << "SIGNAL BUFFER REWIND (at pos " << buffer->buf_pos <<  ")!\n";
    }
    else{
        buffer->zero_level = 0;
    }
    
    // data extraction:
    // t_bin *ch_dat =  (t_bin*)(block + HEADER_LEN + BLOCK_SIZE * batch + 2 * CH_MAP[CHANNEL]);
    
    unsigned char *bin_ptr = buffer->chunk_ptr + buffer->HEADER_LEN;
    
    char pos_flag = *((char*)buffer->chunk_ptr + 3);
    if (pos_flag == '2'){
        // extract position
        unsigned short bx = *((unsigned short*)(buffer->chunk_ptr + 16));
        unsigned short by = *((unsigned short*)(buffer->chunk_ptr + 18));
        unsigned short sx = *((unsigned short*)(buffer->chunk_ptr + 20));
        unsigned short sy = *((unsigned short*)(buffer->chunk_ptr + 22));
        
        buffer->positions_buf_[buffer->pos_buf_pos_][0] = (unsigned int)bx;
        buffer->positions_buf_[buffer->pos_buf_pos_][1] = (unsigned int)by;
        buffer->positions_buf_[buffer->pos_buf_pos_][2] = (unsigned int)sx;
        buffer->positions_buf_[buffer->pos_buf_pos_][3] = (unsigned int)sy;
        buffer->positions_buf_[buffer->pos_buf_pos_][4] = (unsigned int)buffer->last_pkg_id;
        
        // speed estimation
        // TODO: use average of bx, sx or alike
        // TODO: deal with missing points
        if (buffer->pos_buf_pos_ > 16 && bx != 1023 && buffer->positions_buf_[buffer->pos_buf_pos_ - 16][0] != 1023){
            float dx = (float)bx - buffer->positions_buf_[buffer->pos_buf_pos_ - 16][0];
            float dy = (float)by - buffer->positions_buf_[buffer->pos_buf_pos_ - 16][1];
            buffer->speedEstimator_->push(sqrt(dx * dx + dy * dy));
            // TODO: float / scale ?
            buffer->positions_buf_[buffer->pos_buf_pos_ - 8][5] = buffer->speedEstimator_->get_mean_estimate();
            //            std::cout << "speed= " << buffer->speedEstimator_->get_mean_estimate() << "\n";
            
            // update spike speed
            int known_speed_pkg_id =  buffer->positions_buf_[buffer->pos_buf_pos_ - 8][4];
            while (true){
                Spike *spike = buffer->spike_buffer_[buffer->spike_buf_pos_speed_];
                if (spike == NULL || spike->pkg_id_ > known_speed_pkg_id){
                    break;
                }
                
                // find last position sample before the spike
                while(buffer->pos_buf_pos_spike_speed_ < buffer->pos_buf_pos_ - 8 && buffer->positions_buf_[buffer->pos_buf_pos_spike_speed_ + 1][4] < spike->pkg_id_){
                    buffer->pos_buf_pos_spike_speed_ ++;
                }
                
                // interpolate speed during spike :
                int diff_bef = spike->pkg_id_ - buffer->positions_buf_[buffer->pos_buf_pos_spike_speed_][4];
                int diff_aft = (int)(known_speed_pkg_id - spike->pkg_id_);
                float w_bef = 1/(float)(diff_bef + 1);
                float w_aft = 1/(float)(diff_aft + 1);
                // TODO: weights ?
                spike->speed = ( buffer->positions_buf_[buffer->pos_buf_pos_spike_speed_][5] * w_bef + buffer->positions_buf_[buffer->pos_buf_pos_ - 8][5] * w_aft) / (float)(w_bef + w_aft);
                //                std::cout << "Spike speed " << spike->speed << "\n";
                
                buffer->spike_buf_pos_speed_ ++;
            }
        }
        
        buffer->pos_buf_pos_++;
    }
    
    for (int chunk=0; chunk < buffer->num_chunks; ++chunk, bin_ptr += buffer->TAIL_LEN) {
        for (int block=0; block < 3; ++block) {
            short * sbin_ptr = (short*)bin_ptr;
            for (int c=0; c < buffer->CHANNEL_NUM; ++c, sbin_ptr++) {
                // ??? is it necessary to order the channels?
                // ??? filter directly the data in bin buffer? [for better performance]
                
                // !!!??? +1 to make similar to *.dat
                buffer->signal_buf[buffer->CH_MAP_INV[c]][buffer->buf_pos + chunk*3 + block] = *(sbin_ptr) + 1;
                
                // DEBUG
                //if (c == 0)
                //    printf("%d\n", *((short*)bin_ptr) + 1);
            }
            bin_ptr += 2 *  buffer->CHANNEL_NUM;
        }
    }
    
    buffer->buf_pos += 3 * buffer->num_chunks;
    buffer->last_pkg_id += 3 * buffer->num_chunks;
    
    // TODO: use filter width !!!
    buffer->RemoveSpikesOutsideWindow(buffer->last_pkg_id - 20);
}
