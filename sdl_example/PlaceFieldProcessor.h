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
#include "SDLSingleWindowDisplay.h"

//==========================================================================================

class PlaceFieldProcessor : virtual public LFPProcessor, virtual public SDLControlInputProcessor, virtual public SDLSingleWindowDisplay {
    const float EPS = 0.001f;
    
    // unsigned int spike_buf_pos_;
    unsigned int pos_buf_pos_ = 0;
    
    std::vector<PlaceField> tetrode_spike_probs_;

    arma::mat reconstructed_position_;
    
    // [tetrode] [cluster] [session]
    std::vector< std::vector< std::vector< PlaceField > > > place_fields_;
    std::vector< std::vector< std::vector< PlaceField > > > place_fields_smoothed_;
    
    double sigma_;
    double bin_size_;
    unsigned int nbinsx_;
    unsigned int nbinsy_;
    unsigned int spread_;
    
    // [session]
    std::vector<PlaceField> occupancy_;
    std::vector<PlaceField> occupancy_smoothed_;

    unsigned int display_tetrode_ = 0;

    bool pos_updated_ = false;
    unsigned int last_predicted_pkg_ = 0;
    
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

    bool display_prediction_;

    const unsigned int prediction_rate_;

    float POS_SAMPLING_RATE = 0.0;
    double MIN_OCCUPANCY;
    const float SPEED_THOLD;

    unsigned int N_SESSIONS;

    // to which session last received spike belonged
    unsigned int current_session_ = 0;

    // user-selected session for display
    unsigned int selected_session_ = 0;

    //================================
    
    const double DISPLAY_SCALE;
    bool wait_file_read_ = false;

    template <class T>
    void drawMat(const arma::Mat<T>& mat, const std::vector<std::string> text_output = std::vector<std::string>());
    
    void drawPlaceField();
    void drawOccupancy();
    void drawPrediction();
    
    void AddPos(float x, float y, unsigned int time);
    
    // cache Poisson / Normal distribution to be used for fast position inference
    void cachePDF();
    
    // smooth pfs with spike counts into map
    void smoothPlaceFields();
    
    void initArrays();

    // predict
    void ReconstructPosition(std::vector<std::vector<unsigned int > > pop_vec);
    
public:
    
    PlaceFieldProcessor(LFPBuffer *buf, const double& sigma, const double& bin_size, const unsigned int& nbinsx, const unsigned int& nbinsy,
    		const unsigned int& spread, const bool& load, const bool& save, const std::string& base_path,
    		const float& prediction_fr_thold, const unsigned int& min_pkg_id, const bool& use_prior,
    		const unsigned int& processors_number);
    
    // call the constructor above after reading params from config
    PlaceFieldProcessor(LFPBuffer *buf, const unsigned int& processors_number);
    virtual ~PlaceFieldProcessor() {};

//    const arma::mat& GetSmoothedOccupancy();

    // LFPProcessor
    virtual void process();
    virtual inline std::string name() { return "Place Field"; }
    
    // SDLSingleWindowDisplay
    virtual void process_SDL_control_input(const SDL_Event& e);
    virtual void SetDisplayTetrode(const unsigned int& display_tetrode);
    
    void switchSession(const unsigned int& session);
    void dumpCluAndRes();
    void dumpPlaceFields();

};

#endif
