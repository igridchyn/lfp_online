//
//  PositionReconstructionProcessor.h
//  sdl_example
//
//  Created by Igor Gridchyn on 04/06/14.
//  Copyright (c) 2014 Igor Gridchyn. All rights reserved.
//

#ifndef __sdl_example__PositionReconstructionProcessor__
#define __sdl_example__PositionReconstructionProcessor__

#include <iostream>

#include "LFPProcessor.h"

class PositionReconstructionProcessor : public LFPProcessor {
public:
    
    // LFPProcessor methods
    virtual void process();
};

#endif /* defined(__sdl_example__PositionReconstructionProcessor__) */
