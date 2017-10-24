//
//  PlaceField.cpp
//  sdl_example
//
//  Created by Igor Gridchyn on 06/06/14.
//  Copyright (c) 2014 Igor Gridchyn. All rights reserved.
//

#include "PlaceField.h"

PlaceField::PlaceField(const double& sigma, const double& bin_size, const unsigned int& nbinsx, const unsigned int& nbinsy, const unsigned int& spread)
: sigma_(sigma)
, bin_size_(bin_size)
, spread_(spread)
, NBINSX(nbinsx)
, NBINSY(nbinsy){
    place_field_ = arma::mat(nbinsy, nbinsx, arma::fill::zeros);

    gauss_ = arma::mat(spread_*2 + 1, spread_*2 + 1, arma::fill::zeros);
    double g_sum = .0f;
    for (int dx = -spread_; dx <= spread_; ++dx){
        for (int dy = -spread_; dy <= spread_; ++dy) {
        	gauss_(dy+spread_, dx+spread_) = Utils::Math::Gauss2D(sigma_ / bin_size_, dx, dy);
            g_sum += gauss_(dy+spread_, dx+spread_);
        }
    }
    // normalize
    for (int dx = -spread_; dx <= spread_; ++dx){
        for (int dy = -spread_; dy <= spread_; ++dy) {
        	gauss_(dy+spread_, dx+spread_) /= g_sum;
        }
    }
}

void PlaceField::Load(const std::string path, arma::file_type ft){
	place_field_.load(path, ft);
}

bool PlaceField::AddSpike(Spike *spike){
    int xb = (int)round(spike->x / bin_size_ - 0.5);
    int yb = (int)round(spike->y / bin_size_ - 0.5);

    if (yb < 0){
    	yb = 0;
    }
    if (xb < 0){
    	xb = 0;
    }


    if (xb >= NBINSX || yb >= NBINSY){
    	return false;
    }

    place_field_(yb, xb) += 1;
    return true;
}

PlaceField PlaceField::Smooth(){
    PlaceField spf(sigma_, bin_size_, place_field_.n_cols, place_field_.n_rows, spread_);

    // smooth place field
    for (int x=0; x < (int)place_field_.n_cols; ++x) {
        for (int y=0; y < (int)place_field_.n_rows; ++y) {
        	// part of the gaussian that was not accounted for because of nans and infs and has to be noramlized by
        	double ignored_part = .0;
            for (int dx=-spread_; dx <= spread_; ++dx) {
                for (int dy=-spread_; dy <= spread_; ++dy) {
					if (y + dy <0 || x + dx < 0 || y + dy >= (int)place_field_.n_rows || x + dx >= (int)place_field_.n_cols || isinf((float)place_field_(y + dy, x + dx)) || Utils::Math::Isnan((float)place_field_(y + dy, x + dx))){
                		ignored_part += gauss_(dy+spread_, dx+spread_);
                		continue;
                	}

                	double val = place_field_(y + dy, x + dx);
                	val *= gauss_(dy+spread_, dx+spread_);
                    spf(y, x) += val;
                }
            }
            // normalized for only accounted part of the gaussian
            spf(y, x) /= 1.0 - ignored_part;
        }
    }
    
    return spf;
}

void PlaceField::CachePDF(const PlaceField& occupancy, const double& occupancy_factor){
    pdf_cache_ = arma::cube(place_field_.n_rows, place_field_.n_cols, MAX_SPIKES, arma::fill::zeros);
    
    for (size_t r=0; r < place_field_.n_rows; ++r) {
        for (size_t c=0; c < place_field_.n_cols; ++c) {
            const double& lambda = occupancy(r, c) > 0 ? (place_field_(r, c) / occupancy(r, c) * occupancy_factor) : 0;
            double logp = -lambda;
            pdf_cache_(r, c, 0) = logp;
            
            for (int s = 1; s < MAX_SPIKES; ++s) {
                logp += lambda > 0 ? log(lambda / s) : -1000000; // p(s) = exp(-lambda) * lambda^s / s!; log(p(s)/p(s-1)) = log(lambda/s)
                pdf_cache_(r, c, s) = logp;
            }
        }
    }
    
    // TODO: cache log of occupancy_smoothed
}

PlaceField::PlaceField(const arma::mat mat, const double& sigma,
		const double& bin_size, const unsigned int& spread)
	: place_field_(mat)
	, sigma_(sigma)
	, bin_size_(bin_size)
	, spread_(spread)
	, NBINSX(mat.n_rows)
	, NBINSY(mat.n_cols)
{
}

void PlaceField::Zero(){
	place_field_.zeros();
}
