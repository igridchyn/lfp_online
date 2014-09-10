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
    	// changed focus -> switch control + redraw
    	 if (e.type == SDL_WINDOWEVENT) {
    		 if( e.window.event == SDL_WINDOWEVENT_FOCUS_GAINED ) {
    		     	if (cp_by_win_id_.find(e.window.windowID) != cp_by_win_id_.end()){
    		     		buffer->Log("Change focus");
    		     		control_processor_ = cp_by_win_id_[e.window.windowID];
    		     	}
    		  }
    	 }

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

                	int shift = (kmod & KMOD_LSHIFT) ? 10 : 0;
                	int tetrode = -1;
                    
                    switch( e.key.keysym.sym ) {
                            // TODO: all tetrodes (10-16: numpad; 17-32: RALT)
                        case SDLK_0:
                            tetrode = 0 + shift;
                            break;
                        case SDLK_1:
                            tetrode = 1 + shift;
                            break;
                        case SDLK_2:
                            tetrode = 2 + shift;
                            break;
                        case SDLK_3:
                            tetrode = 3 + shift;
                            break;
                        case SDLK_4:
                            tetrode = 4 + shift;
                            break;
                        case SDLK_5:
                            tetrode = 5 + shift;
                            break;
                        case SDLK_6:
                            tetrode = 6 + shift;
                            break;
                        case SDLK_7:
                            tetrode = 7 + shift;
                            break;
                        case SDLK_8:
                            tetrode = 8 + shift;
                            break;
                        case SDLK_9:
                            tetrode = 9 + shift;
                            break;
                    }
                    
                    if (tetrode >= 0){
                    	if (tetrode < buffer->tetr_info_->tetrodes_number){
                    		SwitchDisplayTetrode(tetrode);
                    		buffer->Log(std::string("Switch displays to tetrode #") + Utils::NUMBERS[tetrode]);
                    	}
                    	else{
                    		buffer->Log("Requested tetrode number is higher that number of available tetrodes");
                    	}
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

	for(int i = 0; i < control_processors_.size(); ++i){
		SDLSingleWindowDisplay *cp_w = dynamic_cast<SDLSingleWindowDisplay*>(control_processors_[i]);
		cp_by_win_id_[cp_w->GetWindowID()] = control_processors_[i];
	}
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
