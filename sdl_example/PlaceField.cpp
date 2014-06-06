//
//  PlaceField.cpp
//  sdl_example
//
//  Created by Igor Gridchyn on 06/06/14.
//  Copyright (c) 2014 Igor Gridchyn. All rights reserved.
//

#include "PlaceField.h"


PlaceField::PlaceField(const double& sigma, const double& bin_size, const unsigned int& nbins, const unsigned int& spread)
: sigma_(sigma)
, bin_size_(bin_size)
, spread_(spread){
    place_field_ = arma::mat(nbins, nbins, arma::fill::zeros);
}

void PlaceField::AddSpike(Spike *spike){
    int xb = (int)round(spike->x / bin_size_);
    int yb = (int)round(spike->y / bin_size_);
    
    // TODO: ? reconstruct ?? (in previous proc)
    // unknown coord
    if (spike->x == 1023 || spike->y == 1023){
        return;
    }
    
    place_field_(yb, xb) += 1;
    
    // normalizer - sum of all values to be added
    // TODO: cache vals
    // -- SMOOTHING RAW MAPS BY REQUEST
    //    double norm = .0f;
    //    for(int xba = MAX(xb-spread_, 0); xba < MIN(place_field_.n_cols, xb+spread_); ++xba){
    //        for (int yba = MAX(yb-spread_, 0); yba < MIN(place_field_.n_rows, yb+spread_); ++yba) {
    //            double add = 1/(sqrt(2 * M_PI) * sigma_) * exp(-0.5 * (pow((spike->x - bin_size_*(0.5 + xba)), 2) + pow((spike->y - bin_size_*(0.5 + yba)), 2)) / (sigma_ * sigma_));
    //            norm += add;
    //        }
    //    }
    //
    //    for(int xba = MAX(xb-spread_, 0); xba < MIN(place_field_.n_cols, xb+spread_); ++xba){
    //        for (int yba = MAX(yb-spread_, 0); yba < MIN(place_field_.n_rows, yb+spread_); ++yba) {
    //            double add = 1/(sqrt(2 * M_PI) * sigma_) * exp(-0.5 * (pow((spike->x - bin_size_*(0.5 + xba)), 2) + pow((spike->y - bin_size_*(0.5 + yba)), 2)) / (sigma_ * sigma_));
    //            place_field_(yba, xba) += add / norm;
    //        }
    //    }
}

PlaceField PlaceField::Smooth(){
    PlaceField spf(sigma_, bin_size_, place_field_.n_cols, spread_);
    int width = place_field_.n_cols;
    int height = place_field_.n_rows;
    
    // get Gaussian for smoothing, summing up to 1
    arma::mat gauss(spread_*2 + 1, spread_*2 + 1, arma::fill::zeros);
    double g_sum = .0f;
    for (int dx = -spread_; dx <= spread_; ++dx){
        for (int dy = -spread_; dy <= spread_; ++dy) {
            gauss(dy+spread_, dx+spread_) = Utils::Math::Gauss2D(sigma_ / bin_size_, dx, dy);
            g_sum += gauss(dy+spread_, dx+spread_);
        }
    }
    for (int dx = -spread_; dx <= spread_; ++dx){
        for (int dy = -spread_; dy <= spread_; ++dy) {
            gauss(dy+spread_, dx+spread_) /= g_sum;
            //            std::cout << gauss(dy+spread_, dx+spread_) << " ";
        }
    }
    
    // compute normalizers for border / corner cases
    arma::mat border_normalizers(spread_ + 1, spread_ + 1, arma::fill::zeros);
    for (int bx=0; bx <= spread_; ++bx) {
        for (int by=0; by <= spread_; ++by) {
            for (int dx = -spread_ + bx; dx <= spread_; ++dx){
                for (int dy = -spread_ + by; dy <= spread_; ++dy) {
                    border_normalizers(by, bx) += gauss(dy+spread_, dx+spread_);
                }
            }
        }
    }
    
    // smooth place field
    // TODO: smoothing on the edge corners
    for (int x=spread_; x < place_field_.n_cols-spread_; ++x) {
        for (int y=spread_; y < place_field_.n_rows-spread_; ++y) {
            for (int dx=-spread_; dx <= spread_; ++dx) {
                for (int dy=-spread_; dy <= spread_; ++dy) {
                    spf(y, x) += place_field_(y + dy, x + dx) * gauss(dy+spread_, dx+spread_);
                }
            }
        }
    }
    
    // smooth on the edges
    // !!! TODO: account for different width / height of PF
    for (int b=0; b < spread_; ++b) {
        // on all four sides
        for (int x=spread_; x < place_field_.n_cols-spread_; ++x) {
            for (int dx=-spread_; dx <= spread_; ++dx) {
                for (int dy=-spread_ + b; dy <= spread_; ++dy) {
                    spf(b, x)        += place_field_(b, x + dx) * gauss(dy+spread_, dx+spread_);
                    spf(height - 1 - b, x) += place_field_(height - 1 - b, x + dx) * gauss(dy+spread_, dx+spread_);
                    spf(x, b)        += place_field_(x + dx, b) * gauss(dx+spread_, dy+spread_);
                    spf(x, height - 1 - b) += place_field_(x + dx, height - 1 - b) * gauss(dx+spread_, dy+spread_);
                }
            }
            
            spf(b, x)               /= border_normalizers(b, 0);
            spf(height - 1 - b, x)  /= border_normalizers(b, 0);
            spf(x, b)               /= border_normalizers(b, 0);
            spf(x, height - 1 - b)  /= border_normalizers(b, 0);
        }
    }
    
    // smooth in the corners
    // bx, by - distances of center from the border
    for (int bx=0; bx <= spread_; ++bx) {
        for (int by=0; by <= spread_; ++by) {
            // in all 4
            for (int dx = -bx; dx <= spread_; ++dx) {
                for (int dy = -by; dy <= spread_; ++dy) {
                    spf(by, bx) += place_field_(by + dy, bx + dx) * gauss(dy+spread_, dx+spread_);
                    spf(height - 1 - by, bx) += place_field_(height - 1 - by - dy, bx + dx) * gauss(dy+spread_, dx+spread_);
                    spf(by, width - 1 - bx) += place_field_(by + dy, width - 1 - bx - dx) * gauss(dy+spread_, dx+spread_);
                    spf(height - 1 - by, width - 1 -bx) += place_field_(height - 1 - by - dy, width - 1 - bx - dx) * gauss(dy+spread_, dx+spread_);
                }
            }
            
            spf(by, bx) /= border_normalizers(by, bx);
            spf(height - 1 - by, bx) /= border_normalizers(by, bx);
            spf(by, width - 1 - bx) /= border_normalizers(by, bx);
            spf(height - 1 - by, width - 1 -bx) /= border_normalizers(by, bx);
        }
    }
    
    // std::cout << spf.place_field_ << "\n\n\n";
    
    return spf;
}

void PlaceField::CachePDF(PlaceField::PDFType pdf_type){
    pdf_cache_ = arma::cube(place_field_.n_rows, place_field_.n_cols, MAX_SPIKES, arma::fill::zeros);
    
    for (int r=0; r < place_field_.n_rows; ++r) {
        for (int c=0; c < place_field_.n_cols; ++c) {
            const double& lambda = place_field_(r,c);
            double logp = -lambda;
            pdf_cache_(r, c, 0) = logp;
            
            for (int s = 1; s < MAX_SPIKES; ++s) {
                logp += log(lambda / s);
                pdf_cache_(r, c, s) = logp;
            }
        }
    }
    
    // TODO: cache log of occupancy_smoothed
}