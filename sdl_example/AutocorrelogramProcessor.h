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
#include "SDLSingleWindowDisplay.h"

#endif /* defined(__sdl_example__AutocorrelogramProcessor__) */

enum AC_DISPLAY_MODE{
	AC_DISPLAY_MODE_AC,
	AC_DISPLAY_MODE_CC
};

class AutocorrelogramProcessor : virtual public SDLSingleWindowDisplay, virtual public SDLControlInputProcessor{

	// TODO synchronize init accross all processors
    unsigned int display_tetrode_ = 0;

    // whether to break on -1 cluster or just skip it
    bool wait_clustering_;

    // pixel width of one AC bin
    const int BWIDTH = 2;
    const int CC_BWIDTH = 1;

    // number of clusters in one row
    // TODO define from window width
    const int XCLUST = 7;
    // height of the plot in pixels
    const unsigned int ypix_ = 100;

    // tetrode / cluster / bin values
    std::vector<std::vector<std::vector<float> > > autocorrs_;
    // [tetrode] [ cluster 1] [cluster 2] [ bin ]
    std::vector<std::vector<std::vector<std::vector<float> > > > cross_corrs_;

    AC_DISPLAY_MODE display_mode_ = AC_DISPLAY_MODE_CC;

    std::vector<std::vector<std::list<unsigned int> > > spike_times_lists_;

    static const unsigned int MAX_CLUST = 30;

    const int BIN_SIZE;
    const unsigned int NBINS;

    UserContext& user_context_;
    unsigned int last_processed_user_action_id_;

    unsigned int getCCXShift(const unsigned int& clust1, const unsigned int& clust2);
    unsigned int getCCYShift(const unsigned int& clust1, const unsigned int& clust2);

    unsigned int getXShift(int clust);
    unsigned int getYShift(int clust);
    void drawClusterRect(int clust);

    //
    int getClusterNumberByCoords(const unsigned int& x, const unsigned int& y);

    void plotACorCCs(int tetrode, int cluster);

public:
    AutocorrelogramProcessor(LFPBuffer *buf);
    AutocorrelogramProcessor(LFPBuffer *buf, const float bin_size_ms, const unsigned int nbins);

    virtual void process();
    virtual ~AutocorrelogramProcessor() {};

    void plotAC(const unsigned int tetr, const unsigned int cluster);
    void plotCC(const unsigned int& tetr, const unsigned int& cluster1, const unsigned int& cluster2);

    // SDLControlInputProcessor

    virtual void process_SDL_control_input(const SDL_Event& e);
    virtual void SetDisplayTetrode(const unsigned int& display_tetrode);
};
