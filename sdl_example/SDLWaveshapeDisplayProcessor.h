//
//  SDLWaveshapeDisplayProcessor.h
//  sdl_example
//
//  Created by Igor Gridchyn on 04/06/14.
//  Copyright (c) 2014 Igor Gridchyn. All rights reserved.
//

#ifndef sdl_example_SDLWaveshapeDisplayProcessor_h
#define sdl_example_SDLWaveshapeDisplayProcessor_h

class SDLWaveshapeDisplayProcessor : virtual public SDLSingleWindowDisplay, virtual public SDLControlInputProcessor {
    unsigned int buf_pointer_;
    unsigned last_disp_pkg_id_ = 0;
    
    unsigned int targ_tetrode_ = 0;
    int disp_cluster_ = 0;
    
    unsigned int tetrode_total_spikes_ = 0;
    
    static const unsigned int DISPLAY_RATE = 10;
    
	unsigned int spike_plot_rate_;

    const float scale_;

public:
    SDLWaveshapeDisplayProcessor(LFPBuffer *buf);
    SDLWaveshapeDisplayProcessor(LFPBuffer *buf, const std::string& window_name, const unsigned int& window_width,
    		const unsigned int& window_height);
    
    // LFPProcessor
    virtual void process();
    
    // SDLSingleWindowDisplay
    virtual void process_SDL_control_input(const SDL_Event& e);
    virtual void SetDisplayTetrode(const unsigned int& display_tetrode);

    float transform(float smpl, int chan);
};

#endif
