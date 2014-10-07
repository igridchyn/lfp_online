//
//  AutocorrelogramProcessor.h
//  sdl_example
//
//  Created by Igor Gridchyn on 27/05/14.
//  Copyright (c) 2014 Igor Gridchyn. All rights reserved.
//

#ifndef __sdl_example__AutocorrelogramProcessor__
#define __sdl_example__AutocorrelogramProcessor__

#include <iostream>

#include "LFPProcessor.h"

#endif /* defined(__sdl_example__AutocorrelogramProcessor__) */

class AutocorrelogramProcessor : virtual public SDLSingleWindowDisplay, virtual public SDLControlInputProcessor{

	// TODO synchronize init accross all processors
    unsigned int display_tetrode_ = 0;
    bool wait_clustering_;

    const int BWIDTH = 2;
    const int XCLUST = 7;

    // tetrode / cluster / bin values
    std::vector<std::vector<std::vector<float> > > autocorrs_;
    std::vector<std::vector<unsigned int> > total_counts_;

    std::vector<std::vector<std::vector<unsigned int> > > spike_times_buf_;
    std::vector<std::vector<unsigned int> > spike_times_buf_pos_;

    std::vector<std::vector<bool> > reported_;

    static const unsigned int ST_BUF_SIZE = 30;
    static const unsigned int MAX_CLUST = 30;
    // total of AVG_PER_BIN * NBINS should be collected before displaying the autocorrelation
    const unsigned int MIN_EVENTS;
    static const unsigned int Y_SCALE = 20;

    const int BIN_SIZE;
    const unsigned int NBINS;

    UserContext& user_context_;
    unsigned int last_processed_user_action_id_;

    const unsigned int ypix_ = 50;

    unsigned int getXShift(int clust);
    unsigned int getYShift(int clust);
    void drawClusterRect(int clust);

    //
    int getClusterNumberByCoords(const unsigned int& x, const unsigned int& y);

public:
    AutocorrelogramProcessor(LFPBuffer *buf);
    AutocorrelogramProcessor(LFPBuffer *buf, const float bin_size_ms, const unsigned int nbins);

    virtual void process();
    virtual ~AutocorrelogramProcessor() {};

    void plotAC(const unsigned int tetr, const unsigned int cluster);

    // SDLControlInputProcessor

    virtual void process_SDL_control_input(const SDL_Event& e);
    virtual void SetDisplayTetrode(const unsigned int& display_tetrode);
};
