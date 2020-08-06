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

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

class PlaceField{
    arma::mat place_field_;
    arma::cube pdf_cache_;
    
    double sigma_;
    double bin_size_;
    // how many bins around spikes to take into account
    int spread_;
    
    arma::mat gauss_;
    arma::mat gauss_1d_;

    int NBINSX;
    int NBINSY;

    int last_whlt = -1;
    int last_xb = -1;
    int last_yb = -1;
    unsigned int current_spike_number_ = 0;
//    std::vector< std::vector<unsigned int> > whltime_to_spikes_;

public:
    static const int MAX_SPIKES = 20;
    
    PlaceField(const double& sigma, const double& bin_size, const unsigned int& nbinsx, const unsigned int& nbinsy, const unsigned int& spread);
    PlaceField(const arma::mat mat, const double& sigma, const double& bin_size, const unsigned int& spread);

    // PlaceField doesn't know about its identity and doesn't check spikes
    bool AddSpike(Spike *spike);
    
    inline const double& operator()(unsigned int x, unsigned int y) const { return place_field_(x, y); }
    inline       double& operator()(unsigned int x, unsigned int y)       { return place_field_(x, y); }
    
    inline size_t Width() const { return place_field_.n_cols; }
    inline size_t Height() const { return place_field_.n_rows; }
    
    //inline const double Max() const { return place_field_.max(); }
    inline const double Max() const { return arma::max(place_field_.elem(arma::find_finite(place_field_))); }
    
    PlaceField Smooth();
    PlaceField Downsample(PlaceField& pf, int minocc);
    
    void CachePDF(const double& occupancy_factor);

    inline const double& Prob(unsigned int r, unsigned int c, unsigned int s) { return pdf_cache_(r, c, s); }
    
    inline const arma::mat& Mat() const { return place_field_; }

    void Load(const std::string path, arma::file_type ft);

    friend std::ostream& operator<<(std::ostream& output, const PlaceField& pf){
    	for (int y = 0; y < pf.NBINSY; ++y){
    		for (int x = 0; x < pf.NBINSX; ++x){
    			output << pf.place_field_(x, y) << " ";
    		}
    		output << "\n";
    	}
    	output << "\n";
    	return output;
    }

    void Zero();
};

#endif /* defined(__sdl_example__PlaceField__) */
