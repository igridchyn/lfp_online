//
//  SDLPCADisplayProcessor.h
//  sdl_example
//
//  Created by Igor Gridchyn on 04/06/14.
//  Copyright (c) 2014 Igor Gridchyn. All rights reserved.
//

#ifndef sdl_example_SDLPCADisplayProcessor_h
#define sdl_example_SDLPCADisplayProcessor_h

class SDLPCADisplayProcessor : public SDLSingleWindowDisplay, public SDLControlInputProcessor{
    
    ColorPalette palette_;
    // TODO: display for multiple tetrodes with ability to switch
    int target_tetrode_;
    
    // displayed components, can be changed by the control keys
    unsigned int comp1_ = 0;
    unsigned int comp2_ = 4; // 4 - 2nd channel, PC1; 1 - 1st channel, PC2;
    unsigned int nchan_;
    
    // diplay non-assigned spikes
    bool display_unclassified_;
    
    float scale_;
    int shift_;

    unsigned int time_start_= 0;
    unsigned int time_end_;

public:
    SDLPCADisplayProcessor(LFPBuffer *buffer);
    SDLPCADisplayProcessor(LFPBuffer *buffer, std::string window_name, const unsigned int window_width, const unsigned int window_height, int target_tetrode, bool display_unclassified, const float& scale, const int shift);
    
    // LFPProcessor
    virtual void process();
    
    virtual void SetDisplayTetrode(const unsigned int& display_tetrode) { target_tetrode_ = display_tetrode; ReinitScreen(); buffer->spike_buf_no_disp_pca = buffer->SPIKE_BUF_HEAD_LEN; }
    
    // SDLSingleWindowDisplay
    virtual void process_SDL_control_input(const SDL_Event& e);
};

#endif
