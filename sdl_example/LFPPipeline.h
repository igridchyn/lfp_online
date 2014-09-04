//
//  LFPPipeline.h
//  sdl_example
//
//  Created by Igor Gridchyn on 05/06/14.
//  Copyright (c) 2014 Igor Gridchyn. All rights reserved.
//

#ifndef __sdl_example__LFPPipeline__
#define __sdl_example__LFPPipeline__

#include <iostream>

#include "LFPProcessor.h"

class LFPONLINEAPI LFPPipeline{
    std::vector<LFPProcessor*> processors;
    
public:
    inline void add_processor(LFPProcessor* processor) {processors.push_back(processor);}
    void process(unsigned char *data);
    
    LFPProcessor *get_processor(const unsigned int& index);
    
    std::vector<SDLControlInputProcessor *> GetSDLControlInputProcessors();

    LFPPipeline(LFPBuffer *buf);
    ~LFPPipeline();
};

#endif /* defined(__sdl_example__LFPPipeline__) */
