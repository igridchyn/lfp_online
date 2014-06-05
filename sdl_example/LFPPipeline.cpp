//
//  LFPPipeline.cpp
//  sdl_example
//
//  Created by Igor Gridchyn on 05/06/14.
//  Copyright (c) 2014 Igor Gridchyn. All rights reserved.
//

#include "LFPPipeline.h"

void LFPPipeline::process(unsigned char *data, int nchunks){
    // TODO: put data into buffer
    
    for (std::vector<LFPProcessor*>::const_iterator piter = processors.begin(); piter != processors.end(); ++piter) {
        (*piter)->process();
    }
}

LFPProcessor *LFPPipeline::get_processor(const unsigned int& index){
    return processors[index];
}

std::vector<SDLControlInputProcessor *> LFPPipeline::GetSDLControlInputProcessors(){
    // TODO: use vector
    std::vector<SDLControlInputProcessor *> control_processors;
    
    for (int p=0; p<processors.size(); ++p) {
        SDLControlInputProcessor *ciproc = dynamic_cast<SDLControlInputProcessor*>(processors[p]);
        if (ciproc != NULL){
            control_processors.push_back(ciproc);
        }
    }
    
    return control_processors;
}