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

class AutocorrelogramProcessor : virtual public SDLControlInputProcessor, virtual public SDLSingleWindowDisplay{

    unsigned int display_tetrode_ = 0;

    // pixel width of one AC bin
    const int BWIDTH = 2;
    const int CC_BWIDTH = 1;

    // number of clusters in one row
    // TODO define from window width
    int XCLUST = 4;
    int YCLUST = 5;
    int XCLUST_CC = 6;
    int YCLUST_CC = 5;
    // height of the plot in pixels
    const unsigned int ypix_ = 100;

    // tetrode / cluster / bin values
    std::vector<std::vector<std::vector<unsigned int> > > autocorrs_;
    // [tetrode] [ cluster 1] [cluster 2] [ bin ]
    std::vector<std::vector<std::vector<std::vector<int> > > > cross_corrs_;

    std::vector<std::vector<unsigned int> > spike_counts_;

    AC_DISPLAY_MODE display_mode_ = AC_DISPLAY_MODE_CC;

    std::vector<std::vector<std::list<unsigned int> > > spike_times_lists_;

    const int BIN_SIZE;
    const unsigned int MAX_CLUST;
    const unsigned int NBINS;

    //unsigned int last_processed_user_action_id_;
    UserAction const *last_user_action_ = NULL;

    // whether to break on -1 cluster or just skip it
    bool wait_clustering_;

    double refractory_fraction_threshold_ = 0.01;

    // reset mode - process only given tetrode and cluster until reset_mode_end_
    bool reset_mode_ = false;
    unsigned int reset_mode_end_ = 0;
//    unsigned int reset_cluster_ = -1;
    std::vector<bool> reset_cluster_;

    // page shifts
    unsigned int page_x_ = 0;
    unsigned int page_y_ = 0;
    unsigned int page_x_ac_ = 0;

    unsigned int getCCXShift(const unsigned int& clust1);
    unsigned int getCCYShift(const unsigned int& clust2);

    unsigned int getXShift(int clust);
    unsigned int getYShift(int clust);
    void drawClusterRect(int clust);

    //
    int getClusterNumberByCoords(const unsigned int& x, const unsigned int& y);
    void getPairByCoords(const unsigned int& x, const unsigned int& y, unsigned int & c1, unsigned int & c2);

    void plotACorCCs(int tetrode, int cluster);

    void clearACandCCs(const unsigned int& clu);

public:
    AutocorrelogramProcessor(LFPBuffer *buf);
    AutocorrelogramProcessor(LFPBuffer *buf, const float bin_size_ms, const unsigned int nbins);

    virtual void process();
    virtual inline std::string name() { return "Autocorrelogram"; }
    virtual ~AutocorrelogramProcessor();

    void plotAC(const unsigned int tetr, const unsigned int cluster);
    void plotCC(const unsigned int& tetr, const unsigned int& cluster1, const unsigned int& cluster2);

    // SDLControlInputProcessor

    virtual void process_SDL_control_input(const SDL_Event& e);
    virtual void SetDisplayTetrode(const unsigned int& display_tetrode);

    virtual void Resize();
};
