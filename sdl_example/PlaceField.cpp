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
//    whltime_to_spikes_ = std::vector<std::vector <unsigned int> >();
//    whltime_to_spikes_.resize(nbinsx * nbinsy);

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

    // 1d case
    gauss_1d_ = arma::mat(spread_*2 + 1, 1, arma::fill::zeros);
    g_sum = .0f;
    for (int dx = -spread_; dx <= spread_; ++dx){
    	gauss_1d_(dx + spread, 0) = Utils::Math::Gauss1D(sigma_ / bin_size_, dx);
    	g_sum += gauss_1d_(dx + spread, 0);
    }
    // normalize 1d
    for (int dx = -spread_; dx <= spread_; ++dx){
        gauss_1d_(dx + spread_, 0) /= g_sum;
    }
}

void PlaceField::Load(const std::string path, arma::file_type ft){
	place_field_.load(path, ft);
}

bool PlaceField::AddSpike(Spike *spike){
	// add spike to list for future sampling
//	int whlt = spike->pkg_id_ / 480;

	// the problem is that spikes may have different bins within the same whlt!
    int xb = (int)round(spike->x / bin_size_ - 0.5);
    int yb = (int)round(spike->y / bin_size_ - 0.5);

//	if (whlt > last_whlt){
//		// create new entry in list for this bin
//		if (whltime_to_spikes_[NBINSX * last_yb + last_xb].size() < current_spike_number_ + 1){
//			//for (unsigned int d = 0; d < (current_spike_number_ + 1) - whltime_to_spikes_[NBINSX * last_yb + last_xb].size(); ++d)
//				//whltime_to_spikes_[NBINSX * last_yb + last_xb].push_back(0);
//			whltime_to_spikes_[NBINSX * last_yb + last_xb].resize(current_spike_number_ + 1);
//			std::cout << "Resize to: " << current_spike_number_ << "\n";
//		}
//
//		whltime_to_spikes_[NBINSX * last_yb + last_xb][current_spike_number_] += 1;
//
//		current_spike_number_ = 1;
//
//		last_whlt = whlt;
//		last_xb = xb;
//		last_yb = yb;
//	} else {
//		// if still withing same whl sample -> add spike count
//		// whltime_to_spikes_[NBINSX * last_yb + last_xb][whltime_to_spikes_[NBINSX * last_yb + last_xb].size() - 1] += 1;
//		current_spike_number_ += 1;
//	}

//	if (whltime_to_spikes_.find(whlt) == whltime_to_spikes_.end())
//		whltime_to_spikes_[whlt] = 1;
//	else{
//		whltime_to_spikes_[whlt] ++;
//	}

    if (yb < 0){
    	yb = 0;
    }
    if (xb < 0){
    	xb = 0;
    }


    if (xb >= NBINSX || yb >= NBINSY){
    	return false;
    }

    //place_field_(yb, xb) += 1;

    for (int xs=-1;xs<=1;++xs){
    	for (int ys=-1;ys<=1;++ys){
    		if ((int)xb+xs < 0 || (int)yb+ys < 0 || xb+xs == NBINSX || yb+ys == NBINSY)
    			continue;

    		float dx = MIN(spike->x+0.5, (xb+xs+1)*bin_size_) - MAX(spike->x-0.5, (xb+xs)*bin_size_);
    		float dy = MIN(spike->y+0.5, (yb+ys+1)*bin_size_) - MAX(spike->y-0.5, (yb+ys)*bin_size_);

    		if (dx < 0 || dy < 0)
    			continue;

    		place_field_(yb+ys, xb+xs) += dx*dy;
    	}
    }

    return true;
}

