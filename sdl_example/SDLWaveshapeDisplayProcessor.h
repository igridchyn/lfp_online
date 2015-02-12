//
//  SDLWaveshapeDisplayProcessor.h
//  sdl_example
//
//  Created by Igor Gridchyn on 04/06/14.
//  Copyright (c) 2014 Igor Gridchyn. All rights reserved.
//

#ifndef sdl_example_SDLWaveshapeDisplayProcessor_h
#define sdl_example_SDLWaveshapeDisplayProcessor_h

#include "SDLSingleWindowDisplay.h"

class WaveshapeCut{
public:
	int x1_;
	int y1_;
	int x2_;
	int y2_;

	unsigned int channel_;

	WaveshapeCut(int x1, int y1, int x2, int y2, const unsigned int& channel)
		: x1_(x1)
		, y1_(y1)
		, x2_(x2)
		, y2_(y2)
		, channel_(channel)
	{}
};

class SDLWaveshapeDisplayProcessor : virtual public SDLSingleWindowDisplay, virtual public SDLControlInputProcessor {
    unsigned int& buf_pointer_;
    unsigned last_disp_pkg_id_ = 0;
    
    unsigned int targ_tetrode_ = 0;

    unsigned int tetrode_total_spikes_ = 0;
    
    const unsigned int DISPLAY_RATE;
    
	unsigned int spike_plot_rate_;

    float scale_;

    // TODO compute from window height / width
    const unsigned int x_mult_final_ = 32;
    const unsigned int x_mult_reconstructed_ = 4;
    const unsigned int y_mult_ = 200;

    int x1_ = -1;
    int y1_ = -1;
    int x2_ = -1;
    int y2_ = -1;

    const unsigned int MAX_CLUST = 150;
    // [tetr][clust][cut]
    std::vector<std::vector<std::vector<WaveshapeCut> > > cluster_cuts_;

	// if true, final waveshape (16 points) used for PCA is displayed
	// otherwise - reconstructed 4x upsampled ws
	bool display_final_ = false;

	unsigned int selected_channel_ = 0;

	float XToWaveshapeSampleNumber(int x);
	float YToPower(int chan, int y);

	void displayClusterCuts(const int cluster_id);
	void reinit();

	unsigned int last_ua_id_ = 0;

public:
    SDLWaveshapeDisplayProcessor(LFPBuffer *buf);
    SDLWaveshapeDisplayProcessor(LFPBuffer *buf, const std::string& window_name, const unsigned int& window_width,
    		const unsigned int& window_height);
    
    // LFPProcessor
    virtual void process();
    virtual inline std::string name() { return "Waveshape Display"; }
    
    // SDLSingleWindowDisplay
    virtual void process_SDL_control_input(const SDL_Event& e);
    virtual void SetDisplayTetrode(const unsigned int& display_tetrode);

    float transform(float smpl, int chan);
};

#endif
