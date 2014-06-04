//
//  SDLSignalDisplayProcessor.h
//  sdl_example
//
//  Created by Igor Gridchyn on 04/06/14.
//  Copyright (c) 2014 Igor Gridchyn. All rights reserved.
//

#ifndef sdl_example_SDLSignalDisplayProcessor_h
#define sdl_example_SDLSignalDisplayProcessor_h

class SDLSignalDisplayProcessor : public SDLControlInputProcessor, public SDLSingleWindowDisplay{
    static const int SCREEN_HEIGHT = 600;
    static const int SCREEN_WIDTH = 1280;
    
    static const int DISP_FREQ = 30;
    
    // 11000
    static const int SHIFT = 3000; // 1300
    
    int plot_scale = 40; // 40 - 1
    int plot_hor_scale = 10; // controlled
    
    //
    unsigned int displayed_channels_number_;
    unsigned int *displayed_channels_;
    
    // previous displayed coordinate (y)
    int *prev_vals_;
    
    int transform_to_y_coord(int voltage);
    void drawLine(SDL_Renderer *renderer, int x1, int y1, int x2, int y2);
    int current_x;
    
    // last disaplued position in buffer
    int last_disp_pos;
    
    // number of unrendered signal samples (subject to threshold)
    int unrendered = 0;
    
public:
    virtual void process();
    virtual void SetDisplayTetrode(const int& display_tetrode);
    
    SDLSignalDisplayProcessor(LFPBuffer *buffer, std::string window_name, const unsigned int& window_width, const unsigned int& window_height, unsigned int displayed_channels_number, unsigned int *displayed_channels);
    
    virtual void process_SDL_control_input(const SDL_Event& e);
};

#endif
