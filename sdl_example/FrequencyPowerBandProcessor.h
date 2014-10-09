/*
 * FrequencyPowerBandProcessor.h
 *
 *  Created on: Oct 9, 2014
 *      Author: igor
 */

#ifndef FREQUENCYPOWERBANDPROCESSOR_H_
#define FREQUENCYPOWERBANDPROCESSOR_H_

#include "SDLSingleWindowDisplay.h"

class FrequencyPowerBandProcessor : virtual public SDLSingleWindowDisplay, virtual public SDLControlInputProcessor{
    const int FACTOR;
    const int BUF_LEN;
    const int ANAL_RATE;

    unsigned int last_performed_an = 0;
    unsigned int channel_ = 0;

public:
    FrequencyPowerBandProcessor(LFPBuffer *buf);
    FrequencyPowerBandProcessor(LFPBuffer *buf, std::string window_name, const unsigned int window_width, const unsigned int window_height);
    virtual void process();

    // SDLControlInputProcessor
    virtual void process_SDL_control_input(const SDL_Event& e);
    virtual void SetDisplayTetrode(const unsigned int& display_tetrode);

	virtual ~FrequencyPowerBandProcessor();
};


#endif /* FREQUENCYPOWERBANDPROCESSOR_H_ */
