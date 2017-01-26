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
#include "SDLSingleWindowDisplay.h"

class SDLPCADisplayProcessor : virtual public SDLControlInputProcessor, virtual public SDLSingleWindowDisplay{
    
    ColorPalette palette_;
    int target_tetrode_;
    
    // displayed components, can be changed by the control keys
    unsigned int comp1_ = 0; // 0 - 1st channel, PC1;
    unsigned int comp2_ = 1; // 1 - 2nd channel, PC1;
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

    // considered to be the same for all tetrodes
    unsigned int num_pc_;

    // beware: this using numbering from 0, while 0 means unknown cluster
    int selected_cluster1_ = -1;
    int selected_cluster2_ = -1;

    // cluster, for which spikes within refractory period are displayed
    //	 = -1 if none
    int refractory_display_cluster_ = -1;
    // time of last spike in the given cluster
    unsigned int refractory_last_time_ = 0;
    unsigned int refractory_period_ = 48;

    // to filter spikes by threshold in the clustering
    const unsigned int power_thold_nstd_;
    double power_threshold_factor_ = 0.1;
    const double power_threshold_factor_step_ = 1.05;

    // caching points to display to speedup
    // ONLY for during massive redraw operations
    // per cluster
    // TODO !!! parametrize
    const unsigned int spikes_draw_freq_ = 1;
    std::vector<SDL_Point *> spikes_to_draw_;
    std::vector<unsigned int> spikes_counts_;

    const unsigned int MAX_CLUST = 40;

    // x coordidate -> cluster number
    std::vector<unsigned int> number_panel_mapping_;

    void save_polygon_clusters();
    inline float scale_x(float x) { return x / scale_ + shift_x_; }
    inline float scale_y(float y) { return y / scale_ + shift_y_; }

	void reset_spike_pointer();

	// cluster operations
	void addCluster();
	void mergeClusters();
	void deleteCluster();
	void addExclusiveProjection();
	void displayChiHistogramm();
	PolygonCluster createNewCluster(unsigned int & clun);

	// get coordinates of spike in current projection
	void getSpikeCoords(const Spike *const, int& x, int& y);

	SDL_Point *points_;

	std::vector<bool> display_cluster_;

	bool need_clust_check_ = true;
	int current_projection_ = -1;

	// conrtol-toggled larger size of the selected clsuter spikes
	bool highlight_current_cluster_ = false;

	// the clustering mode in which no projections / cuts are saved and upon any operations only all available spikes are porcessed once
	bool memory_less_clustering_ = true;

	// drawing subsampling - 1 by default, controlled by LALT+ / LALT- : for better resolution of high - density areas
	unsigned int draw_subsample_factor = 1;
	unsigned int total_spikes_to_draw = 0;


public:
    SDLPCADisplayProcessor(LFPBuffer *buffer);
    SDLPCADisplayProcessor(LFPBuffer *buffer, std::string window_name, const unsigned int window_width,
    		const unsigned int window_height, int target_tetrode, bool display_unclassified, const float& scale,
    		const int shift_x, const int shift_y);
    
    // LFPProcessor
    virtual void process();
    virtual inline std::string name() { return "PC display"; }
    
    virtual void SetDisplayTetrode(const unsigned int& display_tetrode);
    
    // SDLSingleWindowDisplay
    virtual void process_SDL_control_input(const SDL_Event& e);

	virtual ~SDLPCADisplayProcessor();
};

#endif
