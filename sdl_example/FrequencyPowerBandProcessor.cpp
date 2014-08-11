//
//  FrequencyPowerBandProcessor.cpp
//  sdl_example
//
//  Created by Igor Gridchyn on 09/05/14.
//  Copyright (c) 2014 Igor Gridchyn. All rights reserved.
//

#include "armadillo"
#include "LFPProcessor.h"

FrequencyPowerBandProcessor::FrequencyPowerBandProcessor(LFPBuffer *buf)
:FrequencyPowerBandProcessor(buf,
		buf->config_->getString("freqpow.win.name"),
		buf->config_->getInt("freqpow.win.width"),
		buf->config_->getInt("freqpow.win.height")
		){}

 FrequencyPowerBandProcessor::FrequencyPowerBandProcessor(LFPBuffer *buf, std::string window_name,
		 const unsigned int window_width, const unsigned int window_height)
 	 	 : LFPProcessor(buf)
 	 	 , SDLSingleWindowDisplay(window_name, window_width, window_height)
		 , FACTOR(buf->config_->getInt("freqpow.factor"))
		 , BUF_LEN(buf->config_->getInt("freqpow.factor") * buf->SAMPLING_RATE)
		 , ANAL_RATE(buf->config_->getInt("freqpow.anal.rate.frac") * buf->config_->getFloat("freqpow.anal.rate.frac"))
		 {}

// http://arma.sourceforge.net/docs.html#fft
void FrequencyPowerBandProcessor::process(){
    
    if (BUF_LEN >= buffer->buf_pos || buffer->last_pkg_id < last_performed_an + ANAL_RATE){
        return;
    }
    
    arma::Mat<double> X(BUF_LEN, 1);
    int i=0;
    short *buf_fft = buffer->signal_buf[0] + buffer->buf_pos-BUF_LEN;
    for (; buf_fft != buffer->signal_buf[0] + buffer->buf_pos; ++buf_fft, ++i  ) {
        X(i, 0) = *buf_fft;
    }
    
    arma::cx_mat freq_pow;
    // TODO: online !!!
    freq_pow = arma::fft(X);
    
    // DEBUG
//    for (int i=1; i < BUF_LEN; i+=2){
//        float mag = sqrt(freq_pow(i, 0).real() * freq_pow(i, 0).real() + freq_pow(i, 0).imag() * freq_pow(i, 0).imag());
//        std::cout << mag << " ";
//    }
    
    last_performed_an = buffer->last_pkg_id;
    
    // DISPLAY
    // TODO: extract a separate processor
    SDL_SetRenderTarget(renderer_, texture_);
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
    SDL_RenderClear(renderer_);
    SDL_RenderPresent(renderer_);
    
    // TODO: plot lines at landmark points
    SDL_SetRenderDrawColor(renderer_, 255,255,255,255);
    int prevy = 0;
    for (int i=FACTOR/2; i < 200 * FACTOR; i+=FACTOR/2){
        float mag = 0;
        
        for (int d=-FACTOR/2; d<=FACTOR/2; ++d) {
            int j = i + d;
            mag += sqrt(freq_pow(j, 0).real() * freq_pow(j, 0).real() + freq_pow(j, 0).imag() * freq_pow(j, 0).imag());
        }
        mag /= FACTOR + 1 - (FACTOR % 2);
        
        int y = window_height_ * (1-mag/(100000000));
        SDL_RenderDrawLine(renderer_, 2*i, prevy, 2*i+4, y);
        prevy = y;
    }

    // RENDER
    SDL_SetRenderTarget(renderer_, NULL);
    SDL_RenderCopy(renderer_, texture_, NULL, NULL);
    SDL_RenderPresent(renderer_);
}
