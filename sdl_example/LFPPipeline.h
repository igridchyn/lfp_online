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
#include <condition_variable>

#include "LFPProcessor.h"

// !!! DON'T FORGET TO EXCLUDE BINARY READER !!!
// #define PIPELINE_THREAD

class LFPONLINEAPI LFPPipeline{
    std::vector<LFPProcessor*> processors;
	LFPBuffer *buf_ = nullptr;

public:
	std::mutex mtx_data_add_;
	std::condition_variable cv_data_added_;
	bool data_added_;

	std::thread *pipeline_thread_;

    inline void add_processor(LFPProcessor* processor) {processors.push_back(processor);}
    void process();
    
    LFPProcessor *get_processor(const unsigned int& index);
    
    std::vector<SDLControlInputProcessor *> GetSDLControlInputProcessors();

    LFPPipeline(LFPBuffer *buf);
    ~LFPPipeline();
};

#endif /* defined(__sdl_example__LFPPipeline__) */
