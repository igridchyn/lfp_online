//
//  SDLSignalDisplayProcessor.h
//  sdl_example
//
//  Created by Igor Gridchyn on 04/06/14.
//  Copyright (c) 2014 Igor Gridchyn. All rights reserved.
//

#ifndef sdl_example_SDLSignalDisplayProcessor_h
#define sdl_example_SDLSignalDisplayProcessor_h

#include "LFPProcessor.h"
#include "SDLSingleWindowDisplay.h"

class SDLSignalDisplayProcessor : virtual public SDLControlInputProcessor, virtual public SDLSingleWindowDisplay{
    static const int DISP_FREQ = 30;

    std::vector<unsigned int> displayed_channels_;
	unsigned int display_tetrode_;
    
    // previous displayed coordinate (y)
    int *prev_vals_ = nullptr;
    
    int transform_to_y_coord(int voltage);
    void drawLine(SDL_Renderer *renderer, int x1, int y1, int x2, int y2);
    unsigned int current_x;
    
    // last disaplued position in buffer
    int last_disp_pos;
    
    const unsigned int SCREEN_HEIGHT;
    const unsigned int SCREEN_WIDTH;

    int plot_hor_scale; // controlled
    float plot_scale; // 40 - 1

    int SHIFT;

    // number of unrendered signal samples (subject to threshold)
    int unrendered = 0;
    
public:
    virtual std::string name();
    virtual void process();
    virtual void SetDisplayTetrode(const unsigned int& display_tetrode);
    
    SDLSignalDisplayProcessor(LFPBuffer *buffer);
    SDLSignalDisplayProcessor(LFPBuffer *buffer, std::string window_name, const unsigned int& window_width,
    		const unsigned int& window_height, std::vector<unsigned int> displayed_channels);
    
    virtual void process_SDL_control_input(const SDL_Event& e);
};

#endif
