//
//  PositionDisplayProcessor.h
//  sdl_example
//
//  Created by Igor Gridchyn on 30/04/14.
//  Copyright (c) 2014 Igor Gridchyn. All rights reserved.
//

#ifndef __sdl_example__PositionDisplayProcessor__
#define __sdl_example__PositionDisplayProcessor__

#include <iostream>
#include "LFPProcessor.h"

class PositionDisplayProcessor : public SDLControlInputProcessor, public SDLSingleWindowDisplay{
    unsigned int target_tetrode_;
    std::vector<bool> display_cluster_;
    
    enum PosDisplayMode{
    	POS_DISPLAY_ALL,
    	POS_DISPLAY_TAIL
    };

    PosDisplayMode disp_mode_ = POS_DISPLAY_ALL;

    const unsigned int TAIL_LENGTH;
    const bool WAIT_PREDICTION;
    const bool DISPLAY_PREDICTION;

public:
    PositionDisplayProcessor(LFPBuffer *buf);
    PositionDisplayProcessor(LFPBuffer *buf, std::string window_name, const unsigned int& window_width,
    		const unsigned int& window_height, const unsigned int& target_tetrode, const unsigned int& tail_length);
   
    
    virtual void process();
    
    // SDLControlInputProcessor
    virtual void process_SDL_control_input(const SDL_Event& e);
    virtual void SetDisplayTetrode(const unsigned int& display_tetrode);

};

#endif /* defined(__sdl_example__PositionDisplayProcessor__) */
