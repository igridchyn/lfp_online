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

class PositionDisplayProcessor : public LFPProcessor, public SDLSingleWindowDisplay{
    const unsigned int target_tetrode_;
    
public:
    PositionDisplayProcessor(LFPBuffer *buf, std::string window_name, const unsigned int& window_width, const unsigned int& window_height, const unsigned int& target_tetrode);
   
    
    virtual void process();
};

#endif /* defined(__sdl_example__PositionDisplayProcessor__) */
