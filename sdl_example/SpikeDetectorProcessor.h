//
//  SpikeDetectorProcessor.h
//  sdl_example
//
//  Created by Igor Gridchyn on 04/06/14.
//  Copyright (c) 2014 Igor Gridchyn. All rights reserved.
//

#ifndef sdl_example_SpikeDetectorProcessor_h
#define sdl_example_SpikeDetectorProcessor_h

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

public:
    SpikeDetectorProcessor(LFPBuffer* buffer);
    SpikeDetectorProcessor(LFPBuffer* buffer, const char* filter_path, const float nstd, const int refractory);
    virtual void process();
    virtual inline std::string name() { return "Spike Detector"; }
};

#endif
