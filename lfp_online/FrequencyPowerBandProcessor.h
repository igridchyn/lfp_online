/*
 * FrequencyPowerBandProcessor.h
 *
 *  Created on: Oct 9, 2014
 *      Author: igor
 */

#ifndef FREQUENCYPOWERBANDPROCESSOR_H_
#define FREQUENCYPOWERBANDPROCESSOR_H_

#include "SDLSingleWindowDisplay.h"

class FrequencyPowerBandProcessor : virtual public SDLControlInputProcessor, virtual public SDLSingleWindowDisplay{
    const int FACTOR;
    const unsigned int BUF_LEN;
    const int ANAL_RATE;

    unsigned int last_performed_an = 0;
    unsigned int channel_ = 0;

    const unsigned int SCALE;

public:
    FrequencyPowerBandProcessor(LFPBuffer *buf);
    FrequencyPowerBandProcessor(LFPBuffer *buf, std::string window_name, const unsigned int window_width, const unsigned int window_height);
    virtual void process();
    virtual inline std::string name() { return "Frequency Power Band"; }

    // SDLControlInputProcessor
    virtual void process_SDL_control_input(const SDL_Event& e);
    virtual void SetDisplayTetrode(const unsigned int& display_tetrode);

	virtual ~FrequencyPowerBandProcessor();
};


#endif /* FREQUENCYPOWERBANDPROCESSOR_H_ */
