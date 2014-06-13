/*
 * WhlFileReaderProcessor.h
 *
 *  Created on: Jun 12, 2014
 *      Author: igor
 */

#ifndef WHLFILEREADERPROCESSOR_H_
#define WHLFILEREADERPROCESSOR_H_

#include "LFPProcessor.h"

class WhlFileReaderProcessor: public LFPProcessor {
	const std::string whl_path_;
	std::ifstream whl_stream_;

	const unsigned int& sampling_rate_;
	int last_pos_pkg_id_ = 0;

public:
	WhlFileReaderProcessor(LFPBuffer *buffer, const std::string& whl_path, const unsigned int& sampling_rate);
	virtual ~WhlFileReaderProcessor();

	//LFPProcessor
	virtual void process();
};

#endif /* WHLFILEREADERPROCESSOR_H_ */
