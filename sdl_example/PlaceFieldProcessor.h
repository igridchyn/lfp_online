//
//  PlaceFieldProcessor.h
//  sdl_example
//
//  Created by Igor Gridchyn on 04/06/14.
//  Copyright (c) 2014 Igor Gridchyn. All rights reserved.
//

#ifndef sdl_example_PlaceFieldProcessor_h
#define sdl_example_PlaceFieldProcessor_h

#include <armadillo>
#include "PlaceField.h"

//==========================================================================================

class PlaceFieldProcessor : public SDLControlInputProcessor, public SDLSingleWindowDisplay {
    constexpr static const float SPEED_THOLD = 80.0f;
    constexpr static const float EPS = 0.001f;
    
    unsigned int spike_buf_pos_;
    unsigned int pos_buf_pos_;
    
    PlaceField occupancy_;
    PlaceField occupancy_smoothed_;
    
    std::vector<PlaceField> tetrode_spike_probs_;

    arma::mat reconstructed_position_;
    
    std::vector< std::vector< PlaceField > > place_fields_;
    std::vector< std::vector< PlaceField > > place_fields_smoothed_;
    
    double bin_size_;
    double sigma_;
    unsigned int nbins_;
    double spread_;
    
    unsigned int display_tetrode_ = 0;
    
    bool display_prediction_ = false;
    bool pos_updated_ = false;
    unsigned int last_predicted_pkg_ = 0;
    
    // every 100 ms
    const unsigned int prediction_rate_ = 100 * 24;
    
    // TODO: improve
    // -1 = occupancy, -2 = reconstructed
    int display_cluster_ = 0;
    
    bool pdf_cached_ = false;
    
    const bool SAVE;
    const bool LOAD;
    const std::string BASE_PATH;

    // 1/3 clusters at 2
    const double RREDICTION_FIRING_RATE_THRESHOLD;

    const unsigned int MIN_PKG_ID;

    const bool USE_PRIOR;

    //================================
    
    void drawMat(const arma::mat& mat);
    
    void drawPlaceField();
    void drawOccupancy();
    void drawPrediction();
    
    void AddPos(int x, int y);
    
    // cache Poisson / Normal distribution to be used for fast position inference
    void cachePDF();
    
    // smooth pfs with spike counts into map
    void smoothPlaceFields();
    
    // predict
    void ReconstructPosition(std::vector<std::vector<unsigned int > > pop_vec);
    
public:
    
    PlaceFieldProcessor(LFPBuffer *buf, const double& sigma, const double& bin_size, const unsigned int& nbins,
    		const unsigned int& spread, const bool& load, const bool& save, const std::string& base_path,
    		const float& prediction_fr_thold, const unsigned int& min_pkg_id, const bool& use_prior);
    
    const arma::mat& GetSmoothedOccupancy();

    // LFPProcessor
    virtual void process();
    
    // SDLSingleWindowDisplay
    virtual void process_SDL_control_input(const SDL_Event& e);
    virtual void SetDisplayTetrode(const unsigned int& display_tetrode);
    
};

#endif
