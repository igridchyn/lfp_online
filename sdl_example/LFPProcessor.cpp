//
//  LFPProcessor.cpp
//  sdl_example
//
//  Created by Igor Gridchyn on 31/03/14.
//  Copyright (c) 2014 Igor Gridchyn. All rights reserved.
//

#include "LFPProcessor.h"
#include <fstream>
#include <vector>

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

class LFPBuffer{
    static const int CHANNEL_NUM = 32;
    static const int LFP_BUF_LEN = 2048;
    
    int signal_buf[CHANNEL_NUM][LFP_BUF_LEN];
    
public:
    int buf_pos;
    int last_pkg_id;
    
    inline int get_signal(int channel, int pkg_id);
};
    
class Spike{
    static const int WL_LENGTH = 32;
    
public:
    int pkg_id;
    int waveshape[WL_LENGTH];
    
    Spike(LFPBuffer *buffer, int pkg_id, int channel);
};

class LFPPipeline{
    
};

class LFPProcessor{
    virtual void process(LFPBuffer* buffer) = 0;
};

class SpikeDetectorProcessor : LFPProcessor{
    // TODO: read from config
    
    static const int POWER_BUF_LEN = 1024;
    static const int SAMPLE_RATE = 24000;
    static const int REFR_LEN = (int)SAMPLE_RATE / 1000;
    
    float filter[ 2 << 7];
    int filter_len;
    
    const int channel;
    const int detection_threshold;
    
    int powerBufPos = 0;
    float powerBuf[POWER_BUF_LEN];
    float powerSum;
    
    int last_processed_id;
    
    std::vector<Spike*> spikes;
    
    SpikeDetectorProcessor(const char* filter_path, const int channel, const float detection_threshold);
    virtual void process(LFPBuffer* buffer);
};

SpikeDetectorProcessor::SpikeDetectorProcessor(const char* filter_path, const int channel, const float detection_threshold)
    : last_processed_id(0)
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

void SpikeDetectorProcessor::process(LFPBuffer* buffer){
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
            spikes.push_back(new Spike(buffer, pow_max_id, channel));
        }
    }
}

Spike::Spike(LFPBuffer *buffer, int pkg_id, int channel)
    : pkg_id(pkg_id)
{
    // pos is where spike has been detected -> so, approx. middle of the spike
    for (int s = -WL_LENGTH/2; s < WL_LENGTH/2; ++s){
        waveshape[s + WL_LENGTH/2] = buffer->get_signal(channel, pkg_id);
    }
}

inline int LFPBuffer::get_signal(int channel, int pkg_id){
    return signal_buf[channel][(pkg_id - last_pkg_id + buf_pos) % LFP_BUF_LEN];
}