PlaceField PlaceField::Downsample(PlaceField& occ, int minocc){

	PlaceField spf(sigma_, bin_size_, place_field_.n_cols, place_field_.n_rows, spread_);
//
//    for (int x=0; x < (int)place_field_.n_cols; ++x) {
//		for (int y=0; y < (int)place_field_.n_rows; ++y){
//			if(occ(y, x) < minocc)
//				continue;
//
//			// calculate number of 0 occurrences from occupancy and counts histogram
//			unsigned int totsum = 0;
//			for (unsigned int ns = 0; ns < whltime_to_spikes_[NBINSX * y + x].size(); ++ns)
//				totsum += whltime_to_spikes_[NBINSX * y + x][ns];
//			if (whltime_to_spikes_[NBINSX * y + x].size() > 0)
//					whltime_to_spikes_[NBINSX * y + x][0] = occ(y,x) - totsum;
//			else
//				whltime_to_spikes_[NBINSX * y + x].push_back(occ(y,x) - totsum);

			// sample if spikes or	no-spikes
//			for (int s=0; s < minocc; ++s){
//				int spikenos = rand() % (int)occ(y, x);
//
//				int cumsum = whltime_to_spikes_[NBINSX * y + x][0];
//				int ispiken = 0;
//				while (cumsum < spikenos){
//					ispiken ++;
//					cumsum += whltime_to_spikes_[NBINSX * y + x][ispiken];
//				}
//				spf(y, x) += ispiken;

				// old way - with every window counted
//				if((unsigned int)spikenos >= whltime_to_spikes_[NBINSX * y + x].size()){
//					// no spike sampled - continue
//					continue;
//				}
//				spf(y, x) += whltime_to_spikes_[NBINSX * y + x][spikenos];
//			}
//		}
//    }

	return spf;
}

PlaceField PlaceField::Smooth(){
    PlaceField spf(sigma_, bin_size_, place_field_.n_cols, place_field_.n_rows, spread_);

    // smooth place field
//    for (int x=0; x < (int)place_field_.n_cols; ++x) {
//        for (int y=0; y < (int)place_field_.n_rows; ++y) {
//        	// part of the gaussian that was not accounted for because of nans and infs and has to be noramlized by
//        	double ignored_part = .0;
//            for (int dx=-spread_; dx <= spread_; ++dx) {
//                for (int dy=-spread_; dy <= spread_; ++dy) {
//					if (y + dy <0 || x + dx < 0 || y + dy >= (int)place_field_.n_rows || x + dx >= (int)place_field_.n_cols || isinf((float)place_field_(y + dy, x + dx)) || Utils::Math::Isnan((float)place_field_(y + dy, x + dx))){
//                		ignored_part += gauss_(dy+spread_, dx+spread_);
//                		continue;
//                	}
//
//                	double val = place_field_(y + dy, x + dx);
//                	val *= gauss_(dy+spread_, dx+spread_);
//                    spf(y, x) += val;
//                }
//            }
//            // normalized for only accounted part of the gaussian
//            spf(y, x) /= 1.0 - ignored_part;
//        }
//    }

    // 2 passes of 1D smoothing
    PlaceField spf2(sigma_, bin_size_, place_field_.n_cols, place_field_.n_rows, spread_);

    for (int x=0; x < (int)place_field_.n_cols; ++x) {
            for (int y=0; y < (int)place_field_.n_rows; ++y) {
            	double ignored_part = .0;
            	for (int dx=-spread_; dx <= spread_; ++dx) {
            		if (x + dx < 0|| x + dx >= (int)place_field_.n_cols || isinf((float)place_field_(y, x + dx)) || Utils::Math::Isnan((float)place_field_(y, x + dx))){
            			ignored_part += gauss_1d_(dx+spread_, 0);
            			continue;
            		}
            		spf(y,x) += place_field_(y, x + dx) * gauss_1d_(dx + spread_, 0);
            	}
            	spf(y,x) /= 1.0 - ignored_part;
            }
    }
    for (int x=0; x < (int)place_field_.n_cols; ++x) {
            for (int y=0; y < (int)place_field_.n_rows; ++y) {
            	double ignored_part = .0;
            	for (int dy=-spread_; dy <= spread_; ++dy) {
            		if (y + dy < 0|| y + dy >= (int)place_field_.n_rows || isinf((float)spf(y + dy, x)) || Utils::Math::Isnan((float)spf(y + dy, x))){
            			ignored_part += gauss_1d_(dy+spread_, 0);
            			continue;
            		}
            		spf2(y,x) += spf(y + dy, x) * gauss_1d_(dy + spread_, 0);
            	}
            	spf2(y,x) /= 1.0 - ignored_part;
            }
    }


    return spf2;
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
