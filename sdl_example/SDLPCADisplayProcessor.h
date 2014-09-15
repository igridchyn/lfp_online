//
//  SDLPCADisplayProcessor.h
//  sdl_example
//
//  Created by Igor Gridchyn on 04/06/14.
//  Copyright (c) 2014 Igor Gridchyn. All rights reserved.
//

#ifndef sdl_example_SDLPCADisplayProcessor_h
#define sdl_example_SDLPCADisplayProcessor_h

#include "PolygonCluster.h"

class SDLPCADisplayProcessor : virtual public SDLSingleWindowDisplay, virtual public SDLControlInputProcessor{
    
    ColorPalette palette_;
    // TODO: display for multiple tetrodes with ability to switch
    int target_tetrode_;
    
    // displayed components, can be changed by the control keys
    unsigned int comp1_ = 0; // 0 - 1st channel, PC1;
    unsigned int comp2_ = 1; // 1 - 1st channel, PC2;
    unsigned int nchan_;
    
    // diplay non-assigned spikes
    bool display_unclassified_;
    
    float scale_;
    int shift_x_;
    int shift_y_;

    unsigned int time_start_= 0;
    unsigned int time_end_;

    bool polygon_closed_ = false;
    std::vector<float> polygon_x_;
    std::vector<float> polygon_y_;

    // polygon clusters
    std::vector< std::vector<PolygonCluster> > polygon_clusters_;

    // rendering frequency (redraw each rend_freq spikes)
    const int rend_freq_;

    // save/load polygon clusters
    const int poly_save_;
    const int poly_load_;
    std::string poly_path_;

    // beware: this using numbering from 0, while 0 means unknown cluster
    int selected_cluster1_ = -1;
    int selected_cluster2_ = -1;

    void save_polygon_clusters();
    inline float scale_x(float x) { return x / scale_ + shift_x_; }
    inline float scale_y(float y) { return y / scale_ + shift_y_; }

public:
    SDLPCADisplayProcessor(LFPBuffer *buffer);
    SDLPCADisplayProcessor(LFPBuffer *buffer, std::string window_name, const unsigned int window_width,
    		const unsigned int window_height, int target_tetrode, bool display_unclassified, const float& scale,
    		const int shift_x, const int shift_y);
    
    // LFPProcessor
    virtual void process();
    
    virtual void SetDisplayTetrode(const unsigned int& display_tetrode);
    
    // SDLSingleWindowDisplay
    virtual void process_SDL_control_input(const SDL_Event& e);

	virtual ~SDLPCADisplayProcessor(){};
};

#endif
