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

class PositionDisplayProcessor : public LFPProcessor{
    SDL_Window *window_;
    SDL_Renderer *renderer_;
    SDL_Texture *texture_;
    
public:
    PositionDisplayProcessor(LFPBuffer *buf, SDL_Window *window, SDL_Renderer *renderer, SDL_Texture *texture)
    : LFPProcessor(buf)
    , window_(window)
    , renderer_(renderer)
    , texture_(texture) { }
    
    virtual void process();
};

#endif /* defined(__sdl_example__PositionDisplayProcessor__) */
