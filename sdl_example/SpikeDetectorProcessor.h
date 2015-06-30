//
//  SpikeDetectorProcessor.h
//  sdl_example
//
//  Created by Igor Gridchyn on 04/06/14.
//  Copyright (c) 2014 Jozsef Csicsvari, Igor Gridchyn. All rights reserved.
//

#ifndef sdl_example_SpikeDetectorProcessor_h
#define sdl_example_SpikeDetectorProcessor_h

#include <mutex>

class SpikeDetectorProcessor : public LFPProcessor{
    
    // int is not enough for convolution
    long long filter_int_ [ 2 << 7 ];
    
    float filter[ 2 << 7];
    unsigned int filter_len;

    float nstd_;
    unsigned int refractory_;

    // position of last processed position in filter array
    // after process() should be equal to buf_pos
    int filt_pos = 0;
    int det_pos = 0;

    int *thresholds_;
    
    std::vector<Spike*> spikes;
    
    const unsigned int min_power_samples_;

    const unsigned int DET_THOLD_CALC_RATE_;

    std::mutex spike_add_mtx_;

public:
    SpikeDetectorProcessor(LFPBuffer* buffer);
    SpikeDetectorProcessor(LFPBuffer* buffer, const char* filter_path, const float nstd, const int refractory);
    virtual void process();
    virtual void process_tetrode(int tetrode_to_process);
    virtual inline std::string name() { return "Spike Detector"; }

    virtual void desync();
    virtual void sync();

private:
    void filter_channel(unsigned int channel);
    void update_threshold(unsigned int channel);
};

#endif
