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
    arma::cube pdf_cache_;
    
    double sigma_;
    double bin_size_;
    // how many bins around spikes to take into account
    int spread_;
    
    static const int MAX_SPIKES = 20;
    
public:
    enum PDFType{
        Poisson,
        Gaussian
    };
    
    PlaceField(const double& sigma, const double& bin_size, const unsigned int& nbins, const unsigned int& spread);
    // PlaceField doesn't know about its identity and doesn't check spikes
    void AddSpike(Spike *spike);
    
    inline const double& operator()(unsigned int x, unsigned int y) const { return place_field_(x, y); }
    inline       double& operator()(unsigned int x, unsigned int y)       { return place_field_(x, y); }
    
    inline size_t Width() const { return place_field_.n_cols; }
    inline size_t Height() const { return place_field_.n_rows; }
    
    inline const double Max() const { return place_field_.max(); }
    
    PlaceField Smooth();
    
    void CachePDF(PDFType pdf_type);
    
    inline const double& Prob(unsigned int r, unsigned int c, unsigned int s) { return pdf_cache_(r, c, s); }
    
    inline const arma::mat& Mat() const { return place_field_; }
};

//==========================================================================================

class PlaceFieldProcessor : public SDLControlInputProcessor, public SDLSingleWindowDisplay {
    constexpr static const float SPEED_THOLD = 50.0f;
    constexpr static const float EPS = 0.001f;
    
    unsigned int spike_buf_pos_;
    unsigned int pos_buf_pos_;
    
    PlaceField occupancy_;
    PlaceField occupancy_smoothed_;
    
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
    
    // TODO: improve
    // -1 = occupancy, -2 = reconstructed
    int display_cluster_ = 0;
    
    bool pdf_cached_ = false;
    
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
    
    PlaceFieldProcessor(LFPBuffer *buf, const double& sigma, const double& bin_size, const unsigned int& nbins, const unsigned int& spread);
    
    // LFPProcessor
    virtual void process();
    
    // SDLSingleWindowDisplay
    virtual void process_SDL_control_input(const SDL_Event& e);
    virtual void SetDisplayTetrode(const unsigned int& display_tetrode);
    
};

#endif
