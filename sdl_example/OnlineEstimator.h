//
//  OnlineEstimator.h
//  sdl_example
//
//  Created by Igor Gridchyn on 06/05/14.
//  Copyright (c) 2014 Igor Gridchyn. All rights reserved.
//

#ifndef __sdl_example__OnlineEstimator__
#define __sdl_example__OnlineEstimator__

#include <iostream>

template<class T, class S>
class OnlineEstimator{
    const unsigned int BUF_SIZE;
    
    T *buf = nullptr;
    unsigned int buf_pos = 0;
    unsigned int num_samples = 0;
    
    S sum = 0;
    S sumsq = 0;
    
public:
    OnlineEstimator(unsigned int buf_size = (1 << 15));
    ~OnlineEstimator();
    
    void push(T value);
    T get_mean_estimate();
    T get_std_estimate();
    
    unsigned int n_samples();
};

#endif /* defined(__sdl_example__OnlineEstimator__) */
