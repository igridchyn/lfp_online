//
//  LFPProcessor.cpp
//  sdl_example
//
//  Created by Igor Gridchyn on 31/03/14.
//  Copyright (c) 2014 Igor Gridchyn. All rights reserved.
//

#include "LFPProcessor.h"
#include <fstream>

// REQUIREMENTS AND SPECIFICATION
// data structure for LFP information (for all processors)
// data incoming for processing in BATCHES (of variable size, potentially)
// channel processor in a separate THREAD
// higher level processors have to combine information accross channels
// large amount of data to be stored:
//      write to file / use buffer for last events ( for batch writing )

// OPTIMIZE
//  iteration over packages in LFPBuffer - create buffer iterator TILL last package

// ???
// do all processors know information about subsequent structure?
//  NO
// put spikes into buffer ? (has to be available later)



const int LFPBuffer::CH_MAP_INV[] = {8,9,10,11,12,13,14,15,24,25,26,27,28,29,30,31,40,41,42,43,44,45,46,47,56,57,58,59,60,61,62,63,0,1,2,3,4,5,6,7,16,17,18,19,20,21,22,23,32,33,34,35,36,37,38,39,48,49,50,51,52,53,54,55};

const int LFPBuffer::CH_MAP[] = {32, 33, 34, 35, 36, 37, 38, 39,0, 1, 2, 3, 4, 5, 6, 7,40, 41, 42, 43, 44, 45, 46, 47,8, 9, 10, 11, 12, 13, 14, 15,48, 49, 50, 51, 52, 53, 54, 55,16, 17, 18, 19, 20, 21, 22, 23,56, 57, 58, 59, 60, 61, 62, 63,24, 25, 26, 27, 28, 29, 30, 31};




// ============================================================================================================

void LFPPipeline::process(unsigned char *data, int nchunks){
    // TODO: put data into buffer
    
    for (std::vector<LFPProcessor*>::const_iterator piter = processors.begin(); piter != processors.end(); ++piter) {
        (*piter)->process();
    }
}


// ============================================================================================================

SpikeDetectorProcessor::SpikeDetectorProcessor(LFPBuffer* buffer, const char* filter_path, const int channel, const float detection_threshold)
    : LFPProcessor(buffer)
    ,last_processed_id(0)
    , channel(channel)
    , detection_threshold(detection_threshold)
{
    // load spike filter
    std::ifstream filter_stream;
    filter_stream.open(filter_path);
    
    int fpos = 0;
    while(!filter_stream.eof()){
         filter_stream >> filter[fpos];
    }
    filter_len = fpos;
}

void SpikeDetectorProcessor::process(LFPBuffer* buffer)
{
    bool spike_detected = false;
    int det_start = 0;
    int pow_max = 0;
    int pow_max_id = 0;
    
    for (int pkg_id = last_processed_id; pkg_id < buffer->last_pkg_id; ++pkg_id) {
        float pow = 0;
        for (int j=0; j < filter_len; ++j) {
            pow += filter[j] * buffer->get_signal(channel, pkg_id - filter_len + j);
        }
        
        powerSum -= powerBuf[(powerBufPos-filter_len) % POWER_BUF_LEN];
        powerSum += pow;
        powerBuf[powerBufPos] = pow;
        
        powerBufPos++ ;
        powerBufPos %= POWER_BUF_LEN;
        
        if (powerSum > detection_threshold){
            // align peak - jsut a local max value, store [snpeakb]
            // TODO: accross all electrodes of a tetrode
            if (!spike_detected){
                spike_detected = true;
                det_start = pkg_id;
                pow_max = powerSum;
                pow_max_id = pkg_id;
            }
            else{
                if (powerSum > pow_max){
                    pow_max = powerSum;
                    pow_max_id = pkg_id;
                }
            }
        }
        
        if (spike_detected && pkg_id - det_start >= REFR_LEN){
            spike_detected = false;
            spikes.push_back(new Spike(buffer->signal_buf[0], pow_max_id, channel));
        }
    }
}

// ============================================================================================================
void PackageExractorProcessor::process(){

    // modify only in the Package Extractor !!!
    buffer->buf_pos = (buffer->buf_pos+1) % buffer->LFP_BUF_LEN;
    
    unsigned char *bin_ptr = buffer->chunk_ptr + buffer->HEADER_LEN;
    for (int chunk=0; chunk < buffer->num_chunks; ++chunk, bin_ptr += buffer->TAIL_LEN) {
        for (int block=0; block < 3; ++block) {
            for (int c=0; c < buffer->CHANNEL_NUM; ++c, bin_ptr += 2) {
                // ??? is it necessary to order the channels?
                // ??? filter directly the data in bin buffer? [for better performance]
                
                buffer->signal_buf[buffer->CH_MAP_INV[c]][buffer->buf_pos] = *((short*)bin_ptr);
                
                // DEBUG
                if (c == 0)
                    printf("%d\n", *((short*)bin_ptr));
            }
        }
    }
}

// ============================================================================================================

Spike::Spike(int *buffer, int pkg_id, int channel)
    : pkg_id(pkg_id)
{
    // pos is where spike has been detected -> so, approx. middle of the spike
    for (int s = -WL_LENGTH/2; s < WL_LENGTH/2; ++s){
        // BS
        waveshape[s + WL_LENGTH/2] = buffer[pkg_id];
    }
}

inline int LFPBuffer::get_signal(int channel, int pkg_id){
    return signal_buf[channel][(pkg_id - last_pkg_id + buf_pos) % LFP_BUF_LEN];
}