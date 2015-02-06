/*
 * PackageExtractorProcessor.h
 *
 *  Created on: Feb 6, 2015
 *      Author: igor
 */

#ifndef PACKAGEEXTRACTORPROCESSOR_H_
#define PACKAGEEXTRACTORPROCESSOR_H_

#include "LFPProcessor.h"

class PackageExractorProcessor : public LFPProcessor{
	const float SCALE;
	unsigned int last_reported_ = 0;
	unsigned int report_rate_;
	bool exit_on_data_over_ = false;
	bool end_reported_ = false;
	// ignore every other position sample, because it repeats the previous one
	bool skip_next_pos_ = false;
	bool read_pos_;

public:
	virtual std::string name();
    virtual void process();
    PackageExractorProcessor(LFPBuffer *buffer);
};


#endif /* PACKAGEEXTRACTORPROCESSOR_H_ */
