//
//  FrequencyPowerBandProcessor.cpp
//  sdl_example
//
//  Created by Igor Gridchyn on 09/05/14.
//  Copyright (c) 2014 Igor Gridchyn. All rights reserved.
//

#include "armadillo"
#include "FrequencyPowerBandProcessor.h"

FrequencyPowerBandProcessor::FrequencyPowerBandProcessor(LFPBuffer *buf)
:FrequencyPowerBandProcessor(buf,
		buf->config_->getString("freqpow.win.name"),
		buf->config_->getInt("freqpow.win.width"),
		buf->config_->getInt("freqpow.win.height")
		){}

 FrequencyPowerBandProcessor::FrequencyPowerBandProcessor(LFPBuffer *buf, std::string window_name,
		 const unsigned int window_width, const unsigned int window_height)
	 : LFPProcessor(buf)
	 , SDLControlInputProcessor(buf)
	 , SDLSingleWindowDisplay(buf, window_name, window_width, window_height)
	 , FACTOR(buf->config_->getInt("freqpow.factor"))
	 , BUF_LEN(buf->config_->getInt("freqpow.factor") * buf->SAMPLING_RATE)
	 , ANAL_RATE(int(buf->config_->getFloat("freqpow.anal.rate.frac") * buf->config_->getInt("sampling.rate")))
	 , channel_(buf->config_->getInt("freqpow.channel", 0))
	 , SCALE(buf->config_->getInt("freqpow.scale", 1000000))
{
	 if (BUF_LEN > buffer->LFP_BUF_LEN){
		 Log("ERROR: Buffer length for Frequency power band estimation is larger than the LFP buffer!");
		 exit(LFPONLINE_BUFFER_TOO_SHORT);
	 }

	 if (!buffer->is_valid_channel_[channel_]){
		 Log("ERROR: requested channel is not present in the tetrode config");
		 exit(LFPONLINE_REQUESTING_INVALID_CHANNEL);
	 }
}

// http://arma.sourceforge.net/docs.html#fft
void FrequencyPowerBandProcessor::process(){
    
    if (BUF_LEN >= buffer->buf_pos || buffer->last_pkg_id < last_performed_an + ANAL_RATE){
        return;
    }
    
    arma::Mat<double> X(BUF_LEN, 1);
    int i=0;
    signal_type *buf_fft = buffer->signal_buf[channel_] + buffer->buf_pos-BUF_LEN;
    for (; buf_fft != buffer->signal_buf[channel_] + buffer->buf_pos; ++buf_fft, ++i  ) {
        X(i, 0) = *buf_fft;
    }
    
    arma::cx_mat freq_pow;

    freq_pow = arma::fft(X);
    
    last_performed_an = buffer->last_pkg_id;
    
    RenderClear();
    
    SDL_SetRenderDrawColor(renderer_, 255,255,255,255);
    int prevy = 0;
    for (int i=FACTOR/2; i < 200 * FACTOR; i+=FACTOR/2){
        double mag = 0;
        
        for (int d=-FACTOR/2; d<=FACTOR/2; ++d) {
            int j = i + d;
            mag += sqrt(freq_pow(j, 0).real() * freq_pow(j, 0).real() + freq_pow(j, 0).imag() * freq_pow(j, 0).imag());
        }
        mag /= FACTOR + 1 - (FACTOR % 2);
        
        int y = int(window_height_ * (1-mag/SCALE));
        SDL_RenderDrawLine(renderer_, 2*i, prevy, 2*i+4, y);
        prevy = y;
    }

    Render();
}

void FrequencyPowerBandProcessor::process_SDL_control_input(
		const SDL_Event& e) {

	if( e.type == SDL_KEYDOWN ){

		switch(e.key.keysym.sym){
			case SDLK_0:{
				channel_ = 0;
				break;
			}
			case SDLK_1:{
				channel_ = 1;
				break;
			}
			case SDLK_2:{
				channel_ = 2;
				break;
			}
			case SDLK_3:{
				channel_ = 3;
				break;
			}
			case SDLK_4:{
				channel_ = 4;
				break;
			}

			case SDLK_5:{
				channel_ = 5;
				break;
			}

			case SDLK_6:{
				channel_ = 6;
				break;
			}

			case SDLK_7:{
				channel_ = 7;
				break;
			}

			default:
				break;
		}
	}
}

void FrequencyPowerBandProcessor::SetDisplayTetrode(
		const unsigned int& display_tetrode) {
	channel_ = buffer->tetr_info_->tetrode_channels[display_tetrode][0];
}

FrequencyPowerBandProcessor::~FrequencyPowerBandProcessor(){
	buffer->Log("Deleting Frequency Power Band Processor...");
}
