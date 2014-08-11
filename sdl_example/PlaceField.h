//
//  PlaceField.h
//  sdl_example
//
//  Created by Igor Gridchyn on 06/06/14.
//  Copyright (c) 2014 Igor Gridchyn. All rights reserved.
//

#ifndef __sdl_example__PlaceField__
#define __sdl_example__PlaceField__

#include <iostream>

#include <armadillo>

#include "LFPBuffer.h"

class PlaceField{
    arma::mat place_field_;
    arma::cube pdf_cache_;
    
    double sigma_;
    double bin_size_;
    // how many bins around spikes to take into account
    int spread_;
    
public:
    static const int MAX_SPIKES = 20;
    
    enum PDFType{
        Poisson,
        Gaussian
    };
    
    PlaceField(const double& sigma, const double& bin_size, const unsigned int& nbins, const unsigned int& spread);
    PlaceField(const arma::mat& mat, const double& sigma, const double& bin_size, const unsigned int& spread);

    // PlaceField doesn't know about its identity and doesn't check spikes
    void AddSpike(Spike *spike);
    
    inline const double& operator()(unsigned int x, unsigned int y) const { return place_field_(x, y); }
    inline       double& operator()(unsigned int x, unsigned int y)       { return place_field_(x, y); }
    
    inline size_t Width() const { return place_field_.n_cols; }
    inline size_t Height() const { return place_field_.n_rows; }
    
    inline const double Max() const { return place_field_.max(); }
    
    PlaceField Smooth();
    
    void CachePDF(PDFType pdf_type, const PlaceField& occupancy, const double& occupancy_factor);

    // TODO: OOB check
    inline const double& Prob(unsigned int r, unsigned int c, unsigned int s) { return pdf_cache_(r, c, s); }
    
    inline const arma::mat& Mat() const { return place_field_; }

    void Load(const std::string path, arma::file_type ft);
};

#endif /* defined(__sdl_example__PlaceField__) */
