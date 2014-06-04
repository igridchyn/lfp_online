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

class PlaceField{
    arma::mat place_field_;
    
    double sigma_;
    double bin_size_;
    // how many bins around spikes to take into account
    int spread_;
    
public:
    PlaceField(const double& sigma, const double& bin_size, const unsigned int& nbins, const unsigned int& spread);
    // PlaceField doesn't know about its identity and doesn't check spikes
    void AddSpike(Spike *spike);
    
    inline const double& operator()(unsigned int x, unsigned int y) const { return place_field_(x, y); }
    
    inline size_t Width() const { return place_field_.n_cols; }
    inline size_t Height() const { return place_field_.n_rows; }
    
    inline const double Max() const { return place_field_.max(); }
};

//==========================================================================================

class PlaceFieldProcessor : public SDLControlInputProcessor, public SDLSingleWindowDisplay {
    constexpr static const float SPEED_THOLD = 50.0f;
    
    unsigned int spike_buf_pos_;
    unsigned int pos_buf_pos_;
    
    arma::mat occupancy_;
    
    std::vector< std::vector< PlaceField > > place_fields_;
    
    double bin_size_;
    double sigma_;
    unsigned int nbins_;
    double spread_;
    
    unsigned int display_tetrode_ = 0;
    unsigned int display_cluster_ = 0;
    
    //================================
    
    void drawPlaceField();
    void drawOccupancy();
    void AddPos(int x, int y);
    
public:
    
    PlaceFieldProcessor(LFPBuffer *buf, const double& sigma, const double& bin_size, const unsigned int& nbins, const unsigned int& spread);
    
    // LFPProcessor
    virtual void process();
    
    // SDLSingleWindowDisplay
    virtual void process_SDL_control_input(const SDL_Event& e);
    virtual void SetDisplayTetrode(const unsigned int& display_tetrode);
    
};

#endif
