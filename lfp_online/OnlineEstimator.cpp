//
//  OnlineEstimator.cpp
//  sdl_example
//
//  Created by Igor Gridchyn on 06/05/14.
//  Copyright (c) 2014 Igor Gridchyn. All rights reserved.
//

#include "OnlineEstimator.h"
#include "Utils.h"

template<class T, class S>
OnlineEstimator<T, S>::OnlineEstimator(unsigned int buf_size)
: BUF_SIZE(buf_size) {
    buf = new T[BUF_SIZE];
}

template<class T, class S>
T OnlineEstimator<T, S>::get_mean_estimate(){
    return T(sum / num_samples);
}

template<class T, class S>
T OnlineEstimator<T, S>::get_std_estimate(){
    // printf("sumsq: %f, num samp: %d\n", sumsq, num_samples);
    return (T)sqrt(sumsq / num_samples - (sum / num_samples) * (sum / num_samples));
}

template<class T, class S>
void OnlineEstimator<T, S>::push(T value){
    // printf("push %f\n", value);
    
	if (Utils::Math::Isnan((float)value))
		return;

    if (num_samples < BUF_SIZE){
        num_samples ++;
    }
    else{
        sum -= buf[buf_pos];
        sumsq -= buf[buf_pos] * buf[buf_pos];
    }
    
    // update estimates
    buf[buf_pos] = value;
    sum += buf[buf_pos];
    sumsq += buf[buf_pos] * buf[buf_pos];
    
    buf_pos = (buf_pos + 1) % BUF_SIZE;
}

template<class T, class S>
unsigned int OnlineEstimator<T, S>::n_samples(){
    return num_samples;
}

template<class T, class S>
OnlineEstimator<T, S>::~OnlineEstimator(){
	delete[] buf;
}
