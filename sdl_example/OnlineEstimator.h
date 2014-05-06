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

template<class T>
class OnlineEstimator{
    static const int BUF_SIZE = 2 << 24;
    
    T buf[BUF_SIZE];
    unsigned int buf_pos = 0;
    unsigned int num_samples = 0;
    
    T sum = 0;
    T sumsq = 0;
    
public:
    //OnlineEstimator();
    void push(T value);
    T get_mean_estimate();
    T get_std_estimate();
};

#endif /* defined(__sdl_example__OnlineEstimator__) */
