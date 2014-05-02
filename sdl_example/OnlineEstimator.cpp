//
//  OnlineEstimator.cpp
//  sdl_example
//
//  Created by Igor Gridchyn on 02/05/14.
//  Copyright (c) 2014 Igor Gridchyn. All rights reserved.
//

#ifndef ONLINE_ESTIMATOR
#define ONLINE_ESTIMATOR

//==========================================================================================

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

//==========================================================================================

template<class T>
T OnlineEstimator<T>::get_mean_estimate(){
    return sumsq / num_samples;
}

template<class T>
T OnlineEstimator<T>::get_std_estimate(){
    // printf("sumsq: %f, num samp: %d\n", sumsq, num_samples);
    return (T)sqrt(sumsq / num_samples - (sum / num_samples) * (sum / num_samples));
}

template<class T>
void OnlineEstimator<T>::push(T value){
    // printf("push %f\n", value);
    
    // TODO: ignore to optimize ?
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

#endif
