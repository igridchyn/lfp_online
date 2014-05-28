//
//  AutocorrelogramProcessor.h
//  sdl_example
//
//  Created by Igor Gridchyn on 27/05/14.
//  Copyright (c) 2014 Igor Gridchyn. All rights reserved.
//

#ifndef __sdl_example__AutocorrelogramProcessor__
#define __sdl_example__AutocorrelogramProcessor__

#include <iostream>

#include "LFPProcessor.h"

#endif /* defined(__sdl_example__AutocorrelogramProcessor__) */

class AutocorrelogramProcessor : public SDLSingleWindowDisplay, public SDLControlInputProcessor{
    
    unsigned int display_tetrode_ = 0;
    
    // TODO: rewind implementation
    unsigned int spike_buf_pos_auto_ = LFPBuffer::SPIKE_BUF_HEAD_LEN;
    
    // tetrode / cluster / bin values
    std::vector<std::vector<std::vector<float> > > autocorrs_;
    std::vector<std::vector<unsigned int> > total_counts_;
    
    std::vector<std::vector<std::vector<unsigned int> > > spike_times_buf_;
    std::vector<std::vector<unsigned int> > spike_times_buf_pos_;
    
    std::vector<std::vector<bool> > reported_;
    
    static const unsigned int ST_BUF_SIZE = 30;
    static const unsigned int NBINS = 15;
    static const unsigned int MAX_CLUST = 30;
    
public:
    AutocorrelogramProcessor(LFPBuffer *buf);
    
    virtual void process();
    
    void plotAC(const unsigned int tetr, const unsigned int cluster);
    
    // SDLControlInputProcessor
    
    virtual void process_SDL_control_input(const SDL_Event& e);
    virtual void SetDisplayTetrode(const unsigned int& display_tetrode);
};