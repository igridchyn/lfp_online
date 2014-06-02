//
//  PlaceFieldProcessor.cpp
//  sdl_example
//
//  Created by Igor Gridchyn on 02/06/14.
//  Copyright (c) 2014 Igor Gridchyn. All rights reserved.
//

#include "LFPProcessor.h"
#include "math.h"

void PlaceField::AddSpike(Spike *spike){
    int xb = (int)round(spike->x / bin_size_);
    int yb = (int)round(spike->y / bin_size_);
    
    for(int xba = MAX(xb-spread_, 0); xba < MIN(pf_.n_cols, xb+spread_); ++xba){
        for (int yba = MAX(yb-spread_, 0); yba < MIN(pf_.n_rows, yb+spread_); ++yba) {
            pf_(yba, xba) += 1/(sqrt(2 * M_PI) * sigma_) * exp(-0.5 * (pow((spike->x - bin_size_*(0.5 + xba)), 2) + pow((spike->y - bin_size_*(0.5 + yba)), 2)) / sigma_);
        }
    }
}