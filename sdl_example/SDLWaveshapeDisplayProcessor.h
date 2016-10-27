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
#include "PutativeCell.h"

class SDLWaveshapeDisplayProcessor : virtual public SDLControlInputProcessor, virtual public SDLSingleWindowDisplay {
    unsigned last_disp_pkg_id_ = 0;
    
    unsigned int targ_tetrode_ = 0;

    unsigned int tetrode_total_spikes_ = 0;
    
    float scale_;

	unsigned int spike_plot_rate_;

    const unsigned int DISPLAY_RATE;

    // TODO compute from window height / width
    const unsigned int x_mult_final_ = 32;
    const unsigned int x_mult_reconstructed_ = 4;
    const unsigned int y_mult_ = 200;

    int x1_ = -1;
    int y1_ = -1;
    int x2_ = -1;
    int y2_ = -1;

	// if true, final waveshape (16 points) used for PCA is displayed
	// otherwise - reconstructed 4x upsampled ws
	bool display_final_ = false;

    unsigned int& buf_pointer_;

	unsigned int selected_channel_ = 0;

	float XToWaveshapeSampleNumber(int x);
	float YToPower(int chan, int y);

	void displayClusterCuts(const int & cluster_id, int highlight_number);
	void reinit();

	unsigned int last_ua_id_ = 0;

	// TODO finish implementation
	// [tetrode][cluster][channel][sample]
	//unsigned int mean_waveshapes_[32][40][4][128] = {0};

	std::string cuts_file_path_;
	bool cuts_save_, cuts_load_;

	int current_cut_ = -1;

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

    void saveCuts();
};

#endif
