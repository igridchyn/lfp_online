//
//  SDLControlInputMetaProcessor.cpp
//  sdl_example
//
//  Created by Igor Gridchyn on 05/06/14.
//  Copyright (c) 2014 Igor Gridchyn. All rights reserved.
//

#include "LFPProcessor.h"

void SDLControlInputMetaProcessor::process(){
    // check meta-events, control change, pass control to current processor
    if (buffer->last_pkg_id == last_pkg_id){
        calls_since_scan ++;
    }
    
    last_pkg_id = buffer->last_pkg_id;
    
    // for effectiveness: perform analysis every input_scan_rate_ packages
    // TODO: select reasonable rate / TIME DELAY ?
    if (buffer->last_pkg_id - last_input_pkg_id_ < input_scan_rate_ && calls_since_scan < INPUT_SCAN_RATE_CALLS){
        // WORKAROUND for IDLE processing (no new packages)
        return;
    }
    else{
        last_input_pkg_id_ = buffer->last_pkg_id;
        calls_since_scan = 0;
    }
    
    SDL_Event e;
    bool quit = false;
    
    // SDL_PollEvent took 2/3 of runtime without limitations
    while( SDL_PollEvent( &e ) != 0 )
    {
        //User requests quit
        if( e.type == SDL_QUIT ) {
            quit = true;
        }
        else {
            // check for control switch
            if( e.type == SDL_KEYDOWN ){
                SDL_Keymod kmod = SDL_GetModState();
                if (kmod & KMOD_LCTRL){
                    // switch to corresponding processor
                    const unsigned int& cp_num = (unsigned int)control_processors_.size();

                    if (cp_num == 0 )
                    	return;
                    
                    switch( e.key.keysym.sym )
                    {
                            // TODO: out of range check
                        case SDLK_0:
                            control_processor_ = control_processors_[MIN(0, cp_num - 1)];
                            break;
                        case SDLK_1:
                            control_processor_ = control_processors_[MIN(1, cp_num - 1)];
                            break;
                        case SDLK_2:
                            control_processor_ = control_processors_[MIN(2, cp_num - 1)];
                            break;
                        case SDLK_3:
                            control_processor_ = control_processors_[MIN(3, cp_num - 1)];
                            break;
                        case SDLK_4:
                            control_processor_ = control_processors_[MIN(4, cp_num - 1)];
                            break;
                    }
                    
                    continue;
                }
                
                if (kmod & KMOD_LALT) {
                    // switch tetrode
                    
                    switch( e.key.keysym.sym ) {
                            // TODO: all tetrodes (10-16: numpad; 17-32: RALT)
                            
                        case SDLK_0:
                            SwitchDisplayTetrode(0);
                            break;
                        case SDLK_1:
                            SwitchDisplayTetrode(1);
                            break;
                        case SDLK_2:
                            SwitchDisplayTetrode(2);
                            break;
                        case SDLK_3:
                            SwitchDisplayTetrode(3);
                            break;
                        case SDLK_4:
                            SwitchDisplayTetrode(4);
                            break;
                        case SDLK_5:
                            SwitchDisplayTetrode(5);
                            break;
                        case SDLK_6:
                            SwitchDisplayTetrode(6);
                            break;
                        case SDLK_7:
                            SwitchDisplayTetrode(7);
                            break;
                        case SDLK_8:
                            SwitchDisplayTetrode(8);
                            break;
                        case SDLK_9:
                            SwitchDisplayTetrode(9);
                            break;
                    }
                    
                    continue;
                }
            }
            
            control_processor_->process_SDL_control_input(e);
        }
    }
}

SDLControlInputMetaProcessor::SDLControlInputMetaProcessor(LFPBuffer* buffer, std::vector<SDLControlInputProcessor *> control_processors)
: LFPProcessor(buffer)
, control_processors_(control_processors)
{
	Log("Constructor start");

	if (control_processors_.size() > 0 && control_processors_[0] != NULL)
		control_processor_ = control_processors_[0];

	Log("Constructor done");
}

void SDLControlInputMetaProcessor::SwitchDisplayTetrode(const unsigned int& display_tetrode){
    for (int pi=0; pi < control_processors_.size(); ++pi) {
        control_processors_[pi]->SetDisplayTetrode(display_tetrode);
    }
}

SDLControlInputProcessor::SDLControlInputProcessor(LFPBuffer *buf)
: LFPProcessor(buf) { }

FileOutputProcessor::FileOutputProcessor(LFPBuffer* buf)
: LFPProcessor(buf){
    f_ = fopen("/Users/igridchyn/Dropbox/IST_Austria/Csicsvari/Data Processing/spike_detection/cpp/cppout.txt", "w");
}


std::string SDLControlInputMetaProcessor::name() {
	return "SDLControlInputMetaProcessor";
}
